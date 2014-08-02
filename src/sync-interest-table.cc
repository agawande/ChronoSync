/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2014 University of California, Los Angeles
 *
 * This file is part of ChronoSync, synchronization library for distributed realtime
 * applications for NDN.
 *
 * ChronoSync is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * ChronoSync is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ChronoSync, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Zhenkai Zhu <http://irl.cs.ucla.edu/~zhenkai/>
 * @author Chaoyi Bian <bcy@pku.edu.cn>
 * @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 */

#include "sync-interest-table.h"
#include "sync-logging.h"

INIT_LOGGER ("SyncInterestTable")

namespace Sync
{

SyncInterestTable::SyncInterestTable (boost::asio::io_service& io, ndn::time::system_clock::Duration lifetime)
  : m_entryLifetime (lifetime)
  , m_scheduler(io)
{
  m_scheduler.scheduleEvent(ndn::time::seconds (4),
                            ndn::bind(&SyncInterestTable::expireInterests, this));
}

SyncInterestTable::~SyncInterestTable ()
{
}

Interest
SyncInterestTable::pop ()
{
  if (m_table.size () == 0)
    BOOST_THROW_EXCEPTION (Error::InterestTableIsEmpty ());

  Interest ret = *m_table.begin ();
  m_table.erase (m_table.begin ());

  return ret;
}

bool
SyncInterestTable::insert (DigestConstPtr digest, const std::string& name, bool unknownState/*=false*/)
{
  bool existent = false;

  InterestContainer::index<named>::type::iterator it = m_table.get<named> ().find (name);
  if (it != m_table.end())
    {
      existent = true;
      m_table.erase (it);
    }
  m_table.insert (Interest (digest, name, unknownState));

  return existent;
}

uint32_t
SyncInterestTable::size () const
{
  return m_table.size ();
}

bool
SyncInterestTable::remove (const std::string& name)
{
  InterestContainer::index<named>::type::iterator item = m_table.get<named> ().find (name);
  if (item != m_table.get<named> ().end ())
    {
      m_table.get<named> ().erase (name);
      return true;
    }

  return false;
}

bool
SyncInterestTable::remove (DigestConstPtr digest)
{
  InterestContainer::index<hashed>::type::iterator item = m_table.get<hashed> ().find (digest);
  if (item != m_table.get<hashed> ().end ())
    {
      m_table.get<hashed> ().erase (digest); // erase all records associated with the digest
      return true;
    }
  return false;
}

void SyncInterestTable::expireInterests ()
{
  uint32_t count = 0;
  ndn::time::system_clock::TimePoint expireTime = ndn::time::system_clock::now() - m_entryLifetime;

  while (m_table.size () > 0)
    {
      InterestContainer::index<timed>::type::iterator item = m_table.get<timed> ().begin ();

      if (item->m_time <= expireTime)
        {
          m_table.get<timed> ().erase (item);
          count ++;
        }
      else
        break;
  }

  m_scheduler.scheduleEvent(ndn::time::seconds (4),
                            ndn::bind(&SyncInterestTable::expireInterests, this));
}


}
