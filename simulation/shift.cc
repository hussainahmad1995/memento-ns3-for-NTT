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
//                               disturbance
//                                  |
//        sender --- switchA --- switchB --- receiver
//
//  The "disturbance" host is used to introduce changes in the network
//  conditions.
//

#include <iostream>
#include <fstream>
#include <unordered_map>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

#include "ns3/cdf-application.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MiniTopology");

const auto TCP = TypeIdValue(TcpSocketFactory::GetTypeId());
const auto UDP = TypeIdValue(UdpSocketFactory::GetTypeId());

// Put the current timestamp and packet size into a log.
void logSize(Ptr<OutputStreamWrapper> stream, Ptr<Packet const> p)
{
    auto current_time = Simulator::Now();
    *stream->GetStream() << current_time.GetSeconds() << ','
                         << p->GetSize() << std::endl;
}

// A timestamp tag that can be added to a packet.
class TimestampTag : public Tag
{
public:
    static TypeId GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::TimestampTag")
                                .SetParent<Tag>()
                                .AddConstructor<TimestampTag>()
                                .AddAttribute("Timestamp",
                                              "Timestamp to save in tag.",
                                              EmptyAttributeValue(),
                                              MakeTimeAccessor(&TimestampTag::timestamp),
                                              MakeTimeChecker());
        return tid;
    };
    TypeId GetInstanceTypeId(void) const { return GetTypeId(); };
    uint32_t GetSerializedSize(void) const { return sizeof(timestamp); };
    void Serialize(TagBuffer i) const
    {
        i.Write(reinterpret_cast<const uint8_t *>(&timestamp),
                sizeof(timestamp));
    };
    void Deserialize(TagBuffer i)
    {
        i.Read(reinterpret_cast<uint8_t *>(&timestamp), sizeof(timestamp));
    };
    void Print(std::ostream &os) const
    {
        os << "t=" << timestamp;
    };

    // these are our accessors to our tag structure
    void SetTime(Time time) { timestamp = time; };
    Time GetTime() { return timestamp; };

private:
    Time timestamp;
};

// Tag a packet with a timestamp.
void setTimeTag(Ptr<Packet const> p)
{
    TimestampTag tag;
    tag.SetTime(Simulator::Now());
    p->AddPacketTag(tag);
};

// If the packet is tagged with a timestamp, write current time and tag
// into the stream. Also add packet size.
void logTimeTag(Ptr<OutputStreamWrapper> stream, Ptr<Packet const> p)
{
    TimestampTag tag;
    if (p->PeekPacketTag(tag))
    {
        auto current_time = Simulator::Now();
        auto diff_time = current_time - tag.GetTime();
        *stream->GetStream() << current_time.GetSeconds() << ','
                             << diff_time.GetSeconds() << ','
                             << p->GetSize() << std::endl;
    }
    else
    {
        //NS_LOG_DEBUG("Packet without timestamp, won't log.");
    };
};

// TODO: Add base stream? Or how to get different random streams?
Ptr<RandomVariableStream> TimeStream(double min = 0.0, double max = 1.0)
{
    return CreateObjectWithAttributes<UniformRandomVariable>(
        "Min", DoubleValue(min),
        "Max", DoubleValue(max));
}

PointerValue TimeStreamValue(double min = 0.0, double max = 1.0)
{
    return PointerValue(TimeStream(min, max));
}

