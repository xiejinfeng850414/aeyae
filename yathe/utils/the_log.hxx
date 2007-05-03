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


// File         : the_log.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Mar 23 10:34:12 MDT 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A text log object -- behaves almost like a std::ostream.

#ifndef THE_LOG_HXX_
#define THE_LOG_HXX_

// system includes:
#include <iosfwd>
#include <iostream>
#include <iomanip>
#include <sstream>


//----------------------------------------------------------------
// the_log_t
// 
class the_log_t
{
public:
  virtual ~the_log_t() {}
  
  virtual the_log_t &
  operator << (std::ostream & (*f)(std::ostream &))
  {
    f(line_);
    return *this;
  }
  
  template <typename data_t>
  the_log_t &
  operator << (const data_t & data)
  {
    line_ << data;
    return *this;
  }
  
  inline int precision()
  { return line_.precision(); }
  
  inline int precision(int n)
  { return line_.precision(n); }
  
  inline std::ios::fmtflags flags() const
  { return line_.flags(); }
  
  inline std::ios::fmtflags flags(std::ios::fmtflags fmt)
  { return line_.flags(fmt); }
  
  inline void setf(std::ios::fmtflags fmt)
  { line_.setf(fmt); }
  
  inline void setf(std::ios::fmtflags fmt, std::ios::fmtflags msk)
  { line_.setf(fmt, msk); }
  
  inline void unsetf(std::ios::fmtflags fmt)
  { line_.unsetf(fmt); }
  
  inline void copyfmt(std::ostream & ostm)
  { line_.copyfmt(ostm); }
  
  std::ostringstream line_;
};


//----------------------------------------------------------------
// the_null_log_t
//
class the_null_log_t : public the_log_t
{
public:
  // virtual:
  the_log_t & operator << (std::ostream & (*)(std::ostream &))
  { return *this; }
  
  template <typename data_t>
  the_log_t & operator << (const data_t &)
  { return *this; }
};

//----------------------------------------------------------------
// the_stream_log_t
//
class the_stream_log_t : public the_log_t
{
public:
  the_stream_log_t(std::ostream & ostm):
    ostm_(ostm)
  {}
  
  // virtual:
  the_log_t & operator << (std::ostream & (*f)(std::ostream &))
  {
    the_log_t::operator << (f);
    ostm_ << the_log_t::line_.str();
    the_log_t::line_.str("");
    return *this;
  }
  
  template <typename data_t>
  the_log_t & operator << (const data_t & data)
  { return the_log_t::operator << (data); }
  
  std::ostream & ostm_;
};


//----------------------------------------------------------------
// the_text_log_t
// 
class the_text_log_t : public the_log_t
{
public:
  // virtual:
  the_log_t & operator << (std::ostream & (*f)(std::ostream &))
  {
    the_log_t::operator << (f);
    text_ += the_log_t::line_.str();
    the_log_t::line_.str("");
    return *this;
  }
  
  template <typename data_t>
  the_log_t & operator << (const data_t & data)
  { return the_log_t::operator << (data); }
  
  inline std::string text()
  { return text_; }
  
  std::string text_;
};

//----------------------------------------------------------------
// null_log
// 
extern the_null_log_t * null_log();

//----------------------------------------------------------------
// cerr_log
// 
extern the_stream_log_t * cerr_log();


#endif // THE_LOG_HXX_
