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

#ifndef THE_ASCII_FONT_HXX_
#define THE_ASCII_FONT_HXX_

// local includes:
#include "opengl/the_font.hxx"


//----------------------------------------------------------------
// the_ascii_font_t
// 
class the_ascii_font_t : public the_font_t
{
public:
  the_ascii_font_t(): the_font_t("ascii") {}
  virtual ~the_ascii_font_t() {}

  // virtual:
  unsigned int width() const  { return 5; }
  unsigned int height() const { return 10; }

  // virtual:
  float x_origin() const { return 0.0; }
  float y_origin() const { return 2.0; }

  // virtual:
  float x_step() const { return 6.0; }
  float y_step() const { return 0.0; }

  // virtual:
  unsigned char * bitmap(const unsigned int & id) const
  { return font_[id]; }

  unsigned char * bitmap_mask(const unsigned int & id) const
  { return mask_[id]; }

private:
  static unsigned char font_[128][10];
  static unsigned char mask_[128][12];
};

//----------------------------------------------------------------
// THE_ASCII_FONT
// 
// a single global instance of the font:
// 
extern the_ascii_font_t THE_ASCII_FONT;


#endif // THE_ASCII_FONT_HXX_
