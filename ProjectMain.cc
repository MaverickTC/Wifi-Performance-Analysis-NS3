
  #include "ns3/command-line.h"
  #include "ns3/config.h"
  #include "ns3/string.h"
  #include "ns3/log.h"
  #include "ns3/yans-wifi-helper.h"
  #include "ns3/ssid.h"
  #include "ns3/mobility-helper.h"
  #include "ns3/on-off-helper.h"
  #include "ns3/yans-wifi-channel.h"
  #include "ns3/mobility-model.h"
  #include "ns3/packet-sink.h"
  #include "ns3/packet-sink-helper.h"
  #include "ns3/tcp-westwood.h"
  #include "ns3/internet-stack-helper.h"
  #include "ns3/ipv4-address-helper.h"
  #include "ns3/ipv4-global-routing-helper.h"
  #include<string>
   
  NS_LOG_COMPONENT_DEFINE ("wifi-tcp");
   
  using namespace ns3;

  uint64_t lastTotalRx = 0;
  Ptr<PacketSink> sink;             
   
  void
  CalculateThroughput ()
  {
    Time now = Simulator::Now ();                                        
    double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5;    
    std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
    lastTotalRx = sink->GetTotalRx ();
    Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
  }

  int main (int argc, char *argv[])
    {
      uint32_t payloadSize = 1024;
      double dataRateVal = 86.7;                          /* Transport layer payload size in bytes. */
      std::string dataRate = "86.7Mbps";                  /* Application layer datarate. */
      bool macMechanism = false;
      uint32_t numSTA = 1;
      uint32_t totalLoadPercent=100;              
      double simulationTime = 10;                        /* Simulation time in seconds. */

    /* Command line argument parser setup. */
      CommandLine cmd (__FILE__);
      cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
      cmd.AddValue ("macMechanism", "Mac mechanism", macMechanism);
      cmd.AddValue ("numSTA", "Number of wifi STA devices", numSTA);
      cmd.AddValue ("totalLoadPercent", "The total load percentage", totalLoadPercent);
      cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
      cmd.Parse (argc, argv);
    
    dataRate = std::to_string(dataRateVal * (totalLoadPercent*0.01)/numSTA) + "Mbps";
    /* Configure TCP Options */
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
    
    UintegerValue ctsThr = (macMechanism ? UintegerValue (100) : UintegerValue (999999));
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThr);

    WifiMacHelper wifiMac;
    WifiHelper wifiHelper;
    wifiHelper.SetStandard (WIFI_STANDARD_80211ac);
    wifiHelper.SetRemoteStationManager ("ns3::MinstrelHtWifiManager");
 
    /* Set up Legacy Channel */
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (24e10));
  
    /* Setup Physical Layer */
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel (wifiChannel.Create ());

    wifiPhy.Set ("ChannelWidth", UintegerValue (20));
   

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (numSTA);
    
    NodeContainer wifiApNode;
    wifiApNode.Create (1);
    
      /* Configure AP */
      Ssid ssid = Ssid ("network");
      wifiMac.SetType ("ns3::ApWifiMac",
                          "Ssid", SsidValue (ssid));
      
       NetDeviceContainer apDevice;
       apDevice = wifiHelper.Install (wifiPhy, wifiMac, wifiApNode);
       
       /* Configure STA */
       wifiMac.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid));
    
    NetDeviceContainer staDevices;
    staDevices = wifiHelper.Install (wifiPhy, wifiMac, wifiStaNodes);
   
  /* Mobility model */
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 0.0));

    MobilityHelper mobilitySta;
    Ptr<ListPositionAllocator> positionAllocSta = CreateObject<ListPositionAllocator> ();
    positionAllocSta->Add (Vector (0.01, 0.01, 0.01));

    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    mobilitySta.SetPositionAllocator (positionAllocSta);
    mobilitySta.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    mobility.Install (wifiApNode);
    mobilitySta.Install (wifiStaNodes);
   
  /* Internet stack */
    InternetStackHelper stack;
    stack.Install (wifiStaNodes);
    stack.Install (wifiApNode);
   
    Ipv4AddressHelper address;
    address.SetBase ("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign (apDevice);
    Ipv4InterfaceContainer staInterface;
    staInterface = address.Assign (staDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
    ApplicationContainer sinkApp = sinkHelper.Install (wifiApNode);
    sink = StaticCast<PacketSink> (sinkApp.Get (0));
 
    OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (apInterface.GetAddress (0), 9)));
    server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
    server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
    ApplicationContainer serverApp = server.Install (wifiStaNodes);

    sinkApp.Start (Seconds (0.0));
    serverApp.Start (Seconds (1.0));
    Simulator::Schedule (Seconds (1.1), &CalculateThroughput);

    Simulator::Stop (Seconds (simulationTime + 1));
    Simulator::Run ();

    double averageThroughput = ((sink->GetTotalRx () * 8) / (1e6 * simulationTime));

    Simulator::Destroy ();
  
  std::cout << "\nAverage throughput: " << averageThroughput << " Mbit/s" << std::endl;
  return 0;
}