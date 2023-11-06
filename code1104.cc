#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ping-helper.h"



using namespace ns3;

int main(int argc, char *argv[])
{
    Time::SetResolution(Time::NS);

    // Create nodes and set positions
    NodeContainer nodes;
    nodes.Create(5);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0, 0, 0)); // node 0
    positionAlloc->Add(Vector(4, 0, 0)); // node 1
    positionAlloc->Add(Vector(4, 2, 0)); // node 2
    positionAlloc->Add(Vector(8, 0, 0)); // node 3
    positionAlloc->Add(Vector(12, 0, 0)); // node 4
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Set up WiFi
    WifiHelper wifi;
    wifi.SetStandard(ns3::WIFI_STANDARD_80211n);
    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

    // IP address and routing
    InternetStackHelper internet;
    internet.Install(nodes);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);
    Ipv4StaticRoutingHelper staticRouting;

    //me
    Ptr<Ipv4StaticRouting> staticRoutingNode0 = staticRouting.GetStaticRouting(nodes.Get(0)->GetObject<Ipv4>());
    Ptr<Ipv4StaticRouting> staticRoutingNode1 = staticRouting.GetStaticRouting(nodes.Get(1)->GetObject<Ipv4>());


     uint32_t dataSizeA = 6 * 1024 * 1024; // 6Mbyte
     //uint32_t dataSizeB = 2 * 1024 * 1024; // 2Mbyte
     //uint32_t dataSizeC = 7 * 1024 * 1024; // 7Mbyte
     //uint32_t dataSizeE = 1 * 1024 * 1024; // 1Mbyte
     InetSocketAddress dstAddrD(interfaces.GetAddress(3), 9);

    // Node 0 send to 1
    OnOffHelper client0to1("ns3::TcpSocketFactory", dstAddrD);
    client0to1.SetAttribute("PacketSize", UintegerValue(dataSizeA)); // 6Mbyte
    client0to1.SetAttribute("DataRate", StringValue("10Mbps"));
    ApplicationContainer clientApps0to1 = client0to1.Install(nodes.Get(0));
    clientApps0to1.Start(Seconds(1.0));
    clientApps0to1.Stop(Seconds(1.0 + dataSizeA * 8 / 10 / 10000000.0));

    // node 1 forward to 3/D
    OnOffHelper client1to3("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(3), 9));
    client1to3.SetAttribute("PacketSize", UintegerValue(dataSizeA));
    client1to3.SetAttribute("DataRate", StringValue("10Mbps"));
    ApplicationContainer clientApps1to3 = client1to3.Install(nodes.Get(1));
    clientApps1to3.Start(Seconds(1.0 + dataSizeA * 8 / 10 / 10000000.0));
    clientApps1to3.Stop(Seconds(2.0 + dataSizeA * 8 / 10 / 10000000.0));


    // Node 2 send
    /*
    OnOffHelper clientCtoD("ns3::TcpSocketFactory", dstAddrD);
    clientCtoD.SetAttribute("PacketSize", UintegerValue(dataSizeC)); // 7Mbyte
    clientCtoD.SetAttribute("DataRate", StringValue("10Mbps"));
    ApplicationContainer clientAppsCtoD = clientCtoD.Install(nodes.Get(2));
    clientAppsCtoD.Start(Seconds(1.5));
    clientAppsCtoD.Stop(Seconds(2.0));
	*/

    // Node 1 send
    /*
    OnOffHelper clientBtoD("ns3::TcpSocketFactory", dstAddrD);
    clientBtoD.SetAttribute("PacketSize", UintegerValue(dataSizeB)); // 2Mbyte
    clientBtoD.SetAttribute("DataRate", StringValue("10Mbps"));
    ApplicationContainer clientAppsBtoD = clientBtoD.Install(nodes.Get(1));
    clientAppsBtoD.Start(Seconds(2.0));
    clientAppsBtoD.Stop(Seconds(2.5));
	*/

    // Node 4 send
    /*
    OnOffHelper clientEtoD("ns3::TcpSocketFactory", dstAddrD);
    clientEtoD.SetAttribute("PacketSize", UintegerValue(dataSizeE)); // 1Mbyte
    clientEtoD.SetAttribute("DataRate", StringValue("10Mbps"));
    ApplicationContainer clientAppsEtoD = clientEtoD.Install(nodes.Get(4));
    clientAppsEtoD.Start(Seconds(2.5));
    clientAppsEtoD.Stop(Seconds(3.0));
	*/

    // Add V4Ping to test connectivity from Node A to Node D
    PingHelper ping = PingHelper(interfaces.GetAddress(3));// Assuming Node D's interface is indexed at 3
    //ping.SetAttribute("Verbose", BooleanValue(true));
    ApplicationContainer pingApps = ping.Install(nodes.Get(0)); // Node A pings Node D
    pingApps.Start(Seconds(0.5)); // You can adjust the start time as needed
    pingApps.Stop(Seconds(4.0));  // And the stop time too, ensuring it's within your simulation time


    // pcap tracing
    wifiPhy.EnablePcap("wifi-trace", devices);

    // NetAnim setup
    AnimationInterface anim("anim.xml");

    // Run the simulation
    Simulator::Stop(Seconds(3.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;

}