NetDeviceContainer GetNetDevices(Ptr<Node> node)
{
    NetDeviceContainer devices;
    for (uint32_t i = 0; i < node->GetNDevices(); ++i)
    {
        devices.Add(node->GetDevice(i));
    }
    return devices;
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

    int n_apps = 10;
    DataRate linkrate("5Mbps");
    DataRate baserate("100kbps");
    DataRate congestion("0Mbps");
    std::string basedir = "/workspaces/ns3-fitnets/distributions/";
    std::string w1 = basedir + "Facebook_WebServerDist_IntraCluster.txt";
    std::string w2 = basedir + "DCTCP_MsgSizeDist.txt";
    std::string w3 = basedir + "Facebook_HadoopDist_All.txt";
    std::string prefix = "shift";
    double c_w1 = 1;
    double c_w2 = 1;
    double c_w3 = 1;

    CommandLine cmd;
    cmd.AddValue("apps", "Number of traffic apps per workload.", n_apps);
    cmd.AddValue("apprate", "Base traffic rate for each app.", baserate);
    cmd.AddValue("linkrate", "Link capacity rate.", linkrate);
    cmd.AddValue("w1", "Factor for W1 traffic (FB webserver).", c_w1);
    cmd.AddValue("w2", "Factor for W2 traffic (DCTCP messages).", c_w2);
    cmd.AddValue("w3", "Factor for W3 traffic (FB hadoop).", c_w3);
    cmd.AddValue("congestion", "Congestion traffic rate.", congestion);
    cmd.AddValue("prefix", "Prefix for log files.", prefix);
    cmd.Parse(argc, argv);

    // Compute resulting workload datarates.
    auto rate_w1 = DataRate(static_cast<uint64_t>(c_w1 * baserate.GetBitRate()));
    auto rate_w2 = DataRate(static_cast<uint64_t>(c_w2 * baserate.GetBitRate()));
    auto rate_w3 = DataRate(static_cast<uint64_t>(c_w3 * baserate.GetBitRate()));

    /*
    // Print Overview of seetings
    NS_LOG_DEBUG("Overview:"
                 "Apps: "
                 << n_apps << std::endl
                 << "Workloads (data rates per app):"
                 << std::endl
                 << "W1: " << w1 << " (" << rate_w1 << ")"
                 << std::endl
                 << "W2: " << w2 << " (" << rate_w2 << ")"
                 << std::endl
                 << "W3: " << w3 << " (" << rate_w3 << ")");
    */

    // Simulation variables
    auto simStart = TimeValue(Seconds(0));
    auto stopTime = Seconds(30);
    auto simStop = TimeValue(stopTime);

    //
    // Explicitly create the nodes required by the topology (shown above).
    //
    NS_LOG_INFO("Create nodes.");
    NodeContainer hosts;
    hosts.Create(3);
    // Keep references to sender, receiver, and disturbance
    auto sender = hosts.Get(0);
    auto receiver = hosts.Get(1);
    auto disturbance = hosts.Get(2);

    NodeContainer switches;
    switches.Create(2);
    auto switchA = switches.Get(0);
    auto switchB = switches.Get(1);

    NS_LOG_INFO("Build Topology");
    CsmaHelper csma;
    csma.SetChannelAttribute("FullDuplex", BooleanValue(true));
    csma.SetChannelAttribute("DataRate", DataRateValue(linkrate));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(5)));

    // Create the csma links
    csma.Install(NodeContainer(sender, switchA));
    csma.Install(NodeContainer(receiver, switchB));
    csma.Install(NodeContainer(disturbance, switchB));
    csma.Install(NodeContainer(switchA, switchB));

    // Create the bridge netdevice, turning the nodes into actual switches
    BridgeHelper bridge;
    bridge.Install(switchA, GetNetDevices(switchA));
    bridge.Install(switchB, GetNetDevices(switchB));

    // Add internet stack and IP addresses to the hosts
    NS_LOG_INFO("Setup stack and assign IP Addresses.");
    NetDeviceContainer hostDevices;
    hostDevices.Add(GetNetDevices(sender));
    hostDevices.Add(GetNetDevices(receiver));
    hostDevices.Add(GetNetDevices(disturbance));

    InternetStackHelper internet;
    internet.Install(hosts);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    auto addresses = ipv4.Assign(hostDevices);
    // Get Address: Device with index 0/1, address 0 (only one address)
    auto addrReceiver = addresses.GetAddress(1, 0);

    NS_LOG_INFO("Create Traffic Applications.");
    uint16_t base_port = 4200; // Note: We need two ports per pair
    auto trafficStart = TimeStream(1, 2);
    for (auto i_app = 0; i_app < n_apps; ++i_app)
    {
        // Addresses
        auto port = base_port + i_app;
        auto recvAddr = AddressValue(InetSocketAddress(addrReceiver, port));

        // Sink
        Ptr<Application> sink = CreateObjectWithAttributes<PacketSink>(
            "Local", recvAddr, "Protocol", TCP,
            "StartTime", simStart, "StopTime", simStop);
        receiver->AddApplication(sink);

        // Sources for each workload

        if (rate_w1 > 0)
        {
            Ptr<CdfApplication> source1 = CreateObjectWithAttributes<CdfApplication>(
                "Remote", recvAddr, "Protocol", TCP,
                "DataRate", DataRateValue(rate_w1), "CdfFile", StringValue(w3),
                "StartTime", TimeValue(Seconds(trafficStart->GetValue())),
                "StopTime", simStop);
            sender->AddApplication(source1);
        }
        if (rate_w2 > 0)
        {
            Ptr<CdfApplication> source2 = CreateObjectWithAttributes<CdfApplication>(
                "Remote", recvAddr, "Protocol", TCP,
                "DataRate", DataRateValue(rate_w2), "CdfFile", StringValue(w2),
                "StartTime", TimeValue(Seconds(trafficStart->GetValue())),
                "StopTime", simStop);
            sender->AddApplication(source2);
        }
        if (rate_w3 > 0)
        {
            Ptr<CdfApplication> source3 = CreateObjectWithAttributes<CdfApplication>(
                "Remote", recvAddr, "Protocol", TCP,
                "DataRate", DataRateValue(rate_w3), "CdfFile", StringValue(w3),
                "StartTime", TimeValue(Seconds(trafficStart->GetValue())),
                "StopTime", simStop);
            sender->AddApplication(source3);
        }
    }

    NS_LOG_INFO("Configure Congestion App.");
    // Just blast UDP traffic from time to time
    Ptr<Application> congestion_sink = CreateObjectWithAttributes<PacketSink>(
        "Local", AddressValue(InetSocketAddress(addrReceiver, 2100)),
        "Protocol", UDP, "StartTime", simStart, "StopTime", simStop);
    receiver->AddApplication(congestion_sink);

    Ptr<Application> congestion_source = CreateObjectWithAttributes<OnOffApplication>(
        "Remote", AddressValue(InetSocketAddress(addrReceiver, 2100)),
        "Protocol", UDP,
        "OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"),
        "OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"),
        "DataRate", DataRateValue(congestion),
        "StartTime", TimeValue(Seconds(trafficStart->GetValue())),
        "StopTime", simStop);
    disturbance->AddApplication(congestion_source);

    NS_LOG_INFO("Install Tracing");
    AsciiTraceHelper asciiTraceHelper;

    // Packet size.
    std::stringstream sizefilename;
    sizefilename << prefix << "_sizes.csv";
    auto sizefile = asciiTraceHelper.CreateFileStream(sizefilename.str());
    sender->GetDevice(0)->TraceConnectWithoutContext(
        "MacTx", MakeBoundCallback(&logSize, sizefile));

    // Log (one-way) delay from sender to receiver (excludes other sources).
    std::stringstream trackfilename;
    trackfilename << prefix << "_delays.csv";
    auto trackfile = asciiTraceHelper.CreateFileStream(trackfilename.str());
    sender->GetDevice(0)->TraceConnectWithoutContext(
        "MacTx", MakeCallback(&setTimeTag));
    receiver->GetDevice(0)->TraceConnectWithoutContext(
        "MacRx", MakeBoundCallback(&logTimeTag, trackfile));

    //csma.EnablePcapAll("csma-bridge", false);

    //
    // Now, do the actual simulation.
    //
    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(stopTime);
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");
}
