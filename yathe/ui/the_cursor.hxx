// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: t -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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


// NOTE: 
//       DO NOT EDIT THIS FILE, IT IS AUTOGENERATED.
//       ALL CHANGES WILL BE LOST UPON REGENERATION.

#ifndef THE_CURSOR_HXX_
#define THE_CURSOR_HXX_


//----------------------------------------------------------------
// the_cursor_id_t
//
typedef enum
{
  THE_ARROW_COPY_CURSOR_E,
  THE_ARROW_MOVEVP_CURSOR_E,
  THE_ARROW_MOVE_CURSOR_E,
  THE_ARROW_SNAP_CURSOR_E,
  THE_ARROW_CURSOR_E,
  THE_DEFAULT_CURSOR_E,
  THE_MOVE_CURSOR_E,
  THE_PAN_CURSOR_E,
  THE_ROTATE_CURSOR_E,
  THE_SNAP_CURSOR_E,
  THE_SPIN_CURSOR_E,
  THE_ZOOM_CURSOR_E
} the_cursor_id_t;


//----------------------------------------------------------------
// the_cursor_t
// 
class the_cursor_t
{
public:
  the_cursor_t(const the_cursor_id_t & cursor_id)
  { setup(cursor_id); }
  
  void setup(const the_cursor_id_t & cursor_id);
  
  const unsigned char * icon_;
  const unsigned char * mask_;
  unsigned int w_;
  unsigned int h_;
  unsigned int x_;
  unsigned int y_;
};


#endif // THE_CURSOR_HXX_
