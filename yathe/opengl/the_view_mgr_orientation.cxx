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


// File         : the_view_mgr_orientation.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun May 30 19:58:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Predefined view orientations.

// system includes:
#include <assert.h>

// local includes:
#include "opengl/the_view_mgr_orientation.hxx"
#include "io/io_base.hxx"


//----------------------------------------------------------------
// the_view_name
// 
const char *
the_view_orientation_text(const the_view_mgr_orientation_t & orientation)
{
  switch (orientation)
  {
    case THE_ISOMETRIC_VIEW_E:	return "Isometric";
    case THE_TOP_VIEW_E:	return "Top";
    case THE_BOTTOM_VIEW_E:	return "Bottom";
    case THE_LEFT_VIEW_E:	return "Left";
    case THE_RIGHT_VIEW_E:	return "Right";
    case THE_FRONT_VIEW_E:	return "Front";
    case THE_BACK_VIEW_E:	return "Back";
    case THE_XY_VIEW_E:		return "XY";
  }
  
  assert(false);
  return NULL;
}

//----------------------------------------------------------------
// THE_ORIENTATION_LF
// 
// this array should be accessed via the_view_type_t:
// 
const v3x1_t
THE_ORIENTATION_LF[] =
{
  !v3x1_t( 1.0,  1.0,  1.0), // isometric
  v3x1_t(  0.0,  0.0,  1.0), // top
  v3x1_t(  0.0,  0.0, -1.0), // bottom
  v3x1_t(  0.0,  1.0,  0.0), // left
  v3x1_t(  0.0, -1.0,  0.0), // right
  v3x1_t(  1.0,  0.0,  0.0), // front
  v3x1_t( -1.0,  0.0,  0.0), // back
  v3x1_t(  0.0,  0.0,  1.0)  // bottom
};

//----------------------------------------------------------------
// THE_ORIENTATION_UP
// 
// this array should be accessed via the_view_type_t:
// 
const v3x1_t
THE_ORIENTATION_UP[] =
{
  !v3x1_t(-1.0, -1.0, 2.0), // isometric
  v3x1_t(  0.0, -1.0, 0.0), // top
  v3x1_t(  0.0, -1.0, 0.0), // bottom
  v3x1_t(  0.0,  0.0, 1.0), // left
  v3x1_t(  0.0,  0.0, 1.0), // right
  v3x1_t(  0.0,  0.0, 1.0), // front
  v3x1_t(  0.0,  0.0, 1.0), // back
  v3x1_t(  0.0,  1.0, 0.0)  // xy
};

//----------------------------------------------------------------
// save
// 
bool
save(std::ostream & stream, the_view_mgr_orientation_t o)
{
  int i = (int)o;
  return ::save(stream, i);
}

//----------------------------------------------------------------
// load
// 
bool
load(std::istream & stream, the_view_mgr_orientation_t & o)
{
  int i = 0;
  bool ok = ::load(stream, i);
  o = (the_view_mgr_orientation_t)i;
  return ok;
}
