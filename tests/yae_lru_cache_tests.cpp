// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jan 14 12:47:05 MST 2018
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php


// standard:
#include <map>

// boost library:
#include <boost/test/unit_test.hpp>

// aeyae:
#include "yae/utils/yae_lru_cache.h"

// shortcut:
using namespace yae;


// keep track of how many times the factory method was called:
static std::map<unsigned int, int> factory_call_count;

//----------------------------------------------------------------
// factory
//
static bool
factory(void * context, const int & key, unsigned int & value)
{
  factory_call_count[key] += 1;
  value = ((unsigned int)key) << 8;
  return true;
}

BOOST_AUTO_TEST_CASE(yae_lru_cache)
{
  typedef LRUCache<int, unsigned int> TCache;

  TCache cache;
  cache.set_capacity(16);

  // first round, cache is empty, every referenced value
  // has to be constructed by the factory:
  for (int i = 0; i < cache.capacity(); i++)
  {
    TCache::TRefPtr ref = cache.get(i, &factory);
    BOOST_CHECK(ref->value() == (((unsigned int)i) << 8));
    BOOST_CHECK(factory_call_count[i] == 1);
  }

  // second round, cached values are unreferenced
  // and reused, no new factory calls are required:
  for (int i = 0; i < cache.capacity(); i++)
  {
    TCache::TRefPtr ref = cache.get(i, &factory);
    BOOST_CHECK(ref->value() == (((unsigned int)i) << 8));
    BOOST_CHECK(factory_call_count[i] == 1);
  }

  // third round, unreferenced cached values are purged
  // and new values are constructed by the factory:
  for (int i = 0; i < 16; i++)
  {
    int j = i + cache.capacity();
    TCache::TRefPtr ref = cache.get(j, &factory);
    BOOST_CHECK(ref->value() == (((unsigned int)j) << 8));
    BOOST_CHECK(factory_call_count[j] == 1);
  }

  // final round, cached values are unreferenced
  // and pured, new factory calls are required:
  for (int i = 0; i < cache.capacity(); i++)
  {
    TCache::TRefPtr ref = cache.get(i, &factory);
    BOOST_CHECK(ref->value() == (((unsigned int)i) << 8));
    BOOST_CHECK(factory_call_count[i] == 2);
  }

  std::list<TCache::TRefPtr> refs;
  for (int i = 0; i < cache.capacity(); i++)
  {
    refs.push_back(cache.get(i, &factory));
  }

#if 0
  // this should block because the cache is full:
  cache.get(0, &factory);
  BOOST_CHECK(false);
#else
  refs.pop_back();
  cache.get(0, &factory);
  BOOST_CHECK(factory_call_count[0] == 3);
#endif
}
