/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012 University of California, Los Angeles
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
 * Author: Zhenkai Zhu <zhenkai@cs.ucla.edu>
 *         Chaoyi Bian <bcy@pku.edu.cn>
 *	   Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#ifndef SYNC_CCNX_WRAPPER_H
#define SYNC_CCNX_WRAPPER_H

extern "C" {
#include <ccn/ccn.h>
#include <ccn/charbuf.h>
#include <ccn/keystore.h>
#include <ccn/uri.h>
#include <ccn/bloom.h>
#include <ccn/signing.h>
}

#include <boost/exception/all.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/function.hpp>
#include <string>
#include <sstream>
#include <map>

/**
 * \defgroup sync SYNC protocol
 *
 * Implementation of SYNC protocol
 */
namespace Sync {

struct CcnxOperationException : virtual boost::exception, virtual std::exception { };
/**
 * \ingroup sync
 * @brief A wrapper for ccnx library; clients of this code do not need to deal
 * with ccnx library
 */
class CcnxWrapper {
public:
  typedef boost::function<void (std::string, std::string)> StringDataCallback;
  typedef boost::function<void (std::string, const char *buf, size_t len)> RawDataCallback;
  typedef boost::function<void (std::string)> InterestCallback;

public:

#ifdef _DEBUG_WRAPPER_      
  CcnxWrapper(char c='.');
  char m_c;
#else
  CcnxWrapper();
#endif

  ~CcnxWrapper();

  /**
   * @brief send Interest; need to grab lock m_mutex first
   *
   * @param strInterest the Interest name
   * @param dataCallback the callback function to deal with the returned data
   * @return the return code of ccn_express_interest
   */
  int
  sendInterestForString (const std::string &strInterest, const StringDataCallback &strDataCallback, int retry = 0);

  int 
  sendInterest (const std::string &strInterest, const RawDataCallback &rawDataCallback, int retry = 0);

  /**
   * @brief set Interest filter (specify what interest you want to receive)
   *
   * @param prefix the prefix of Interest
   * @param interestCallback the callback function to deal with the returned data
   * @return the return code of ccn_set_interest_filter
   */
  int
  setInterestFilter (const std::string &prefix, const InterestCallback &interestCallback);

  /**
   * @brief clear Interest filter
   * @param prefix the prefix of Interest
   */
  void
  clearInterestFilter (const std::string &prefix);

  /**
   * @brief publish data and put it to local ccn content store; need to grab
   * lock m_mutex first
   *
   * @param name the name for the data object
   * @param dataBuffer the data to be published
   * @param freshness the freshness time for the data object
   * @return code generated by ccnx library calls, >0 if success
   */
  int
  publishStringData (const std::string &name, const std::string &dataBuffer, int freshness);

  int 
  publishRawData (const std::string &name, const char *buf, size_t len, int freshness);

  std::string
  getLocalPrefix ();
  
protected:
  void
  connectCcnd();

  /// @cond include_hidden 
  void
  createKeyLocator ();

  void
  initKeyStore ();

  const ccn_pkey *
  getPrivateKey ();

  const unsigned char *
  getPublicKeyDigest ();

  ssize_t
  getPublicKeyDigestLength ();

  void
  ccnLoop ();

  int 
  sendInterest (const std::string &strInterest, void *dataPass);
  /// @endcond
protected:
  ccn* m_handle;
  ccn_keystore *m_keyStore;
  ccn_charbuf *m_keyLoactor;
  // to lock, use "boost::recursive_mutex::scoped_lock scoped_lock(mutex);
  boost::recursive_mutex m_mutex;
  boost::thread m_thread;
  bool m_running;
  bool m_connected;
  std::map<std::string, InterestCallback> m_registeredInterests;
  // std::list< std::pair<std::string, InterestCallback> > m_registeredInterests;
};

typedef boost::shared_ptr<CcnxWrapper> CcnxWrapperPtr;

enum CallbackType { STRING_FORM, RAW_DATA};

class ClosurePass {
public:
  ClosurePass(CallbackType type, int retry): m_retry(retry), m_type(type) {}
  int getRetry() {return m_retry;}
  void decRetry() { m_retry--;}
  CallbackType getCallbackType() {return m_type;}
  virtual ~ClosurePass(){}
  virtual void runCallback(std::string name, const char *data, size_t len) = 0;

protected:
  int m_retry;
  CallbackType m_type;
};

class DataClosurePass: public ClosurePass {
public:
  DataClosurePass(CallbackType type, int retry, const CcnxWrapper::StringDataCallback &strDataCallback);
  virtual ~DataClosurePass();
  virtual void runCallback(std::string name, const char *, size_t len);
private:
  CcnxWrapper::StringDataCallback * m_callback;  
};

class RawDataClosurePass: public ClosurePass {
public:
  RawDataClosurePass(CallbackType type, int retry, const CcnxWrapper::RawDataCallback &RawDataCallback);
  virtual ~RawDataClosurePass();
  virtual void runCallback(std::string name, const char *, size_t len);
private:
  CcnxWrapper::RawDataCallback * m_callback;  
};

} // Sync

#endif // SYNC_CCNX_WRAPPER_H