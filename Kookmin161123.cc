#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/ping6-helper.h" // Changed the include statement

using namespace ns3;
static NodeContainer nodes;

uint32_t packetsReceived = 0; // Static variable to keep count

bool ReceivePacket(Ptr<NetDevice> netdevice, Ptr<const Packet> packet, uint16_t protocol, const Address &sourceAddress)
{
    return true;
}

void ReceivePacketWithRss(std::string context, Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu, SignalNoiseDbm signalNoise, uint16_t staId)
{
    packetsReceived++;
    WifiMacHeader hdr;
    packet->PeekHeader(hdr);
    uint32_t index = std::stoi(context.substr(10, 2) );
    NS_LOG_UNCOND("*******************************************************************************************************************");
    NS_LOG_UNCOND("I am Node " << index << " received packet with " << signalNoise.signal << " dbm");
    NS_LOG_UNCOND("*******************************************************************************************************************");

}

void PingResult(Ptr<const Packet> packet, const Address& sourceAddress)
{
    NS_LOG_UNCOND("Received Ping: SourceAddress = " << sourceAddress);
}

int main(int argc, char *argv[])
{
    // Enable Log
    Time::SetResolution(Time::NS);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // Create nodes and set positions

    nodes.Create(5);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0, 0, 0));   // Node 0
    positionAlloc->Add(Vector(15, 0, 0));  // Node 1
    positionAlloc->Add(Vector(15, 15, 0)); // Node 2
    positionAlloc->Add(Vector(30, 0, 0));  // Node 3 (Server)
    positionAlloc->Add(Vector(45, 0, 0));  // Node 4
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // SETUP WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());
    wifiPhy.Set("TxPowerStart", DoubleValue(3.0)); // Set the minimum transmission power to 3 dBm
    wifiPhy.Set("TxPowerEnd", DoubleValue(3.0));   // Set the maximum transmission power to 3 dBm

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac", "QosSupported", BooleanValue(false));
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

    // IP address and routing
    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // static route
    Ipv4StaticRoutingHelper staticRoutingNode1;
    Ptr<Ipv4StaticRouting> staticRouting1 = staticRoutingNode1.GetStaticRouting(nodes.Get(1)->GetObject<Ipv4>());
    staticRouting1->SetDefaultRoute(Ipv4Address("10.1.1.1"), 1); // Node 1 is the default route

    // Add a static route on node 0 to forward packets to node 3 through node 1
    Ipv4StaticRoutingHelper staticRoutingNode0;
    Ptr<Ipv4StaticRouting> staticRouting0 = staticRoutingNode0.GetStaticRouting(nodes.Get(0)->GetObject<Ipv4>());
    Ipv4Address nextHopAddress0("10.1.1.2"); // IP address of node 1
    staticRouting0->AddHostRouteTo(Ipv4Address("10.1.1.3"), nextHopAddress0, 1);

    // Add a static route on node 2 to forward packets to node 3 through node 1
    Ipv4StaticRoutingHelper staticRoutingNode2;
    Ptr<Ipv4StaticRouting> staticRouting2 = staticRoutingNode2.GetStaticRouting(nodes.Get(2)->GetObject<Ipv4>());
    Ipv4Address nextHopAddress2("10.1.1.2"); // IP address of node 1
    staticRouting2->AddHostRouteTo(Ipv4Address("10.1.1.3"), nextHopAddress2, 1);

    // Add a static route on node 4 to forward packets to node 3
    Ipv4StaticRoutingHelper staticRoutingNode4;
    Ptr<Ipv4StaticRouting> staticRouting4 = staticRoutingNode4.GetStaticRouting(nodes.Get(4)->GetObject<Ipv4>());
    staticRouting4->AddHostRouteTo(Ipv4Address("10.1.1.3"), 1);

    // Ping test from node 3 to node 4
    Ipv6Address dstAddr4("2001:db8::4");  // Adjust the IPv6 address based on your network topology
    Ping6Helper ping4;
    ping4.SetRemote(dstAddr4);
    ping4.SetAttribute("MaxPackets", UintegerValue(10));
    ping4.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    ApplicationContainer apps4 = ping4.Install(nodes.Get(3));
    apps4.Start(Seconds(1.0));
    apps4.Stop(Seconds(10.0));

    // Ping test from node 3 to node 1
    Ipv6Address dstAddr1("2001:db8::1");  // Adjust the IPv6 address based on your network topology
    Ping6Helper ping1;
    ping1.SetRemote(dstAddr1);
    ping1.SetAttribute("MaxPackets", UintegerValue(10));
    ping1.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    ApplicationContainer apps1 = ping1.Install(nodes.Get(3));
    apps1.Start(Seconds(2.0));
    apps1.Stop(Seconds(10.0));

    // Ping test from node 3 to node 1 to node 0
    Ipv6Address dstAddr0("2001:db8::0");  // Adjust the IPv6 address based on your network topology
    Ping6Helper ping0;
    ping0.SetRemote(dstAddr0);
    ping0.SetAttribute("MaxPackets", UintegerValue(10));
    ping0.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    ApplicationContainer apps0 = ping0.Install(nodes.Get(3));
    apps0.Start(Seconds(3.0));
    apps0.Stop(Seconds(10.0));

    // Ping test from node 3 to node 1 to node 2
    Ipv6Address dstAddr2("2001:db8::2");  // Adjust the IPv6 address based on your network topology
    Ping6Helper ping2;
    ping2.SetRemote(dstAddr2);
    ping2.SetAttribute("MaxPackets", UintegerValue(10));
    ping2.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    ApplicationContainer apps2 = ping2.Install(nodes.Get(3));
    apps2.Start(Seconds(4.0));
    apps2.Stop(Seconds(10.0));

    // Data sizes
    uint32_t dataSize0 = 6 * 1024 * 1024; // 6Mbyte
    uint32_t dataSize1 = 2 * 1024 * 1024; // 2Mbyte
    uint32_t dataSize2 = 7 * 1024 * 1024; // 7Mbyte
    uint32_t dataSize3 = 1 * 1024 * 1024; // 1Mbyte

    InetSocketAddress dstAddrD(interfaces.GetAddress(3), 9);
    ApplicationContainer clientApps;

    // Create a PacketSink application on node 3 to receive packets
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
    ApplicationContainer packetSinkApps = packetSinkHelper.Install(nodes.Get(3));
    packetSinkApps.Start(Seconds(0.0));
    packetSinkApps.Stop(Seconds(10.0));

    devices.Get(3)->SetReceiveCallback(MakeCallback(&ReceivePacket));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/MonitorSnifferRx", MakeCallback(&ReceivePacketWithRss));

    // from node 0 to 3/D
    OnOffHelper client0to1("ns3::TcpSocketFactory", dstAddrD);
    client0to1.SetAttribute("PacketSize", UintegerValue(dataSize0));
    client0to1.SetAttribute("DataRate", StringValue("10Mbps"));
    ApplicationContainer clientApps0to1 = client0to1.Install(nodes.Get(0));
    clientApps0to1.Start(Seconds(1.0));
    clientApps0to1.Stop(Seconds(1.0 + dataSize0 * 6 / 10 / 10000000.0)); //

    // node 1 forward to 3/D
    OnOffHelper client1to3Forward("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(3), 9));
    client1to3Forward.SetAttribute("PacketSize", UintegerValue(dataSize0));
    client1to3Forward.SetAttribute("DataRate", StringValue("10Mbps"));
    ApplicationContainer clientApps1to3Forward = client1to3Forward.Install(nodes.Get(1));
    clientApps1to3Forward.Start(Seconds(1.0 + dataSize0 * 6 / 10 / 10000000.0));
    clientApps1to3Forward.Stop(Seconds(2.0 + dataSize0 * 6 / 10 / 10000000.0));

    // from node 2 to 3/D
    OnOffHelper client2to1("ns3::TcpSocketFactory", dstAddrD);
    client2to1.SetAttribute("PacketSize", UintegerValue(dataSize1));
    client2to1.SetAttribute("DataRate", StringValue("10Mbps"));
    ApplicationContainer clientApps2to1 = client2to1.Install(nodes.Get(2));
    clientApps2to1.Start(Seconds(3.0));
    clientApps2to1.Stop(Seconds(3.0 + dataSize1 * 2 / 10 / 10000000.0)); //

    // node 1 forward to 3/D
    OnOffHelper client1to3Backward("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(3), 9));
    client1to3Backward.SetAttribute("PacketSize", UintegerValue(dataSize1));
    client1to3Backward.SetAttribute("DataRate", StringValue("10Mbps"));
    ApplicationContainer clientApps1to3Backward = client1to3Backward.Install(nodes.Get(1));
    clientApps1to3Backward.Start(Seconds(3.0 + dataSize1 * 2 / 10 / 10000000.0));
    clientApps1to3Backward.Stop(Seconds(4.0 + dataSize1 * 2 / 10 / 10000000.0));

    // from node 1 to 3/D
    OnOffHelper client1to3("ns3::TcpSocketFactory", dstAddrD);
    client1to3.SetAttribute("PacketSize", UintegerValue(dataSize2));
    client1to3.SetAttribute("DataRate", StringValue("10Mbps"));
    ApplicationContainer clientApps1to3 = client1to3.Install(nodes.Get(1));
    clientApps1to3.Start(Seconds(5.0));
    clientApps1to3.Stop(Seconds(6.0 + dataSize2 * 2 / 10 / 10000000.0)); //

    // from node 4 to 3/D
    OnOffHelper client4to3("ns3::TcpSocketFactory", dstAddrD);
    client4to3.SetAttribute("PacketSize", UintegerValue(dataSize3));
    client4to3.SetAttribute("DataRate", StringValue("10Mbps"));
    ApplicationContainer clientApps4to3 = client4to3.Install(nodes.Get(4));
    clientApps4to3.Start(Seconds(7.0));
    clientApps4to3.Stop(Seconds(8.0 + dataSize3 * 1 / 10 / 10000000.0)); //

    // pcap tracing
    wifiPhy.EnablePcap("wifi-trace", devices);

    // NetAnim setup
    AnimationInterface anim("Kookmin161123.xml");

    // Run the simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    // 결과 출력
    NS_LOG_UNCOND("Packets received by Node 3 (sink node): " << packetsReceived);

    // Print ping test results
    Ptr<PacketSink> sink3 = DynamicCast<PacketSink>(packetSinkApps.Get(0));
    NS_LOG_UNCOND("Ping test results from Node 3 to Node 4:");
    sink3->TraceConnectWithoutContext("Rx", MakeCallback(&PingResult));

    return 0;
}
