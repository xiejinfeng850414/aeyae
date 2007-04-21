// File         : the_dynamic_array.hxx
// Author       : Paul A. Koshevoy
// Created      : Fri Oct 31 17:16:25 MDT 2003
// Copyright    : (C) 2003
// License      : GPL.
// Description  : Implementation of a dynamically resizable array
//                that grows automatically.

#ifndef THE_DYNAMIC_ARRAY_HXX_
#define THE_DYNAMIC_ARRAY_HXX_

// system includes:
#include <algorithm>
#include <iostream>
#include <vector>

// forward declarations:
template<class T> class the_dynamic_array_t;

#undef min
#undef max


//----------------------------------------------------------------
// the_dynamic_array_ref_t
// 
template<class T>
class the_dynamic_array_ref_t
{
public:
  the_dynamic_array_ref_t(the_dynamic_array_t<T> & array,
			  unsigned int index = 0):
    array_(array),
    index_(index)
  {}
  
  inline the_dynamic_array_ref_t<T> & operator << (const T & elem)
  {
    array_[index_++] = elem;
    return *this;
  }
  
private:
  // reference to the array:
  the_dynamic_array_t<T> & array_;
  
  // current index into the array:
  unsigned int index_;
};


//----------------------------------------------------------------
// the_dynamic_array_t
// 
template<class T>
class the_dynamic_array_t
{
public:
  the_dynamic_array_t():
    array_(NULL),
    page_size_(16),
    size_(0),
    init_value_()
  {}
  
  the_dynamic_array_t(const unsigned int & init_size):
    array_(NULL),
    page_size_(init_size),
    size_(0),
    init_value_()
  {}
  
  the_dynamic_array_t(const unsigned int & init_size,
		      const unsigned int & page_size,
		      const T & init_value):
    array_(NULL),
    page_size_(page_size),
    size_(0),
    init_value_(init_value)
  {
    resize(init_size);
  }
  
  // copy constructor:
  the_dynamic_array_t(const the_dynamic_array_t<T> & a):
    array_(NULL),
    page_size_(0),
    size_(0),
    init_value_(a.init_value_)
  {
    (*this) = a;
  }
  
  // destructor:
  ~the_dynamic_array_t()
  {
    clear();
  }
  
  // remove all contents of this array:
  void clear()
  {
    unsigned int num = num_pages();
    for (unsigned int i = 0; i < num; i++)
    {
      delete (*array_)[i];
    }
    
    delete array_;
    array_ = NULL;
    
    size_ = 0;
  }
  
  // the assignment operator:
  the_dynamic_array_t<T> & operator = (const the_dynamic_array_t<T> & array)
  {
    clear();
    
    page_size_  = array.page_size_;
    init_value_ = array.init_value_;
    
    resize(array.size_);
    for (unsigned int i = 0; i < size_; i++)
    {
      (*this)[i] = array[i];
    }
    
    return *this;
  }
  
  // resize the array, all contents will be preserved:
  void resize(const unsigned int & new_size)
  {
    // bump the current size value:
    size_ = new_size;
    
    // do nothing if resizing is unnecessary:
    if (size_ <= max_size()) return;
    
    // we'll have to do something about the existing data:
    unsigned int old_num_pages = num_pages();
    unsigned int new_num_pages =
      std::max((unsigned int)(2 * old_num_pages),
	       (unsigned int)(1 + size_ / page_size_));
    
    // create a new array:
    std::vector< std::vector<T> * > * new_array =
      new std::vector< std::vector<T> * >(new_num_pages);
    
    // shallow-copy the old content:
    for (unsigned int i = 0; i < old_num_pages; i++)
    {
      (*new_array)[i] = (*array_)[i];
    }
    
    // initialize the new pages:
    for (unsigned int i = old_num_pages; i < new_num_pages; i++)
    {
      (*new_array)[i] = new std::vector<T>(page_size_);
      for (unsigned int j = 0; j < page_size_; j++)
      {
	(*(*new_array)[i])[j] = init_value_;
      }
    }
    
    // get rid of the old array:
    delete array_;
    
    // put the new array in place of the old array:
    array_ = new_array;
  }
  
