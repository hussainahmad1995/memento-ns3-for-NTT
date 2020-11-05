/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
//

// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.

#include "fstream"
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/double.h"

#include "cdf-application.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CdfApplication");

NS_OBJECT_ENSURE_REGISTERED(CdfApplication);

TypeId
CdfApplication::GetTypeId(void)
{
  static TypeId tid =
      TypeId("ns3::CdfApplication")
          .SetParent<Application>()
          .SetGroupName("Applications")
          .AddConstructor<CdfApplication>()
          .AddAttribute("DataRate", "The data rate in on state.",
                        DataRateValue(DataRate("500kb/s")),
                        MakeDataRateAccessor(&CdfApplication::m_Rate),
                        MakeDataRateChecker())
          .AddAttribute("CdfFile", "Message size distribution file.",
                        StringValue(""),
                        MakeStringAccessor(&CdfApplication::m_filename),
                        MakeStringChecker())
          .AddAttribute("Remote", "The address of the destination",
                        AddressValue(),
                        MakeAddressAccessor(&CdfApplication::m_peer),
                        MakeAddressChecker())
          .AddAttribute("MaxBytes",
                        "The total number of bytes to send. Once these bytes are sent, "
                        "no packet is sent again, even in on state. The value zero means "
                        "that there is no limit.",
                        UintegerValue(0),
                        MakeUintegerAccessor(&CdfApplication::m_maxBytes),
                        MakeUintegerChecker<uint64_t>())
          .AddAttribute("Protocol", "The type of protocol to use. This should be "
                                    "a subclass of ns3::SocketFactory",
                        TypeIdValue(UdpSocketFactory::GetTypeId()),
                        MakeTypeIdAccessor(&CdfApplication::m_tid),
                        // This should check for SocketFactory as a parent
                        MakeTypeIdChecker())
          .AddTraceSource("Tx", "A new packet is created and is sent",
                          MakeTraceSourceAccessor(&CdfApplication::m_txTrace),
                          "ns3::Packet::TracedCallback")
          .AddTraceSource("TxWithAddresses", "A new packet is created and is sent",
                          MakeTraceSourceAccessor(&CdfApplication::m_txTraceWithAddresses),
                          "ns3::Packet::TwoAddressTracedCallback");
  return tid;
}

CdfApplication::CdfApplication()
    : m_socket(0),
      m_connected(false),
      m_lastStartTime(Seconds(0)),
      m_totBytes(0),
      m_loaded_filename(""),
      m_average_size(0),
      m_sizeDist(CreateObject<EmpiricalRandomVariable>()),
      m_timeDist(CreateObject<ExponentialRandomVariable>())
{
  NS_LOG_FUNCTION(this);
  LoadDistribution(); // TODO REMOVE
}

CdfApplication::~CdfApplication()
{
  NS_LOG_FUNCTION(this);
}

void CdfApplication::SetMaxBytes(uint64_t maxBytes)
{
  NS_LOG_FUNCTION(this << maxBytes);
  m_maxBytes = maxBytes;
}

Ptr<Socket>
CdfApplication::GetSocket(void) const
{
  NS_LOG_FUNCTION(this);
  return m_socket;
}

int64_t
CdfApplication::AssignStreams(int64_t stream)
{
  NS_LOG_FUNCTION(this << stream);
  m_sizeDist->SetStream(stream);
  m_timeDist->SetStream(stream + 1);
  return 2;
}

void CdfApplication::DoDispose(void)
{
  NS_LOG_FUNCTION(this);

  CancelEvents();
  m_socket = 0;
  // chain up
  Application::DoDispose();
}

// Application Methods
void CdfApplication::StartApplication() // Called at time specified by Start
{
  NS_LOG_FUNCTION(this);

  // Create the socket if not already
  if (!m_socket)
  {
    m_socket = Socket::CreateSocket(GetNode(), m_tid);
    if (Inet6SocketAddress::IsMatchingType(m_peer))
    {
      if (m_socket->Bind6() == -1)
      {
        NS_FATAL_ERROR("Failed to bind socket");
      }
    }
    else if (InetSocketAddress::IsMatchingType(m_peer) ||
             PacketSocketAddress::IsMatchingType(m_peer))
    {
      if (m_socket->Bind() == -1)
      {
        NS_FATAL_ERROR("Failed to bind socket");
      }
    }
    m_socket->Connect(m_peer);
    m_socket->SetAllowBroadcast(true);
    m_socket->ShutdownRecv();

    m_socket->SetConnectCallback(
        MakeCallback(&CdfApplication::ConnectionSucceeded, this),
        MakeCallback(&CdfApplication::ConnectionFailed, this));
  }

  // Insure no pending event
  CancelEvents();
  // If we are not yet connected, there is nothing to do here
  // The ConnectionComplete upcall will start timers at that time
  //if (!m_connected) return;
  ScheduleNextTx();
}

