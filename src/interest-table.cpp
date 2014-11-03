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
 * @author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "interest-table.hpp"

namespace chronosync {

InterestTable::InterestTable(boost::asio::io_service& io)
  : m_scheduler(io)
{
}

InterestTable::~InterestTable()
{
  clear();
}

bool
InterestTable::insert(shared_ptr<const Interest> interest,
                      ndn::ConstBufferPtr digest,
                      bool isKnown/*=false*/)
{
  bool doesExist = erase(digest);

  UnsatisfiedInterestPtr request =
    make_shared<UnsatisfiedInterest>(interest, digest, isKnown);

  time::milliseconds entryLifetime = interest->getInterestLifetime();
  if (entryLifetime < time::milliseconds::zero())
    entryLifetime = ndn::DEFAULT_INTEREST_LIFETIME;

  request->expirationEvent =
    m_scheduler.scheduleEvent(entryLifetime,
                              [=] () { quiteErase(digest); });

  m_table.insert(request);

  return doesExist;
}

void
InterestTable::quiteErase(ndn::ConstBufferPtr digest)
{
  erase(digest);
}

bool
InterestTable::erase(ndn::ConstBufferPtr digest)
{
  InterestContainer::index<hashed>::type::iterator it = m_table.get<hashed>().find(digest);
  if (it != m_table.get<hashed>().end()) {
    m_scheduler.cancelEvent((*it)->expirationEvent);
    m_table.erase(it);
    return true;
  }
  return false;
}

size_t
InterestTable::size() const
{
  return m_table.size();
}

void
InterestTable::clear()
{
  for (InterestContainer::iterator it = m_table.begin();
       it != m_table.end(); it++) {
    m_scheduler.cancelEvent((*it)->expirationEvent);
  }

  m_table.clear();
}

}
