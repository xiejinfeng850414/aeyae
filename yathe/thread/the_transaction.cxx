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


// File         : the_transaction.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Feb 16 09:52:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A thread transaction class.

// system includes:
#include <stddef.h>
#include <assert.h>

// local includes:
#include "thread/the_transaction.hxx"
#include "thread/the_thread_interface.hxx"


//----------------------------------------------------------------
// the_transaction_t::the_transaction_t
// 
the_transaction_t::the_transaction_t():
  state_(PENDING_E),
  notify_cb_(NULL),
  notify_cb_data_(NULL),
  status_cb_(NULL),
  status_cb_data_(NULL)
{}

//----------------------------------------------------------------
// the_transaction_t::~the_transaction_t
// 
the_transaction_t::~the_transaction_t()
{}

//----------------------------------------------------------------
// the_transaction_t::notify
// 
void
the_transaction_t::notify(the_transaction_handler_t * handler,
			  state_t s,
			  const char * message)
{
  set_state(s);
  blab(handler, message);
  
  if (notify_cb_ == NULL)
  {
    handler->handle(this, s);
  }
  else
  {
    notify_cb_(notify_cb_data_, this, s);
  }
}

//----------------------------------------------------------------
// the_transaction_t::blab
// 
void
the_transaction_t::blab(the_transaction_handler_t * handler,
			const char * message)
{
  if (message == NULL) return;
  
  if (status_cb_ == NULL)
  {
    handler->blab(message);
  }
  else
  {
    status_cb_(status_cb_data_, this, message);
  }
}


//----------------------------------------------------------------
// operator <<
// 
std::ostream &
operator << (std::ostream & so, const the_transaction_t::state_t & state)
{
  switch (state)
  {
    case the_transaction_t::PENDING_E:
      so << "pending";
      return so;
      
    case the_transaction_t::SKIPPED_E:
      so << "skipped";
      return so;
      
    case the_transaction_t::STARTED_E:
      so << "started";
      return so;
      
    case the_transaction_t::ABORTED_E:
      so << "aborted";
      return so;
      
    case the_transaction_t::DONE_E:
      so << "done";
      return so;
      
    default:
      so << int(state);
      assert(0);
      return so;
  }
}
