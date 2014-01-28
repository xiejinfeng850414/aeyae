// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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
