// File         : the_fifo.cxx
// Author       : Paul A. Koshevoy
// Created      : Sat Oct 30 16:20:00 MDT 2004
// Copyright    : (C) 2004
// License      : GPL.
// Description  : 

#ifndef THE_FIFO_HXX_
#define THE_FIFO_HXX_

// system includes:
#include <vector>
#include <assert.h>


//----------------------------------------------------------------
// the_fifo_t
// 
template <class data_t>
class the_fifo_t
{
public:
  the_fifo_t(const unsigned int & size = 0):
    data_(size),
    head_(0),
    size_(0)
  {}
  
  // accessors to the element last added to the fifo (last in):
  inline const data_t & head() const
  { return data_[head_]; }
  
  inline data_t & head()
  { return data_[head_]; }
  
  // accessors to the element first added to the fifo (first in):
  inline const data_t & tail() const
  {
    const unsigned int & cap = capacity();
    return data_[(head_ + size_ - 1) % cap];
  }
  
  inline data_t & tail()
  {
    const unsigned int & cap = capacity();
    return data_[(head_ + size_ - 1) % cap];
  }
  
  // accessors to the fifo queue elements (0 is most recently added):
  inline const data_t & operator [] (const unsigned int & i) const
  {
    const unsigned int & cap = capacity();
    assert(i < size_);
    return data_[(head_ + i) % cap];
  }
  
  inline data_t & operator [] (const unsigned int & i)
  {
    const unsigned int & cap = capacity();
    assert(i < size_);
    return data_[(head_ + i) % cap];
  }
  
  // shift the fifo queue by one (most recent becomes 2nd most recent):
  inline void shift()
  {
    const unsigned int & cap = capacity();
    head_ = (head_ + (cap - 1)) % cap;
    size_ = (size_ + 1) - (size_ + 1) / (cap + 1);
  }
  
  // current number of element in the queue:
  inline const unsigned int & size() const
  { return size_; }
  
  // for debugging, dumps this list into a stream:
  void dump(std::ostream & strm, unsigned int indent = 0) const
  {
    strm << "the_fifo_t<data_t>(" << (void *)this << ") {\n"
	 << "head_ = " << head_ << std::endl
	 << "size_ = " << size_ << std::endl
	 << "data_ = " << std::endl
	 << data_ << std::endl
	 << '}' << std::endl;
  }
  
private:
  // the maximum number of elements this fifo queue can store:
  inline const unsigned int capacity() const
  { return data_.size(); }
  
  // the fifo elements:
  std::vector<data_t> data_;
  
  // index of the most recently added queue element (last in):
  unsigned int head_;
  
  // at most this will be equal to the maximum capacity of the queue:
  unsigned int size_;
};

//----------------------------------------------------------------
// operator <<
// 
template <class data_t>
std::ostream &
operator << (std::ostream & s, const the_fifo_t<data_t> & fifo)
{
  fifo.dump(s);
  return s;
}


#endif // THE_FIFO_HXX_
