// File         : the_qt_mutex.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Feb 18 16:12:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A Qt mutex wrapper class.

#ifndef THE_QT_MUTEX_HXX_
#define THE_QT_MUTEX_HXX_

// Qt includes:
#include <qmutex.h>

// local includes:
#include "thread/the_mutex_interface.hxx"


//----------------------------------------------------------------
// the_qt_mutex_t
// 
class the_qt_mutex_t : public QMutex, public the_mutex_interface_t
{
public:
  // the creation method:
  static the_mutex_interface_t * create();
  
  // virtual:
  void lock();
  
  // virtual:
  void unlock();
  
  // virtual:
  bool try_lock();
};


#endif // THE_QT_MUTEX_HXX_
