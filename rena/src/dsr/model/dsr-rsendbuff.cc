/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Yufei Cheng
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
 * Author: Yufei Cheng   <yfcheng@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

#include "dsr-rsendbuff.h"
#include <algorithm>
#include <functional>
#include "ns3/ipv4-route.h"
#include "ns3/socket.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("DsrSendBuffer");

namespace ns3 {
namespace dsr {

uint32_t
SendBuffer::GetSize ()
{
  Purge ();
  return m_sendBuffer.size ();
}

bool
SendBuffer::Enqueue (SendBuffEntry & entry)
{
  Purge ();
  for (std::vector<SendBuffEntry>::const_iterator i = m_sendBuffer.begin (); i
       != m_sendBuffer.end (); ++i)
    {
      NS_LOG_INFO ("packet id " << i->GetPacket ()->GetUid () << " " << entry.GetPacket ()->GetUid ()
                   << " dst " << i->GetDestination () << " " << entry.GetDestination ());

      if ((i->GetPacket ()->GetUid () == entry.GetPacket ()->GetUid ())
          && (i->GetDestination () == entry.GetDestination ()))
        {
          return false;
        }
    }

  entry.SetExpireTime (m_sendBufferTimeout);     // Initialize the send buffer timeout
  /*
   * Drop the most aged packet when buffer reaches to max
   */
  if (m_sendBuffer.size () >= m_maxLen)
    {
      Drop (m_sendBuffer.front (), "Drop the most aged packet");         // Drop the most aged packet
      m_sendBuffer.erase (m_sendBuffer.begin ());
    }
  // enqueue the entry
  m_sendBuffer.push_back (entry);
  return true;
}

void
SendBuffer::DropPacketWithDst (Ipv4Address dst)
{
  NS_LOG_FUNCTION (this << dst);
  Purge ();
  /*
   * Drop the packet with destination address dst
   */
  for (std::vector<SendBuffEntry>::iterator i = m_sendBuffer.begin (); i
       != m_sendBuffer.end (); ++i)
    {
      if (IsEqual (*i, dst))
        {
          Drop (*i, "DropPacketWithDst");
        }
    }
  m_sendBuffer.erase (std::remove_if (m_sendBuffer.begin (), m_sendBuffer.end (),
                                      std::bind2nd (std::ptr_fun (SendBuffer::IsEqual), dst)), m_sendBuffer.end ());
}

bool
SendBuffer::Dequeue (Ipv4Address dst, SendBuffEntry & entry)
{
  Purge ();
  /*
   * Dequeue the entry with destination address dst
   */
  for (std::vector<SendBuffEntry>::iterator i = m_sendBuffer.begin (); i != m_sendBuffer.end (); ++i)
    {
      if (i->GetDestination () == dst)
        {
          entry = *i;
          m_sendBuffer.erase (i);
          NS_LOG_DEBUG ("Packet size while dequeuing " << entry.GetPacket ()->GetSize ());
          return true;
        }
    }
  return false;
}

bool
SendBuffer::Find (Ipv4Address dst)
{
  /*
   * Make sure if the send buffer contains entry with certain dst
   */
  for (std::vector<SendBuffEntry>::const_iterator i = m_sendBuffer.begin (); i
       != m_sendBuffer.end (); ++i)
    {
      if (i->GetDestination () == dst)
        {
          NS_LOG_DEBUG ("Found the packet");
          return true;
        }
    }
  return false;
}

struct IsExpired
{
  bool
  operator() (SendBuffEntry const & e) const
  {
    // NS_LOG_DEBUG("Expire time for packet in req queue: "<<e.GetExpireTime ());
    return (e.GetExpireTime () < Seconds (0));
  }
};

void
SendBuffer::Purge ()
{
  /*
   * Purge the buffer to eliminate expired entries
   */
  NS_LOG_DEBUG ("The send buffer size " << m_sendBuffer.size ());
  IsExpired pred;
  for (std::vector<SendBuffEntry>::iterator i = m_sendBuffer.begin (); i
       != m_sendBuffer.end (); ++i)
    {
      if (pred (*i))
        {
          NS_LOG_DEBUG ("Dropping Queue Packets");
          Drop (*i, "Drop out-dated packet ");
        }
    }
  m_sendBuffer.erase (std::remove_if (m_sendBuffer.begin (), m_sendBuffer.end (), pred),
                      m_sendBuffer.end ());
}

void
SendBuffer::Drop (SendBuffEntry en, std::string reason)
{
  NS_LOG_LOGIC (reason << en.GetPacket ()->GetUid () << " " << en.GetDestination ());
//  en.GetErrorCallback () (en.GetPacket (), en.GetDestination (),
//     Socket::ERROR_NOROUTETOHOST);
  return;
}
}  // namespace dsr
}  // namespace ns3
