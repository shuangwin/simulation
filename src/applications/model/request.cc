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
*ͬʱ������������ļ���request.h, request-helper.cc, request-helper.h
*����ӵ�wscript�ļ��Ķ�Ӧ�б���
*Ŀǰȫ����tcp����,��ʼ����һ��Socket��Ȼ���Ժ㶨����������޸���������
*9.2
*���������Ѿ�����
*�ͻ��ˣ�֮ǰ��req��packetsink�ֱ𴴽�socket����������ͽ������ݣ��������ϲ���һ��
*10.12 
*�ͻ��˵ķ���������trace�оۺ������Ĺ��ɷ���
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
  
  m_socCreateEvent = Simulator::Schedule (Seconds (value), &Request::randomSocketCreat, this); //todo: ��ʼʱ����� 

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
  
  //JMS 10.20 add to get the contentBytes...start
  std::list<Link>::iterator iter; 
  for (iter = LinkListUE.begin(); iter != LinkListUE.end(); ++iter)
  {
    if(iter->socket == socket)
      {
          std::cout<<"There's a socket in LinkListUE!"<<std::endl;
          break;
      }        
  }
  
  if(iter == LinkListUE.end() && iter->socket!= socket)
  {
       std::cout<<"There is no inter->socket == socket in LinkListUE!"<<std::endl;
       return;  //return��䣡 ǿ�ƽ�����ǰ����
  }
  //JMS 10.20 add to get the contentBytes...end 
  
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);
  
  uint8_t buf[2];
  
  
  //JMS add the uniform random variable to create the random content type in the user request...start
  double min = 0;
  double max = 1;
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (min));
  x->SetAttribute ("Max", DoubleValue (max));
  double value = x->GetValue ();
  
  //std::cout<<"request.cc----------------uniformrandom--------"<<value<<(Simulator::Now ()).GetSeconds ()<<std::endl;
  if(value >0 && value <=0.2)
  {
    buf[0] = '1';
    iter->contentBytes = 1024*100;
    std::cout<<"Request("<<this<<") will request for 1024*100 bytes"<<std::endl; 
  }
  else if (value >0.2 && value <= 0.90)
  {
    buf[0] = '2';
    iter->contentBytes = 1024*1024*30;
    std::cout<<"Request("<<this<<") will request for 1024*1024*30 bytes"<<std::endl;
  }
  else
  {
    buf[0] = '3';
    iter->contentBytes = 1024*1024*500;
    std::cout<<"Request("<<this<<") will request for 1024*1024*500 bytes"<<std::endl;
  }
 
  //JMS add the uniform random variable to create the random content type in the user request...end
  
  //buf[0]='1'; //content name, JMS 10.19
  buf[1]='2'; //deadline,selected based on the prices, same to the behavior model in TUBE and Async JMS 10.19
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
  
  //JMS 10.20 find the content info that the socket relate to ,START
  
  std::list<Link>::iterator iter; 
  for (iter = LinkListUE.begin(); iter != LinkListUE.end(); ++iter)
  {
    if(iter->socket == socket)
      {
          std::cout<<"There's a socket in LinkListUE!"<<std::endl;
          break;
      }        
  }
  
  if(iter == LinkListUE.end() && iter->socket!= socket)
  {
       std::cout<<"There is no inter->socket == socket in LinkListUE!"<<std::endl;
       return;  //return��䣡 ǿ�ƽ�����ǰ����
  }
  
  //JMS 10.20 find the content info that the socket relate to ,END
  
  
  
  Ptr<Ipv4> p = Request::GetNode()->GetObject<Ipv4> (); //��δ�ӡ��ǰ�հ���UE��IP? ��֪�Բ���
  //"<< p->GetAddress(1,0).GetLocal () <<"
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      iter->totBytes += packet->GetSize ();
           
      
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s The UE "<< p->GetAddress(1,0).GetLocal () <<" received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " have received " << iter->totBytes << " bytes");
        }
        
        
      if (iter->totBytes == iter->contentBytes)
      {
        iter->socket->Close (); //close the socket upon total receiving the content
        std::cout<<"The UE "<<p->GetAddress(1,0).GetLocal ()<<"has finished receiving "<<iter->contentBytes<<" Bytes content"<<std::endl;
        LinkListUE.erase(iter); //delete the socket and relate info from the linklist
        
        
      }
      
    }
} //HandleRead