  // the size of this array:
  inline const unsigned int & size() const
  { return size_; }
  
  inline const unsigned int & page_size() const
  { return page_size_; }
  
  // maximum usable size of the array that does not require resizing the array:
  inline unsigned int max_size() const
  { return num_pages() * page_size_; }
  
  // number of pages currently allocated:
  inline unsigned int num_pages() const
  { return (array_ == NULL) ? 0 : array_->size(); }
  
  inline const T * page(const unsigned int & page_index) const
  { return &((*(*array_)[page_index])[0]); }
  
  inline T * page(const unsigned int & page_index)
  { return &((*(*array_)[page_index])[0]); }
  
  // return either first or last index into the array:
  inline unsigned int end_index(bool last) const
  {
    if (last == false) return 0;
    return size_ - 1;
  }
  
  // return either first or last element in the array:
  inline const T & end_elem(bool last) const
  { return elem(end_index(last)); }
  
  inline T & end_elem(bool last)
  { return elem(end_index(last)); }
  
  inline const T & front() const
  { return end_elem(false); }
  
  inline T & front()
  { return end_elem(false); }
  
  inline const T & back() const
  { return end_elem(true); }
  
  inline T & back()
  { return end_elem(true); }
  
  // non-const accessors:
  inline T & elem(const unsigned int i)
  {
    if (i >= size_) resize(i + 1);
    return (*(*array_)[i / page_size_])[i % page_size_];
  }
  
  inline T & operator [] (const unsigned int & i)
  { return elem(i); }
  
  // const accessors:
  inline const T & elem(const unsigned int & i) const
  { return (*(*array_)[i / page_size_])[i % page_size_]; }
  
  inline const T & operator [] (const unsigned int & i) const
  { return elem(i); }
  
  // this is usefull for filling-in the array:
  the_dynamic_array_ref_t<T> operator << (const T & elem)
  {
    (*this)[0] = elem;
    return the_dynamic_array_ref_t<T>(*this, 1);
  }
  
  // grow the array by one and insert a new element at the tail:
  inline void push_back(const T & elem)
  { (*this)[size_] = elem; }
  
  inline void append(const T & elem)
  { push_back(elem); }
  
  // return the index of the first occurrence of a given element in the array:
  unsigned int index_of(const T & element) const
  {
    for (unsigned int i = 0; i < size_; i++)
    {
      if (!(elem(i) == element)) continue;
      return i;
    }
    
    return ~0u;
  }
  
  // check whether this array contains a given element:
  inline bool has(const T & element) const
  { return index_of(element) != ~0u; }
  
  // remove an element from the array:
  bool remove(const T & element)
  {
    unsigned int idx = index_of(element);
    if (idx == ~0u) return false;
    
    for (unsigned int i = idx + 1; i < size_; i++)
    {
      elem(i - 1) = elem(i);
    }
    
    size_--;
    return true;
  }
  
  void assign(const unsigned int & size, const T & element)
  {
    resize(size);
    for (unsigned int i = 0; i < size; i++)
    {
      elem(i) = element;
    }
  }
  
  // for debugging, dumps this list into a stream:
  void dump(std::ostream & strm) const
  {
    strm << "the_dynamic_array_t(" << (void *)this << ") {\n";
    for (unsigned int i = 0; i < size_; i++)
    {
      strm << elem(i) << std::endl;
    }
    strm << '}';
  }
  
protected:
  // an array of pointers to arrays (pages) of data:
  std::vector< std::vector<T> *> * array_;
  
  // page size:
  unsigned int page_size_;
  
  // current array size:
  unsigned int size_;
  
  // init value used when resizing the array:
  T init_value_;
};

//----------------------------------------------------------------
// operator <<
// 
template <class T>
std::ostream &
operator << (std::ostream & s, const the_dynamic_array_t<T> & a)
{
  a.dump(s);
  return s;
}


#endif // THE_DYNAMIC_ARRAY_HXX_
