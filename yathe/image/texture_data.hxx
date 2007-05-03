/*
Copyright 2004-2007 University of Utah

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


// File         : texture_data.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 3 18:27:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : a texture data helper class

#ifndef TEXTURE_DATA_HXX_
#define TEXTURE_DATA_HXX_

// system includes:
#include <stdlib.h>
#include <assert.h>


//----------------------------------------------------------------
// texture_data_t
//
class texture_data_t
{
public:
  texture_data_t(size_t bytes);
  ~texture_data_t();
  
  // size of this buffer (in bytes):
  inline const size_t & size() const
  { return size_; }
  
  // byte accessors:
  inline const unsigned char & operator[] (const size_t & i) const
  {
    assert(i < size_);
    return *(data() + i);
  }
  
  inline unsigned char & operator[] (const size_t & i)
  {
    assert(i < size_);
    return *(data() + i);
  }
  
  // buffer accessors:
  inline const unsigned char * data() const
  { return (const unsigned char *)(data_); }
  
  inline unsigned char * data()
  { return (unsigned char *)(data_); }
  
  inline operator const void * () const
  { return data_; }
  
private:
  texture_data_t(const texture_data_t &);
  texture_data_t & operator = (const texture_data_t &);
  
  void * data_;
  size_t size_;
};


//----------------------------------------------------------------
// const_texture_data_t
// 
class const_texture_data_t
{
public:
  const_texture_data_t(const void * data):
    data_(data)
  {}
  
  inline operator const void * () const
  { return data_; }
  
  inline unsigned char * data()
  { return (unsigned char *)(data_); }
  
  const void * data_;
};


#endif // TEXTURE_DATA_HXX_
