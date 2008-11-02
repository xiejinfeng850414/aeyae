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


// File         : image_tile_generator.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 24 16:54:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : a helper class for splitting an image into a set
//                of tiles that may be used as OpenGL textures.

#ifndef IMAGE_TILE_GENERATOR_HXX_
#define IMAGE_TILE_GENERATOR_HXX_

// local includes:
#include "image/image_tile.hxx"
#include "image/texture_data.hxx"
#include "utils/the_dynamic_array.hxx"
#include "math/v3x1p3x1.hxx"
#include "math/the_aa_bbox.hxx"

// system includes:
#include <string.h>
#include <math.h>


//----------------------------------------------------------------
// pixel_converter_t
// 
class pixel_converter_t
{
public:
  virtual ~pixel_converter_t() {}
  
  // it's up to the subclasses to implement the actual conversion:
  virtual void operator() (unsigned char * dst_addr,
			   const unsigned char * src_addr,
			   const size_t & src_bytes_to_read) const = 0;
};

//----------------------------------------------------------------
// copy_pixels_t
//
// A trivial pixel converter
// 
class copy_pixels_t : public pixel_converter_t
{
public:
  // virtual:
  void operator() (unsigned char * dst_addr,
		   const unsigned char * src_addr,
		   const size_t & src_bytes_to_read) const
  { memcpy(dst_addr, src_addr, src_bytes_to_read); }
};


//----------------------------------------------------------------
// image_tile_generator_t
// 
// function execution order:
// 
// 1. layout
// 2. convert_and_pad
// 3. make_tiles
// 3. flip (optional)
// 
class image_tile_generator_t
{
public:
  // default constructor initializes everything to zero:
  image_tile_generator_t();
  ~image_tile_generator_t();
  
  // calculate the image padding and generate the tile layout:
  void layout(const unsigned int w,
	      const unsigned int h,
	      const double origin_x = 0,
	      const double origin_y = 0,
	      const double spacing_x = 1,
	      const double spacing_y = 1);
  
  // allocate the padded image buffer:
  void allocate(const unsigned int & bytes_per_pixel);
  
  // accessors to one un-padded scan line of the padded image:
  inline const unsigned char * scanline(int y) const
  { return buffer_->data() + ((1 + y) * w_pad_ + 1) * bytes_per_pixel_; }
  
  inline unsigned char * scanline(int y)
  { return buffer_->data() + ((1 + y) * w_pad_ + 1) * bytes_per_pixel_; }
  
  inline unsigned char * pixel(int x, int y)
  { return buffer_->data() + ((1 + y) * w_pad_ + 1 + x) * bytes_per_pixel_; }
  
  // this copies the padding data out of the un-padded image
  // stored in the padded image buffer:
  void pad();
  
  // generate a padded image, each pixel is generated by the converter:
  void convert_and_pad(const unsigned char * src,
		       const unsigned int & src_alignment,
		       const unsigned int & src_bytes_per_pixel,
		       const unsigned int & dst_bytes_per_pixel,
		       const pixel_converter_t & convert);
  
  // setup the tiles for the entire unpadded image:
  inline void make_tiles(const GLenum & data_type,
			 const GLenum & format_internal,
			 const GLenum & format,
			 const unsigned int max_texture)
  {
    return make_tiles(data_type,
		      format_internal,
		      format,
		      max_texture,
		      origin_x_,
		      origin_y_,
		      origin_x_ + spacing_x_ * double(w_),
		      origin_y_ + spacing_y_ * double(h_));
  }
  
  // setup the tiles for a given region within the unpadded image:
  void make_tiles(const GLenum & data_type,
		  const GLenum & format_internal,
		  const GLenum & format,
		  const unsigned int max_texture,
		  double min_x,
		  double min_y,
		  double max_x,
		  double max_y);
  
  // change the origin of the image, update the tiles accordingly:
  void set_origin(double ox, double oy);
  
  // flip the data left to right:
  void flip();
  
  // given non-integer coordinates, find the left-aligned pixel coordinates:
  inline bool
  get_pixel_coords(// physical coordinates:
		   double x,
		   double y,
		   
		   // computed pixel coordinates (left-aligned)
		   int & ix,
		   int & iy) const
  {
    if (x + spacing_x_ < origin_x_ || x >= bbox_.max_.x() ||
	y + spacing_y_ < origin_y_ || y >= bbox_.max_.y())
    {
      return false;
    }
    
    ix = int(floor((x - origin_x_) / spacing_x_));
    iy = int(floor((y - origin_y_) / spacing_y_));
    
    // sanity check:
    assert(ix >= -1 && ix < int(w_) && iy >= -1 && iy < int(h_));
    
    return true;
  }
  
