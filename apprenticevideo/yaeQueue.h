// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 15 12:32:01 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_QUEUE_H_
#define YAE_QUEUE_H_

// system includes:
#include <set>
#include <list>

// boost includes:
#include <boost/thread.hpp>

// yae includes:
#include <yaeAPI.h>


namespace yae
{
  
  //----------------------------------------------------------------
  // Queue
  // 
  // Thread-safe queue
  // 
  template <typename TData>
  struct Queue
  {
    typedef Queue<TData> TSelf;
    
    Queue(std::size_t maxSize = 61):
      closed_(true),
      size_(0),
      maxSize_(maxSize)
    {}
    
    ~Queue()
    {
      close();
      
      try
      {
        boost::this_thread::disable_interruption disableInterruption;
        boost::lock_guard<boost::mutex> lock(mutex_);
        
        while (!data_.empty())
        {
          data_.pop_front();
          size_--;
        }
        
        // sanity check:
        assert(size_ == 0);
      }
      catch (...)
      {}
    }
    
    void setMaxSize(std::size_t maxSize)
    {
      // change max queue size:
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (maxSize_ == maxSize)
        {
          // same size, nothing changed:
          return;
        }
        
        maxSize_ = maxSize;
      }
      
      cond_.notify_all();
    }
    
    // check whether the Queue is empty:
    bool isEmpty() const
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      bool isEmpty = data_.empty();
      return isEmpty;
    }
    
    // check whether the queue is closed:
    bool isClosed() const
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      return closed_;
    }
    
    // close the queue, abort any pending push/pop/etc... calls:
    void close()
    {
      // close the queue:
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (closed_)
        {
          // already closed:
          return;
        }
        
        closed_ = true;
      }
      
      cond_.notify_all();
    }
    
    // open the queue allowing push/pop/etc... calls:
    void open()
    {
      // open the queue:
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (!closed_)
        {
          // already open:
          return;
        }
        
        data_.clear();
        closed_ = false;
      }
      
      cond_.notify_all();
    }
    
    // push data into the queue:
    bool push(const TData & newData)
    {
      try
      {
        // add to queue:
        {
          boost::unique_lock<boost::mutex> lock(mutex_);
          while (!closed_ && size_ >= maxSize_)
          {
            cond_.wait(lock);
          }
          
          if (size_ >= maxSize_)
          {
            return false;
          }
          
          data_.push_back(newData);
          size_++;
        }
        
        cond_.notify_one();
        return true;
      }
      catch (...)
      {}
      
      return false;
    }

    // remove data from the queue:
    bool pop(TData & data)
    {
      try
      {
        // remove from queue:
        {
          boost::unique_lock<boost::mutex> lock(mutex_);
          while (!closed_ && data_.empty())
          {
            cond_.wait(lock);
          }
          
          if (data_.empty())
          {
            return false;
          }
          
          data = data_.front();
          data_.pop_front();
          size_--;
        }
        
        cond_.notify_one();
        return true;
      }
      catch (...)
      {}
      
      return false;
    }
    
    // push data into the queue:
    bool tryPush(const TData & newData)
    {
      try
      {
        // add to queue:
        {
          boost::unique_lock<boost::mutex> lock(mutex_);
          
          if (closed_ || size_ >= maxSize_)
          {
            return false;
          }
          
          data_.push_back(newData);
          size_++;
        }
        
        cond_.notify_one();
        return true;
      }
      catch (...)
      {}
      
      return false;
    }

    // remove data from the queue:
    bool tryPop(TData & data)
    {
      try
      {
        // remove from queue:
        {
          boost::unique_lock<boost::mutex> lock(mutex_);
          
          if (closed_ || data_.empty())
          {
            return false;
          }
          
          data = data_.front();
          data_.pop_front();
          size_--;
        }
        
        cond_.notify_one();
        return true;
      }
      catch (...)
      {}
      
      return false;
    }
    
    // peek at the head of the Queue:
    bool peekHead(TData & data) const
    {
      try
      {
        boost::unique_lock<boost::mutex> lock(mutex_);
        while (!closed_ && data_.empty())
        {
          cond_.wait(lock);
        }
        
        if (data_.empty())
        {
          return false;
        }
        
        data = data_.front();
        return true;
      }
      catch (...)
      {}
      
      return false;
    }
    
    // peek at the head of the Queue:
    bool peekTail(TData & data) const
    {
      try
      {
        boost::unique_lock<boost::mutex> lock(mutex_);
        while (!closed_ && data_.empty())
        {
          cond_.wait(lock);
        }
        
        if (data_.empty())
        {
          return false;
        }
        
        data = data_.back();
        return true;
      }
      catch (...)
      {}
      
      return false;
    }

  protected:
    mutable boost::mutex mutex_;
    mutable boost::condition_variable cond_;
    bool closed_;
    std::list<TData> data_;
    std::size_t size_;
    const std::size_t maxSize_;
  };
  
}


#endif // YAE_QUEUE_H_