/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Network topology
//
//        h0
//        |
//       ----------
//       | Switch |
//       ----------
//        |
//        h1
//
// (TODO, below is inaccurate)
// - CBR/UDP flows from n0 to n1 and from n3 to n0
// - DropTail queues
// - Tracing of queues and packet receptions to file "csma-bridge.tr"

#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

#include "ns3/probing-client.h"
#include "ns3/probing-server.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MiniTopology");

void RTTMeasurement(Ptr<OutputStreamWrapper> stream, double newValue)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << ','
                         << newValue << std::endl;
}

int main(int argc, char *argv[])
{
    //
    // Users may find it convenient to turn on explicit debugging
    // for selected modules; the below lines suggest how to do this
    //
#if 1
    LogComponentEnable("MiniTopology", LOG_LEVEL_INFO);
#endif

    //
    // Allow the user to override any of the defaults and the above Bind() at
    // run-time, via command-line arguments
    //
    CommandLine cmd;
    cmd.Parse(argc, argv);

    //
    // Explicitly create the nodes required by the topology (shown above).
    //
    NS_LOG_INFO("Create nodes.");
    NodeContainer hosts;
    hosts.Create(2);
    // Keep references to host and server
    auto clientNode = hosts.Get(0);
    auto serverNode = hosts.Get(1);

    NodeContainer csmaSwitch;
    csmaSwitch.Create(1);

    NS_LOG_INFO("Build Topology");
    CsmaHelper csma;
    csma.SetChannelAttribute("FullDuplex", BooleanValue(true));
    csma.SetChannelAttribute("DataRate", StringValue("5Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

    // Create the csma links, from each terminal to the switch

    NetDeviceContainer hostDevices;
    NetDeviceContainer switchDevices;

    for (int i = 0; i < 2; i++)
    {
        NetDeviceContainer link = csma.Install(NodeContainer(hosts.Get(i), csmaSwitch));
        hostDevices.Add(link.Get(0));
        switchDevices.Add(link.Get(1));
    }

    // Create the bridge netdevice, which will do the packet switching
    Ptr<Node> switchNode = csmaSwitch.Get(0);
    BridgeHelper bridge;
    bridge.Install(switchNode, switchDevices);

    // Add internet stack and IP addresses to the hosts
    NS_LOG_INFO("Setup stack and assign IP Addresses.");
    InternetStackHelper internet;
    internet.Install(hosts);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    auto addresses = ipv4.Assign(hostDevices);
    // Get Address: Device with index 1, address 0 (there is only one address)
    auto server_address = addresses.GetAddress(1, 0);

    // Create the probing client and server apps
    NS_LOG_INFO("Create Applications.");
    uint16_t port = 9; // Discard port (RFC 863)

    auto probing_client = CreateObjectWithAttributes<ProbingClient>(
        "RemoteAddress", AddressValue(server_address),
        "RemotePort", UintegerValue(port));
    auto probing_server = CreateObjectWithAttributes<ProbingServer>(
        "Port", UintegerValue(port));

    // Install apps
    clientNode->AddApplication(probing_client);
    serverNode->AddApplication(probing_server);

    probing_server->SetStartTime(Seconds(0));
    probing_server->SetStopTime(Seconds(20));
    probing_client->SetStartTime(Seconds(1));
    probing_client->SetStopTime(Seconds(19));

    NS_LOG_INFO("Configure Tracing.");
    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream("rtt.csv");
    probing_client->TraceConnectWithoutContext("RTT", MakeBoundCallback(&RTTMeasurement, stream));

    //
    // Configure tracing of all enqueue, dequeue, and NetDevice receive events.
    // Trace output will be sent to the file "csma-bridge.tr"
    //
    //AsciiTraceHelper ascii;
    //csma.EnableAsciiAll(ascii.CreateFileStream("csma-bridge.tr"));

    //
    // Also configure some tcpdump traces; each interface will be traced.
    // The output files will be named:
    //     csma-bridge-<nodeId>-<interfaceId>.pcap
    // and can be read by the "tcpdump -r" command (use "-tt" option to
    // display timestamps correctly)
    //
    csma.EnablePcapAll("csma-bridge", false);

    //
    // Now, do the actual simulation.
    //
    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");
}