  // given non-integer coordinates, find the left-aligned pixel coordinates
  // and contributions weights from right-aligned neighboring pixels:
  inline bool
  get_pixel_coords(// physical coordinates:
		   double x,
		   double y,
		   
		   // computed pixel coordinates (left-aligned)
		   int & ix,
		   int & iy,
		   
		   // weights of the contributions from the right-aligned
		   // neighboring pixels (used for linear interpolation
		   // between the neighboring pixels -- anti-aliasing):
		   double & u1,
		   double & v1) const
  {
    if (!get_pixel_coords(x, y, ix, iy))
    {
      return false;
    }
    
    double rx = x - double(ix) * spacing_x_;
    double ry = y - double(iy) * spacing_y_;
    
    u1 = rx / spacing_x_;
    v1 = ry / spacing_y_;
    
    return true;
  }
  
  // given non-integer coordinates, find the contributing pixels
  // and their respective contribution weights, return false if
  // the requested coordinates fall outside the image buffer:
  bool evaluate(double x,
		double y,
		const unsigned char * pixel[4],
		double contributions[4]) const;
  
  inline bool valid_pixel_coords(unsigned int ix, unsigned int iy) const
  { return ix < w_ && iy < h_; }
  
  // width, height of the original image:
  unsigned int w_;
  unsigned int h_;
  
  // width, height of the padded image:
  unsigned int w_pad_;
  unsigned int h_pad_;
  
  // additional padding (included in the padded image dimensions) required
  // for proper power-of-two texture tiling of the original image:
  unsigned int w_odd_;
  unsigned int h_odd_;
  
  // image origin:
  double origin_x_;
  double origin_y_;
  
  // pixel spacing:
  double spacing_x_;
  double spacing_y_;
  
  // bounding box of the original image:
  the_aa_bbox_t bbox_;
  
  // bytes per pixel:
  size_t bytes_per_pixel_;
  
  // the padded image buffer:
  boost::shared_ptr<texture_data_t> buffer_;
  
  // tiles cut from the padded image:
  std::vector<image_tile_t> tiles_;
};

//----------------------------------------------------------------
// interpolate_luminance
// 
// NOTE: this function assumes that luminance is
// passed in already initialized to 0, and that each of the
// four pixels consists of 1 byte -- luminance:
// 
extern void
interpolate_luminance(double & luminance,
		      const unsigned char * pixel[4],
		      const double weight[4]);

//----------------------------------------------------------------
// interpolate_luminance_alpha
// 
// NOTE: this function assumes that luminance and alpha are
// passed in already initialized to 0, and that each of the
// four pixels consists of 2 bytes -- luminance and alpha:
// 
extern void
interpolate_luminance_alpha(double & luminance,
			    double & alpha,
			    const unsigned char * pixel[4],
			    const double weight[4]);


//----------------------------------------------------------------
// encode_la_fn_t
// 
typedef void(*encode_la_fn_t)(const unsigned char &,
			      const unsigned char &,
			      unsigned char &);

//----------------------------------------------------------------
// decode_la_fn_t
// 
typedef void(*decode_la_fn_t)(const unsigned char &,
			      unsigned char &,
			      unsigned char &);

//----------------------------------------------------------------
// encode_la
// 
extern encode_la_fn_t encode_la;

//----------------------------------------------------------------
// decode_la
// 
extern decode_la_fn_t decode_la;

//----------------------------------------------------------------
// encode_la_v1
//
// Lossy compression of luminance and alpha into 1 byte.
// 
extern void
encode_la_v1(const unsigned char & l,
	     const unsigned char & a,
	     unsigned char & la);

//----------------------------------------------------------------
// encode_la_v2
// 
extern void
encode_la_v2(const unsigned char & l,
	     const unsigned char & a,
	     unsigned char & la);

//----------------------------------------------------------------
// decode_la_v1
//
// Decompress luminance and alpha.
// 
extern void
decode_la_v1(const unsigned char & la,
	     unsigned char & l,
	     unsigned char & a);

//----------------------------------------------------------------
// decode_la_v2
// 
extern void
decode_la_v2(const unsigned char & la,
	     unsigned char & l,
	     unsigned char & a);

//----------------------------------------------------------------
// interpolate_luminance_alpha
// 
// NOTE: this function assumes that luminance and alpha are
// passed in already initialized to 0, and that each of the
// four pixels consists of 1 byte encoded using encode_la:
// 
extern void
interpolate_compressed_luminance_alpha(double & luminance,
				       double & alpha,
				       const unsigned char * pixel[4],
				       const double weight[4]);

//----------------------------------------------------------------
// la_interpolator_t
// 
typedef void(*la_interpolator_t)(double & luminance,
				 double & alpha,
				 const unsigned char * pixel[4],
				 const double weight[4]);

//----------------------------------------------------------------
// encode_rgba16
// 
// Lossy compression of RGBA into 16 bits -- 5-5-5-1.
// 
extern void
encode_rgba16(const float * rgba, unsigned char * rgba16);

//----------------------------------------------------------------
// decode_rgba16
// 
extern void
decode_rgba16(const unsigned char * rgba16, float * rgba);


#endif // IMAGE_TILE_GENERATOR_HXX_
