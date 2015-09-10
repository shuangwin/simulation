/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
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
 *
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
//#include "ns3/gtk-config-store.h"


using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");

int
main (int argc, char *argv[])
{

  //LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable ("Request", LOG_LEVEL_ALL);
  LogComponentEnable ("ContentServer", LOG_LEVEL_ALL);
  //LogComponentEnable ("Socket", LOG_LEVEL_ALL);
  
  //uint16_t numberOfNodes = 2;
  uint16_t numberOfues = 1;
  double simTime = 400;  // s
  double distance = 60.0; // m
  double interPacketInterval = 100;  // ms

  // Command line arguments
  CommandLine cmd;
  //cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> (); //p2pEpc，功能？
  lteHelper->SetEpcHelper (epcHelper);

  ConfigStore inputConfig;  //ConfigStroe? 
  inputConfig.ConfigureDefaults(); //作用？ 输入默认配置

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode (); //看下PGW从初始配置相关的代码？

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  //其他节点interface0也是localhost吗？
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(1);
  ueNodes.Create(numberOfues);

  // Install Mobility Model
  
  
  MobilityHelper mobility;
  //eNB固定位置
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.Install(enbNodes);
  
  
  //UE设置为随机游走模式
  //Mobility::SetPositionAllocator(Ptr<PositionAllocator> allocator)
  mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                "X",StringValue("100.0"),
                                "Y",StringValue("100.0"),
                                "Rho",StringValue("ns3::UniformRandomVariable[Min=0|Max=30]"));
                                
  mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                            "Mode", StringValue("Time"),
                            "Time", StringValue("2s"),
                            "Speed", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
                            "Bounds", StringValue("0|200|0|200"));
  
  mobility.Install(ueNodes);
  
  
  
  //打印各个节点的位置坐标
  for (uint16_t i= 0; i<1; i++)
    {
        Ptr<Node> node = enbNodes.Get(i);
        Ptr<MobilityModel> position = node->GetObject<MobilityModel>();
        Vector pos = position->GetPosition();
        std::cout<< "The "<<i<<" eNB's position is "<<pos.x<<" "<<pos.y<<" "<<pos.z<<std::endl;
    }
    
    
    for (uint16_t i= 0; i<numberOfues; i++)
    {
        Ptr<Node> node = ueNodes.Get(i);
        Ptr<MobilityModel> position = node->GetObject<MobilityModel>();
        Vector pos = position->GetPosition();
        std::cout<< "The "<<i<<" ue's position is "<<pos.x<<" "<<pos.y<<" "<<pos.z<<std::endl;
    }
  //打印各个节点的位置坐标
  
  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numberOfues; i++)
      {
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(0));
        // side effect: the default EPS bearer will be activated
      }
  //default bearer=?

  // Install and start applications on UEs and remote host
  //uint16_t dlPort = 1234; // port on UE to receive the content from the server
  uint16_t ulPort = 2000;

  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  
  /* 修改部分 */
  RequestHelper req (remoteHostAddr, ulPort);
  //PacketSinkHelper contentrcv ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
  
  ContentServerHelper server("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), ulPort));
                         
  serverApps.Add(server.Install(remoteHost)); //给服务器安装packetsink                       
  /* 修改部分 */
  
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      clientApps.Add (req.Install (ueNodes.Get(u)));
      //clientApps.Add (contentrcv.Install (ueNodes.Get(u)));
      
      //++ulPort; //来自两个eNB的上行链路的数据，在服务器上用两个端口接收，一个eNB的不同用户数据，服务器端用一个端口就行？

      /***
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      //1)The type id of the protocol to use for the rx socket
      //2)The Address on which to Bind the rx socket
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      
      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
      ***/
      
      
      /***
      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      //1)RemoteAddress(destination address),2)RemotePort(destination port)
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

      
      clientApps.Add (dlClient.Install (remoteHost));
      clientApps.Add (ulClient.Install (ueNodes.Get(u)));
      ***/
      
      
    }
  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.02));
  lteHelper->EnableTraces ();
  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lena-epc-first");

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy();
  return 0;

}

