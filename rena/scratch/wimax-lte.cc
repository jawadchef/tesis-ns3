/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright (c) 2007,2008, 2009 INRIA, UDcast
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 */

//
// Default network topology includes a base station (BS) and 2
// subscriber station (SS).

//      +-----+
//      | SS0 |
//      +-----+
//     10.1.1.1
//      -------
//        ((*))
//
//                  10.1.1.7
//               +------------+
//               |Base Station| ==((*))
//               +------------+
//
//        ((*))
//       -------
//      10.1.1.2
//       +-----+
//       | SS1 |
//       +-----+

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wimax-module.h"
#include "ns3/internet-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/ipcs-classifier-record.h"
#include "ns3/service-flow.h"
#include "ns3/lte-module.h"
#include <iostream>

NS_LOG_COMPONENT_DEFINE ("WimaxSimpleExample");

using namespace ns3;

int main (int argc, char *argv[])
{
Config::SetDefault ("ns3::LteAmc::AmcModel", EnumValue (LteAmc::PiroEW2010));

  bool verbose = false;

  uint16_t numberOfUEs=2; 		//Default number of ues attached to each eNodeB  	

  Ptr<LteHelper> lteHelper;	//Define LTE	
  Ptr<EpcHelper>  epcHelper;	//Define EPC

  NodeContainer remoteHostContainer;		//Define the Remote Host
  NetDeviceContainer internetDevices; 	//Define the Network Devices in the Connection between EPC and the remote host
	  
  Ptr<Node> pgw;				//Define the Packet Data Network Gateway(P-GW)	
  Ptr<Node> remoteHost;		//Define the node of remote Host
	
  InternetStackHelper internet;			//Define the internet stack	
  PointToPointHelper p2ph;				//Define Connection between EPC and the Remote Host
  Ipv4AddressHelper ipv4h;				//Ipv4 address helper
  Ipv4StaticRoutingHelper ipv4RoutingHelper;	//Ipv4 static routing helper	
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting;

  Ipv4InterfaceContainer internetIpIfaces;	//Ipv4 interfaces


  int duration = 200, schedType = 0;
  WimaxHelper::SchedulerType scheduler = WimaxHelper::SCHED_TYPE_SIMPLE;

  CommandLine cmd;
  cmd.AddValue ("scheduler", "type of scheduler to use with the network devices", schedType);
  cmd.AddValue ("duration", "duration of the simulation in seconds", duration);
  cmd.AddValue ("verbose", "turn on all WimaxNetDevice log components", verbose);
  cmd.Parse (argc, argv);
  //LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  //LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  switch (schedType)
    {
    case 0:
      scheduler = WimaxHelper::SCHED_TYPE_SIMPLE;
      break;
    case 1:
      scheduler = WimaxHelper::SCHED_TYPE_MBQOS;
      break;
    case 2:
      scheduler = WimaxHelper::SCHED_TYPE_RTPS;
      break;
    default:
      scheduler = WimaxHelper::SCHED_TYPE_SIMPLE;
    }

  NodeContainer ssNodes;
  NodeContainer bsNodes;
  NodeContainer bsNodes1;

  ssNodes.Create (2);
  bsNodes.Create (1);
  bsNodes1.Create (1);


   
  NetDeviceContainer ssDevs1, bsDevs1;

  WimaxHelper wimax;

  NetDeviceContainer ssDevs, bsDevs;

  ssDevs = wimax.Install (ssNodes,
                          WimaxHelper::DEVICE_TYPE_SUBSCRIBER_STATION,
                          WimaxHelper::SIMPLE_PHY_TYPE_OFDM,
                          scheduler);
  bsDevs = wimax.Install (bsNodes, WimaxHelper::DEVICE_TYPE_BASE_STATION, WimaxHelper::SIMPLE_PHY_TYPE_OFDM, scheduler);

  
  Ptr<SubscriberStationNetDevice> ss[2];

  for (int i = 0; i < 2; i++)
    {
      ss[i] = ssDevs.Get (i)->GetObject<SubscriberStationNetDevice> ();
      ss[i]->SetModulationType (WimaxPhy::MODULATION_TYPE_QAM16_12);
    }

  Ptr<BaseStationNetDevice> bs;

  bs = bsDevs.Get (0)->GetObject<BaseStationNetDevice> ();


  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc;
  positionAlloc = CreateObject<ListPositionAllocator> ();
  
  positionAlloc->Add (Vector (-0.0, 70.0, 0.0)); //STA
  
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");  
  mobility.Install(ssNodes.Get(0));
  
  Ptr<ConstantVelocityMobilityModel> cvm = ssNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>();
  cvm->SetVelocity(Vector (10, 0, 0)); //move to left to right 10.0m/s

  positionAlloc = CreateObject<ListPositionAllocator> ();
  
  positionAlloc->Add (Vector (-0.0, 40.0, 0.0)); //MAG1AP
  positionAlloc->Add (Vector (0.0, 40.0, 0.0));  //MAG2AP
  positionAlloc->Add (Vector (0.0, 40.0, 0.0));  //MAG2AP
  
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  
  mobility.Install (NodeContainer(bsNodes.Get(0),ssNodes.Get(1),bsNodes1.Get(0)));

  InternetStackHelper stack;
  stack.Install (bsNodes);
  stack.Install (ssNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer SSinterfaces = address.Assign (ssDevs);
  Ipv4InterfaceContainer BSinterface = address.Assign (bsDevs);

  if (verbose)
    {
      wimax.EnableLogComponents ();  // Turn on all wimax logging
    }
  /*------------------------------*/
  UdpServerHelper udpServer;
  ApplicationContainer serverApps;
  UdpClientHelper udpClient;
  ApplicationContainer clientApps;

  udpServer = UdpServerHelper (100);

  serverApps = udpServer.Install (ssNodes.Get (0));
  serverApps.Start (Seconds (6));
  serverApps.Stop (Seconds (duration));

  udpClient = UdpClientHelper (SSinterfaces.GetAddress (0), 100);
  udpClient.SetAttribute ("MaxPackets", UintegerValue (1200));
  udpClient.SetAttribute ("Interval", TimeValue (Seconds (0.5)));
  udpClient.SetAttribute ("PacketSize", UintegerValue (1024));

  clientApps = udpClient.Install (ssNodes.Get (1));
  clientApps.Start (Seconds (6));
  clientApps.Stop (Seconds (duration));

  Simulator::Stop (Seconds (duration + 0.1));

  IpcsClassifierRecord DlClassifierUgs (Ipv4Address ("0.0.0.0"),
                                        Ipv4Mask ("0.0.0.0"),
                                        SSinterfaces.GetAddress (0),
                                        Ipv4Mask ("255.255.255.255"),
                                        0,
                                        65000,
                                        100,
                                        100,
                                        17,
                                        1);
  ServiceFlow DlServiceFlowUgs = wimax.CreateServiceFlow (ServiceFlow::SF_DIRECTION_DOWN,
                                                          ServiceFlow::SF_TYPE_RTPS,
                                                          DlClassifierUgs);

  IpcsClassifierRecord UlClassifierUgs (SSinterfaces.GetAddress (1),
                                        Ipv4Mask ("255.255.255.255"),
                                        Ipv4Address ("0.0.0.0"),
                                        Ipv4Mask ("0.0.0.0"),
                                        0,
                                        65000,
                                        100,
                                        100,
                                        17,
                                        1);
  ServiceFlow UlServiceFlowUgs = wimax.CreateServiceFlow (ServiceFlow::SF_DIRECTION_UP,
                                                          ServiceFlow::SF_TYPE_RTPS,
                                                          UlClassifierUgs);
  ss[0]->AddServiceFlow (DlServiceFlowUgs);
  ss[1]->AddServiceFlow (UlServiceFlowUgs);

  


  lteHelper = CreateObject<LteHelper> ();
  epcHelper = CreateObject<EpcHelper> ();

  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
  lteHelper->SetAttribute ("PathlossModel",
                               StringValue ("ns3::FriisPropagationLossModel"));
 
  pgw = epcHelper->GetPgwNode ();

  remoteHostContainer.Create (1);
  remoteHost = remoteHostContainer.Get (0);
  internet.Install (remoteHostContainer);
	
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010))); 
  internetDevices = p2ph.Install (pgw, remoteHost);

  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  internetIpIfaces = ipv4h.Assign (internetDevices);
  
  remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
	  
  bsDevs1 = lteHelper->InstallEnbDevice (bsNodes1);
  
  ssDevs1=lteHelper->InstallUeDevice (ssNodes);

  for (uint16_t j=0; j < numberOfUEs; j++)
  {
        lteHelper->Attach (ssDevs1.Get(j), bsDevs1.Get(0));  
  }     
	    
  Ipv4InterfaceContainer iueIpIface;
  iueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ssDevs1));

  lteHelper->ActivateEpsBearer (ssDevs1, EpsBearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT), EpcTft::Default ());
	
  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps1 = echoServer.Install (ssNodes.Get (0));
  serverApps1.Start (Seconds (1.0));
  serverApps1.Stop (Seconds (duration));

  UdpEchoClientHelper echoClient (iueIpIface.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (200000));
  //echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.004)));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps1 = echoClient.Install (pgw);
  clientApps1.Start (Seconds (2.0));
  clientApps1.Stop (Seconds (duration));


	 		  
  lteHelper->EnableTraces ();

  NS_LOG_INFO ("Starting simulation.....");
  Simulator::Run ();

  ss[0] = 0;
  ss[1] = 0;
  bs = 0;

  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  return 0;
}
