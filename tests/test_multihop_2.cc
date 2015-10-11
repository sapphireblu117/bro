#include "broker/broker.hh"
#include "broker/endpoint.hh"
#include "broker/message_queue.hh"
#include "broker/report.hh"
#include "testsuite.h"
#include <vector>
#include <string>
#include <iostream>

// A test of "event" style messages -- vectors of data w/ first element
// being the event name.

using namespace broker;

int main(int argc, char** argv)
	{
	init();

	/* Overlay configuration
	 *       n0 [a]
	 *       |
 	 *       n1 [b]
	 *      /  \
	 * [c] n2  n3 [b]
	 *         |
	 *         n4 [d]
	 */

	// Node 0
	endpoint node0("node0");
	message_queue q0("a", node0);
	// Node 1
	endpoint node1("node1");
	message_queue q1("b", node1);
	// Node 2
	endpoint node2("node2");
	message_queue q2("c", node2);
	// Node 3
	endpoint node3("node3");
	message_queue q3("b", node3);
	// Node 4
	endpoint node4("node4");
	message_queue q4("d", node4);

	// Connections
	node1.peer(node0);
	if ( node1.outgoing_connection_status().need_pop().front().status !=
	     outgoing_connection_status::tag::established)
		{
		BROKER_TEST(false);
		return 1;
		}

	node2.peer(node1);
	if ( node2.outgoing_connection_status().need_pop().front().status !=
	     outgoing_connection_status::tag::established)
		{
		BROKER_TEST(false);
		return 1;
		}

	node3.peer(node1);
	if ( node3.outgoing_connection_status().need_pop().front().status !=
	     outgoing_connection_status::tag::established)
		{
		BROKER_TEST(false);
		return 1;
		}

	node4.peer(node3);
	if ( node4.outgoing_connection_status().need_pop().front().status !=
	     outgoing_connection_status::tag::established)
		{
		BROKER_TEST(false);
		return 1;
		}

	// Sending and receiving
	std::vector<message> pings;
	std::vector<message> pongs;

	// node2 sends ping messages
	for ( int i = 0; i < 4; ++i )
		{
		pings.push_back(message{"ping", vector{i, "yo"}});
		node2.send("a", pings[i], 0x02);
		}

	// node0 receives pings and sends pongs
	while ( pongs.size() != 4 )
		{
		for ( auto& msg : q0.need_pop() )
			{
			std::cout << "node0: ping received, msg size " << msg.size() << std::endl;
			msg[0] = "pong";
			node0.send("d", msg, 0x02);
			pongs.push_back(std::move(msg));
			}
		}

	std::vector<message> returned;

	// node4 receives pongs
	while ( returned.size() != 4 )
		for ( auto& msg : q4.need_pop() )
			{
			std::cout << "node4: pong received" << std::endl;
			returned.push_back(std::move(msg));
			}

	BROKER_TEST(returned.size() == 4);
	BROKER_TEST(returned == pongs);

	for ( int i = 0; i < (int)returned.size(); ++i )
		{
		BROKER_TEST(returned[i][0] == "pong");
		const auto& v = *get<vector>(returned[i][1]);
		BROKER_TEST(v[0] == i);
		BROKER_TEST(v[1] == "yo");
		}

	return BROKER_TEST_RESULT();
	}