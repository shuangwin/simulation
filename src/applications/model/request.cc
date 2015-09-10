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
  m_socket = 0;
  m_sendEvent = EventId ();
  
  //add 
  m_connected = false;
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
  
  
  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid); //可在随机时间点创建socket
      if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket->Bind ();
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket->Bind6 ();
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
    }

  //m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  m_socket->SetRecvCallback (MakeCallback (&Request::HandleRead, this));
  m_socket->SetConnectCallback (
        MakeCallback (&Request::ConnectionSucceeded, this),
        MakeCallback (&Request::ConnectionFailed, this)); 
        
  m_socket->SetCloseCallbacks (
    MakeCallback (&Request::HandlePeerClose, this),
    MakeCallback (&Request::HandlePeerError, this));
  
  if (m_connected)
    { // Only send new data if the connection has completed
      std::cout<<"m_connected= "<< m_connected<<"socket is ready"<<std::endl;
      m_sendEvent = Simulator::Schedule (Seconds (0.0), &Request::Send, this);
    }

}

void
Request::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
}


void Request::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  std::cout<<"The tcp client connect successful at "<<(Simulator::Now ()).GetSeconds ()<<std::endl;
  m_connected = true;
  m_sendEvent = Simulator::Schedule (Seconds (0.0), &Request::Send, this);
  
}


void Request::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("Request, Connection Failed");
}


void
Request::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());
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

  if ((m_socket->Send (p)) >= 0)
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

  if (m_sent < m_count)
    {
      m_sendEvent = Simulator::Schedule (m_interval, &Request::Send, this);
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
  NS_LOG_FUNCTION (this << socket);
  socket->Close();
  //delete m_socket;    deletes() is private
  
}

void Request::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}


} // Namespace ns3