void Request::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
 
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
    
  //JMS 10.20 start 
  Link linknew; 
  linknew.socket = socketToUse; //give the socket pointer to the link element         
  linknew.totBytes = 0;  //initialize the total receive bytes
  linknew.contentBytes = 0; //initialize the total contentbytes bytes
  
  LinkListUE.push_back(linknew); //finish creating a socket and initialize the content information related to it 10.20
  //JMS 10.20 end
  
  double now_minute = Simulator::Now().GetMinutes(); //ȡ����ʱ�䣬�����¸�req��շ���ȥ��req֮��ļ����ֵ���ĵ�����
  double now_slotnum = floor(now_minute/15); //slot number start from 0 to 5; simtime = 60 mins
  //std::cout<<"Request.cc----now_minute = "<< now_minute<<", "<<"now_slotnum = "<< now_slotnum<<std::endl;
  //std::cout<<"request.cc---- now_slotnum = "<< floor(now_slotnum)<<std::endl; //floor() ����ȡ��
  double mean,bound;
  
  switch ((int)now_slotnum)
  {
    
    
    case 0 :
    {
        std::cout<<"request.cc in case 0 "<<std::endl;
        mean = 12;
        bound = 13;
        break;
    }
    
    case 1 :
    {
        std::cout<<"request.cc in case 1 "<<std::endl;
        mean = 9.8;
        bound = 11.4;
        break;
    }
    
    
    case 2 :
    {
        std::cout<<"request.cc in case 2 "<<std::endl;
        mean = 12.2;
        bound = 13.2 ;
        break;
    }
    
    case 3 :
    {
        std::cout<<"request.cc in case 3 "<<std::endl;
        mean = 12.6;
        bound = 13.2;
        break;
    }
    
    case 4 :
    {
        std::cout<<"request.cc in case 4 "<<std::endl;
        mean = 12.6;
        bound = 13.2;
        break;
    }
    
    case 5 :
    {
        std::cout<<"request.cc in case 5 "<<std::endl;
        mean = 11;
        bound = 12;
        break;
    }
    
    
    case 6 :
    {
        std::cout<<"request.cc in case 6 "<<std::endl;
        mean = 11.5;
        bound = 12;
        break;
    }
    
    case 7 :
    {
        std::cout<<"request.cc in case 7 "<<std::endl;
        mean = 9.8;
        bound = 10.5;
        break;
    }
    
    case 8 :
    {
        std::cout<<"request.cc in case 8 "<<std::endl;
        mean = 3.5;
        bound = 4;
        break;
    }
    
    case 9 :
    {
        std::cout<<"request.cc in case 9 "<<std::endl;
        mean = 1.2;
        bound = 1.3;
        break;
    }
    
    case 10 :
    {
        std::cout<<"request.cc in case 10 "<<std::endl;
        mean = 0.95;
        bound = 1.05;
        break;
    }
    
    case 11 :
    {
        std::cout<<"request.cc in case 11 "<<std::endl;
        mean = 0.8;
        bound = 0.85;
        break;
    }
    
    case 12 :
    {
        std::cout<<"request.cc in case 12 "<<std::endl;
        mean = 0.65;
        bound = 0.7;
        break;
    }
    
    case 13 :
    {
        std::cout<<"request.cc in case 13 "<<std::endl;
        mean = 0.5;
        bound = 0.6;
        break;
    }
    
    case 14 :
    {
        std::cout<<"request.cc in case 14 "<<std::endl;
        mean = 0.4;
        bound = 0.5;
        break;
    }
    
    case 15 :
    {
        std::cout<<"request.cc in case 15 "<<std::endl;
        mean = 0.65;
        bound = 0.7;
        break;
    }
    
    case 16 :
    {
        std::cout<<"request.cc in case 16 "<<std::endl;
        mean = 0.3;
        bound = 0.45;
        break;
    }
    
    case 17 :
    {
        std::cout<<"request.cc in case 17 "<<std::endl;
        mean = 0.28;
        bound = 0.3;
        break;
    }
    
    
    case 18 :
    {
        std::cout<<"request.cc in case 18 "<<std::endl;
        mean = 0.275;
        bound = 0.28;
        break;
    }
    
    case 19 :
    {
        std::cout<<"request.cc in case 19 "<<std::endl;
        mean = 0.4;
        bound = 0.55;
        break;
    }
    
    case 20 :
    {
        std::cout<<"request.cc in case 20 "<<std::endl;
        mean = 0.65;
        bound = 0.75;
        break;
    }
    
    case 21 :
    {
        std::cout<<"request.cc in case 21 "<<std::endl;
        mean = 0.9;
        bound = 1.3;
        break;
    }
    
    case 22 :
    {
        std::cout<<"request.cc in case 22 "<<std::endl;
        mean = 1.3;
        bound = 1.4;
        break;
    }
    
    case 23 :
    {
        std::cout<<"request.cc in case 23 "<<std::endl;
        mean = 1.5;
        bound = 1.6;
        break;
    }
          
    
  }
  
  //double mean = 10; origin 
  //double bound = 20; origin
  
  Ptr<ExponentialRandomVariable> x = CreateObject<ExponentialRandomVariable> ();
  x->SetAttribute ("Mean", DoubleValue (mean));
  x->SetAttribute ("Bound", DoubleValue (bound));
  double value = x->GetValue ();
  m_socCreateEvent = Simulator::Schedule (Minutes(value), &Request::randomSocketCreat, this); //wait and send another request
  
}

} // Namespace ns3
