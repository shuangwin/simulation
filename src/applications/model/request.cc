/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "request.h"
#include "seq-ts-header.h"
#include <cstdlib>
#include <cstdio>

#include "ns3/ipv4.h"
#include <ns3/random-variable-stream.h>  //add 9.14
#include <ns3/double.h>



/*Modify udp-client.cc to be the app on UE to send udp/tcp content request randomly during the simulation time(12/24 hours)
*8.31
*同时添加其他三个文件：request.h, request-helper.cc, request-helper.h
*并添加到wscript文件的对应列表中
*目前全部是tcp请求,开始建立一个Socket，然后以恒定间隔发送有限个服务请求
*9.2
*服务器端已经完善
*客户端：之前是req和packetsink分别创建socket来发送请求和接收数据，接下来合并成一个
*/
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Request");

NS_OBJECT_ENSURE_REGISTERED (Request);

TypeId
Request::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Request")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<Request> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (1),
                   MakeUintegerAccessor (&Request::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&Request::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&Request::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&Request::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&Request::m_size),
                   MakeUintegerChecker<uint32_t> (12,1500))
  ;
  return tid;
}

Request::Request ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  //m_socket = 0;
  //m_sendEvent = EventId ();
  m_socCreateEvent = EventId (); 
}

Request::~Request ()
{
  NS_LOG_FUNCTION (this);
}

void
Request::SetRemote (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address(ip);
  m_peerPort = port;
}

void
Request::SetRemote (Ipv6Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address(ip);
  m_peerPort = port;
}

void
Request::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
Request::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
Request::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  std::cout<<"The UE starts at "<<Simulator::Now ().GetSeconds ()<<std::endl;
  double mean = 3.14;
  double bound = 10;
  
  Ptr<ExponentialRandomVariable> x = CreateObject<ExponentialRandomVariable> ();
  x->SetAttribute ("Mean", DoubleValue (mean));
  x->SetAttribute ("Bound", DoubleValue (bound));
  double value = x->GetValue ();
  
  m_socCreateEvent = Simulator::Schedule (Seconds (value), &Request::randomSocketCreat, this); //todo: 开始时间随机 

}


void
Request::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_socCreateEvent);
}


void Request::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  std::cout<<"The tcp client connect successful at "<<(Simulator::Now ()).GetSeconds ()<<std::endl;
  Send(socket);
  
}


void Request::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("Request, Connection Failed");
}


void
Request::Send (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);
  
  uint8_t buf[2];
  buf[0]='1';
  buf[1]='2';
  Ptr<Packet> p = Create<Packet> (buf,2);
  //Ptr<Packet> p = Create<Packet> (m_size-(8+4)); // 8+4 : the size of the seqTs header
  p->AddHeader (seqTs);

  std::stringstream peerAddressStringStream;
  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv6Address::ConvertFrom (m_peerAddress);
    }

  if ((socket->Send (p)) >= 0)
    {
      ++m_sent;
      NS_LOG_INFO ("TraceDelay TX " << m_size << " bytes to "
                                    << peerAddressStringStream.str () << " Uid: "
                                    << p->GetUid () << " Time: "
                                    << (Simulator::Now ()).GetSeconds ());

    }
  else
    {
      NS_LOG_INFO ("Error while sending " << m_size << " bytes to "
                                          << peerAddressStringStream.str ());
    }
    
  
} //Send

void Request::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Ptr<Ipv4> p = Request::GetNode()->GetObject<Ipv4> (); //如何打印当前收包的UE的IP? 不知对不对
  //"<< p->GetAddress(1,0).GetLocal () <<"
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      m_totalRx += packet->GetSize ();
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s The UE "<< p->GetAddress(1,0).GetLocal () <<" received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      
    }
} //HandleRead


void Request::HandlePeerClose (Ptr<Socket> socket)
{
  //Close the socket and erase the socket in the m_socketList
  NS_LOG_FUNCTION (this << socket);
  socket->Close();
  std::list<Ptr<Socket> >::iterator iter;
  for (iter = m_socketList.begin(); iter != m_socketList.end(); ++iter)
  {
      if(*iter == socket)
      {
          std::cout<<"Request---There's a socket in m_socketList, going to delete it"<<std::endl;
          break;
      }        
  }
  
  if(iter == m_socketList.end() && *iter!= socket)
  {
       std::cout<<"Request---There is no inter->socket == socket in m_socketList!"<<std::endl;
       return;  //return语句！ 强制结束当前函数
  }
  
  m_socketList.erase(iter);
 
}

void Request::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}


void Request::randomSocketCreat()
{
  NS_LOG_FUNCTION (this<<Simulator::Now ().GetSeconds ());
  
  NS_ASSERT (m_socCreateEvent.IsExpired ());

  //Create a socket and send a request 
  Ptr<Socket> socketToUse = 0;
  TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
  socketToUse = Socket::CreateSocket (GetNode (), tid); 
  if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
  {
    socketToUse->Bind ();
    socketToUse->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
  }
  
  
  socketToUse->SetRecvCallback (MakeCallback (&Request::HandleRead, this));
  socketToUse->SetConnectCallback (
        MakeCallback (&Request::ConnectionSucceeded, this),
        MakeCallback (&Request::ConnectionFailed, this)); 
        
  socketToUse->SetCloseCallbacks (
    MakeCallback (&Request::HandlePeerClose, this),
    MakeCallback (&Request::HandlePeerError, this)); 
    
  m_socketList.push_back(socketToUse); 
  
  
  /*
  double mean = 30;
  double bound = 50;
  
  Ptr<ExponentialRandomVariable> x = CreateObject<ExponentialRandomVariable> ();
  x->SetAttribute ("Mean", DoubleValue (mean));
  x->SetAttribute ("Bound", DoubleValue (bound));
  double value = x->GetValue ();
  //m_socCreateEvent = Simulator::Schedule (Seconds (value), &Request::randomSocketCreat, this); //wait and send another request
  */
}

} // Namespace ns3
