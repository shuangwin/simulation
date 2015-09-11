/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"


#include "ns3/tcp-socket-factory.h"
#include "content-server.h"
#include "ns3/uinteger.h"
#include "ns3/ptr.h"

/*Modify packet-sink.cc, refer to bulk-send-application.cc to be the app on remotehost to send udp/tcp content to UEs
*8.31
*同时添加其他三个文件：content-server.h, content-server-helper.cc, content-server-helper.h
*并添加到wscript文件的对应列表中
*/

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ContentServer");

NS_OBJECT_ENSURE_REGISTERED (ContentServer);

TypeId 
ContentServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ContentServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<ContentServer> ()
    .AddAttribute ("Local",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&ContentServer::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&ContentServer::m_tid),
                   MakeTypeIdChecker ())
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&ContentServer::m_rxTrace),
                     "ns3::Packet::PacketAddressTracedCallback")
    .AddAttribute ("SendSize", "The amount of data to send each time.",
                   UintegerValue (512),
                   MakeUintegerAccessor (&ContentServer::m_sendSize),
                   MakeUintegerChecker<uint32_t> (1))
  ;
  return tid;
}

ContentServer::ContentServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  
  
}

ContentServer::~ContentServer()
{
  NS_LOG_FUNCTION (this);
}

uint32_t ContentServer::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}

Ptr<Socket>
ContentServer::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
ContentServer::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void ContentServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void ContentServer::StartApplication ()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  std::cout<<"The contentServer starts at "<<Simulator::Now ().GetSeconds ()<<std::endl;
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      m_socket->Bind (m_local);
      m_socket->Listen ();
      //m_socket->ShutdownSend (); 此处不注释掉，ACCEPT到的socket默认都是shutdowndsend的
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
    }

  
  
  /*
  *函数原型
  *Socket::SetRecvCallback(Callback<void, Ptr<Socket>> receivedData)
  *通知一个处于阻塞状态中的socket有数据可以读
  */
  m_socket->SetRecvCallback (MakeCallback (&ContentServer::HandleRead, this));
  
  /*
  *函数原型
  *Socket::SetAccpetCallback(Callback<bool, Ptr<Socket>, const Address &> connectionRequest,
                             Callback<void, Ptr<Socket>, const Address &> newConnectionCreated)
  *回调1：接受socket请求返回真，否则假；若接受了请求，则执行回调2
  *回调2：给新建立的socket传递一个指针，及连接发起方的ip地址及端口号
  */
  
  //MakeCallback
  
  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&ContentServer::HandleAccept, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&ContentServer::HandlePeerClose, this),
    MakeCallback (&ContentServer::HandlePeerError, this));
}

void ContentServer::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
      Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
    }
  if (m_socket) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void ContentServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
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
                       << "s content server received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
          uint8_t buf[14];
          packet->CopyData(buf,14);
          uint8_t content = buf[12];
          //uint8_t rate = buf[13];
          
          Link linknew;          
          linknew.totBytes = 0; 
          switch(content)
           {
                case '1':
                    {
                        //std::cout<< "The requested type from "<<InetSocketAddress::ConvertFrom(from).GetIpv4 ()<<" is "<<"1"<<std::endl;
                        linknew.contentBytes = 1024*5;
                        break;     
                    }
                 
                case '2':
                    {
                        linknew.contentBytes = 1024*20;
                        break;    
                    }
                case '3':
                    { 
                        linknew.contentBytes = 1024*100; 
                        break;
                    }
           
           }//switch
           
           //std::cout<<"The m_contentBytes= "<< m_contentBytes<< std::endl;
            
           linknew.socket = socket;
           //linknew.socket->SetSendCallback (MakeCallback (&ContentServer::DataSend, this));
           LinkList.push_back(linknew);
           SendData (linknew.socket);
        
        }
   
    }
}


void ContentServer::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 
void ContentServer::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}


void ContentServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&ContentServer::HandleRead, this));
  m_socketList.push_back (s);
}

//add start
void ContentServer::SendData(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  //遍历LinkList,查询跟socket对应的contentBytes和当前的totBytes
  std::list<Link>::iterator iter;  //针对list类型声明局部迭代器
  
  
  //LinkList.end()是链表最后一个元素的下一个（无效的指针）! 如数组a[2],LinkList.end()相当于是a[2].
  for (iter = LinkList.begin(); iter != LinkList.end(); ++iter)
  {
      if(iter->socket == socket)
      {
          std::cout<<"There's a socket in LinkList!"<<std::endl;
          break;
      }
      
          
  }
    
    
    if(iter == LinkList.end() && iter->socket!= socket)
    {
          std::cout<<"There is no inter->socket == socket in LinkList!"<<std::endl;
          return;  //return语句！ 强制结束当前函数
    }
    
        
        
  //遍历finish
  
  while(iter->totBytes < iter->contentBytes)
    {
      
      //std::cout<< "iter->totBytes= "<<iter->totBytes<<"  iter->contentBytes= "<<iter->contentBytes<<std::endl;
      uint32_t toSend = m_sendSize;
      toSend = std::min (m_sendSize, iter->contentBytes - iter->totBytes);
      Ptr<Packet> packet = Create<Packet> (toSend);
      
      int actual = iter->socket->Send (packet);
      std::cout<<"actual= "<< actual<<std::endl;
     
      if (actual > 0)
        {
          iter->totBytes += actual;
        }
      if ((unsigned)actual != toSend)
        {
          std::cout<<"actual!= toSend "<< std::endl;
          break;
          //return; //?
        }
    }
    
    
    if (iter->totBytes == iter->contentBytes)
    {
      
      //std::cout<<"Send finish, totBytes == contentBytes"<<std::endl;
      iter->socket->Close ();
      //如果这个link的数据已经传输完毕，则从LinkList中删除这个结构体元素
      LinkList.erase(iter);
      
      //进一步删除m_socketList中的socket
      std::list<Ptr<Socket> >::iterator socket_iter;
      for (socket_iter = m_socketList.begin(); socket_iter != m_socketList.end(); ++socket_iter)
      {
        if(*socket_iter == socket)
        {
          m_socketList.erase(socket_iter);
          std::cout<<"Have erase the socket_iter in m_socketList!"<<std::endl;
          break;
        }
          
      }
      
    }


}


//修改Bulksend中的DataSend，以给SendData传递正在发送包的socket的Ptr<Socket>变量。
//num:the number of bytes available for writing into the buffer (an absolute value)
void ContentServer::DataSend (Ptr<Socket> socket, uint32_t num)
{
  NS_LOG_FUNCTION (this<<socket<<num);

     
  std::list<Link>::iterator iter;
  for (iter = LinkList.begin(); iter != LinkList.end(); ++iter)
    {
      if(iter->socket == socket)
      {
         SendData (socket); 
         std::cout<<"ContentServer---DataSend---will break when there's a socket in LinkList"<<std::endl;
         break;     
      }
    }
    
  if(iter == LinkList.end() && iter->socket!= socket)
    {
        std::cout<<"ContentServer---DataSend---will return when there's no socket in LinkList"<<std::endl;
        return;
    }
        
    
     
}

//add finish

} // Namespace ns3