void CdfApplication::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION(this);

  CancelEvents();
  if (m_socket != 0)
  {
    m_socket->Close();
  }
  else
  {
    NS_LOG_WARN("CdfApplication found null socket to close in StopApplication");
  }
}

void CdfApplication::CancelEvents()
{
  NS_LOG_FUNCTION(this);
  Simulator::Cancel(m_sendEvent);
  Simulator::Cancel(m_startStopEvent);
}

// Private helpers
void CdfApplication::ScheduleNextTx()
{
  NS_LOG_FUNCTION(this);

  if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
  {
    // Load dist (noop if already loaded).
    LoadDistribution();
    // Draw waiting time.
    auto nextTime = Seconds(m_timeDist->GetValue());
    NS_LOG_DEBUG("Wait Time: " << nextTime.GetMilliSeconds() << "ms.");
    m_sendEvent = Simulator::Schedule(nextTime,
                                      &CdfApplication::SendPacket, this);
  }
  else
  { // All done, cancel any pending events
    StopApplication();
  }
}

void CdfApplication::SendPacket()
{
  NS_LOG_FUNCTION(this);

  // Draw packet size.
  LoadDistribution(); // Ensure dist is loaded. Does nothing if it is.
  auto size = m_sizeDist->GetInteger();
  NS_LOG_DEBUG("Choosen Size: " << size << " Bytes.");

  NS_ASSERT(m_sendEvent.IsExpired());
  Ptr<Packet> packet = Create<Packet>(size);
  m_txTrace(packet);
  m_socket->Send(packet);
  m_totBytes += size;
  Address localAddress;
  m_socket->GetSockName(localAddress);
  if (InetSocketAddress::IsMatchingType(m_peer))
  {
    NS_LOG_INFO("At time " << Simulator::Now().GetSeconds()
                           << "s on-off application sent "
                           << packet->GetSize() << " bytes to "
                           << InetSocketAddress::ConvertFrom(m_peer).GetIpv4()
                           << " port " << InetSocketAddress::ConvertFrom(m_peer).GetPort()
                           << " total Tx " << m_totBytes << " bytes");
    m_txTraceWithAddresses(packet, localAddress, InetSocketAddress::ConvertFrom(m_peer));
  }
  else if (Inet6SocketAddress::IsMatchingType(m_peer))
  {
    NS_LOG_INFO("At time " << Simulator::Now().GetSeconds()
                           << "s on-off application sent "
                           << packet->GetSize() << " bytes to "
                           << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6()
                           << " port " << Inet6SocketAddress::ConvertFrom(m_peer).GetPort()
                           << " total Tx " << m_totBytes << " bytes");
    m_txTraceWithAddresses(packet, localAddress, Inet6SocketAddress::ConvertFrom(m_peer));
  }
  m_lastStartTime = Simulator::Now();
  ScheduleNextTx();
}

void CdfApplication::ConnectionSucceeded(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION(this << socket);
  m_connected = true;
}

void CdfApplication::ConnectionFailed(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION(this << socket);
}

void CdfApplication::LoadDistribution()
{
  NS_LOG_FUNCTION(this);
  // in any case, make sure the data rate is up to date, in case m_Rate was
  // changed.
  // If loaded already, do nothing else.
  if (m_filename == m_loaded_filename)
  {
    UpdateRateDistribution();
    return;
  }

  // Reset dist
  m_sizeDist = CreateObject<EmpiricalRandomVariable>();

  NS_LOG_DEBUG("FILENAME " << m_filename);
  std::ifstream distFile(m_filename);

  if (!(distFile >> m_average_size))
  {
    NS_FATAL_ERROR("Could not parse file: " << m_filename);
  }
  UpdateRateDistribution();
  NS_LOG_DEBUG("Average size: " << m_average_size << " Bytes.");
  NS_LOG_DEBUG("Average interarrival time: " << m_timeDist->GetMean() << "s.");

  double value, probability;
  while (distFile >> value >> probability)
  {
    NS_LOG_DEBUG(value << ", " << probability);
    m_sizeDist->CDF(value, probability);
  }

  m_loaded_filename = m_filename;
}

void CdfApplication::UpdateRateDistribution()
{
  NS_LOG_FUNCTION(this);
  auto timeBetween = m_Rate.CalculateBytesTxTime(m_average_size);
  NS_LOG_DEBUG(8 * m_average_size / timeBetween.GetSeconds() / 1000 << " kbps.");
  m_timeDist->SetAttribute("Mean", DoubleValue(timeBetween.GetSeconds()));
}

} // Namespace ns3
