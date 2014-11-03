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
 */

#include "interest-table.hpp"

#include <ndn-cxx/util/scheduler.hpp>
#include <unistd.h>

#include "boost-test.hpp"

namespace chronosync {
namespace test {

class InterestTableFixture
{
public:
  InterestTableFixture()
    : scheduler(io)
  {
    uint8_t origin[4] = {0x01, 0x02, 0x03, 0x04};
    Name prefix("/test/prefix");

    Name interestName1;
    digest1 = ndn::crypto::sha256(origin, 1);
    interestName1.append(prefix).append(name::Component(digest1));
    interest1 = make_shared<Interest>(interestName1);
    interest1->setInterestLifetime(time::milliseconds(100));

    Name interestName2;
    digest2 = ndn::crypto::sha256(origin, 2);
    interestName2.append(prefix).append(name::Component(digest2));
    interest2 = make_shared<Interest>(interestName2);
    interest2->setInterestLifetime(time::milliseconds(100));

    Name interestName3;
    digest3 = ndn::crypto::sha256(origin, 3);
    interestName3.append(prefix).append(name::Component(digest3));
    interest3 = make_shared<Interest>(interestName3);
    interest3->setInterestLifetime(time::milliseconds(100));
  }

  void
  insert(InterestTable& table,
         shared_ptr<Interest> interest,
         ndn::ConstBufferPtr digest)
  {
    table.insert(interest, digest);
  }

  void
  check(InterestTable& table, size_t size)
  {
    BOOST_CHECK_EQUAL(table.size(), size);
  }

  void
  terminate()
  {
    io.stop();
  }

  shared_ptr<Interest> interest1;
  ndn::ConstBufferPtr digest1;

  shared_ptr<Interest> interest2;
  ndn::ConstBufferPtr digest2;

  shared_ptr<Interest> interest3;
  ndn::ConstBufferPtr digest3;

  boost::asio::io_service io;
  ndn::Scheduler scheduler;
};

BOOST_FIXTURE_TEST_SUITE(InterestTableTest, InterestTableFixture)

BOOST_AUTO_TEST_CASE(Container)
{
  InterestContainer container;

  container.insert(make_shared<UnsatisfiedInterest>(interest1, digest1));
  container.insert(make_shared<UnsatisfiedInterest>(interest2, digest2));
  container.insert(make_shared<UnsatisfiedInterest>(interest3, digest3));

  BOOST_CHECK_EQUAL(container.size(), 3);
  BOOST_CHECK(container.find(digest3) != container.end());
  BOOST_CHECK(container.find(digest2) != container.end());
  BOOST_CHECK(container.find(digest1) != container.end());
}

BOOST_AUTO_TEST_CASE(Basic)
{
  InterestTable table(io);

  table.insert(interest1, digest1);
  table.insert(interest2, digest2);
  table.insert(interest3, digest3);

  BOOST_CHECK_EQUAL(table.size(), 3);
  InterestTable::const_iterator it = table.begin();
  BOOST_CHECK(it != table.end());
  it++;
  BOOST_CHECK(it != table.end());
  it++;
  BOOST_CHECK(it != table.end());
  it++;
  BOOST_CHECK(it == table.end());

  BOOST_CHECK_EQUAL(table.size(), 3);
  table.erase(digest1);
  BOOST_CHECK_EQUAL(table.size(), 2);
  table.erase(digest2);
  BOOST_CHECK_EQUAL(table.size(), 1);
  ConstUnsatisfiedInterestPtr pendingInterest = *table.begin();
  table.clear();
  BOOST_CHECK_EQUAL(table.size(), 0);
  BOOST_CHECK(*pendingInterest->digest == *digest3);
}

BOOST_AUTO_TEST_CASE(Expire)
{
  InterestTable table(io);

  scheduler.scheduleEvent(ndn::time::milliseconds(50),
                          ndn::bind(&InterestTableFixture::insert, this,
                                    ndn::ref(table), interest1, digest1));

  scheduler.scheduleEvent(ndn::time::milliseconds(150),
                          ndn::bind(&InterestTableFixture::insert, this,
                                    ndn::ref(table), interest2, digest2));

  scheduler.scheduleEvent(ndn::time::milliseconds(150),
                          ndn::bind(&InterestTableFixture::insert, this,
                                    ndn::ref(table), interest3, digest3));

  scheduler.scheduleEvent(ndn::time::milliseconds(200),
                          ndn::bind(&InterestTableFixture::insert, this,
                                    ndn::ref(table), interest2, digest2));

  scheduler.scheduleEvent(ndn::time::milliseconds(220),
                          ndn::bind(&InterestTableFixture::check, this,
                                    ndn::ref(table), 2));

  scheduler.scheduleEvent(ndn::time::milliseconds(270),
                          ndn::bind(&InterestTableFixture::check, this,
                                    ndn::ref(table), 1));

  scheduler.scheduleEvent(ndn::time::milliseconds(420),
                          ndn::bind(&InterestTableFixture::check, this,
                                    ndn::ref(table), 0));

  scheduler.scheduleEvent(ndn::time::milliseconds(500),
                          ndn::bind(&InterestTableFixture::terminate, this));

  io.run();
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace test
} // namespace chronosync
