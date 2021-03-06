// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : image_tile_dl_elem.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 24 16:54:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : a display list element for the image tiles

#ifndef IMAGE_TILE_DL_ELEM_HXX_
#define IMAGE_TILE_DL_ELEM_HXX_


// local includes:
#include "opengl/OpenGLCapabilities.h"
#include "opengl/the_disp_list.hxx"
#include "image/image_tile_generator.hxx"
#include "image/texture_data.hxx"
#include "image/texture.hxx"
#include "math/the_bbox.hxx"
#include "utils/the_utils.hxx"

// Boost includes:
#include <boost/shared_ptr.hpp>

// system includes:
#include <vector>
#include <list>


//----------------------------------------------------------------
// image_tile_dl_elem_t
//
class image_tile_dl_elem_t : public the_dl_elem_t
{
public:
  image_tile_dl_elem_t(const image_tile_generator_t & data,
		       GLenum min_filter = GL_NEAREST,
		       GLenum mag_filter = GL_NEAREST);

  // virtual:
  ~image_tile_dl_elem_t();

  void setup_textures() const;

  // virtual:
  const char * name() const
  { return "image_tile_dl_elem_t"; }

  // virtual:
  void draw() const;

  inline void draw(const the_color_t & color,
		   const bool & use_textures) const
  { draw(draw_tile, this, color, use_textures); }

  //----------------------------------------------------------------
  // draw_tile_cb_t
  //
  typedef void(*draw_tile_cb_t)(const void * data,
				const size_t & tile_index);

  void draw(draw_tile_cb_t draw_tile_cb,
	    const void * draw_tile_cb_data,
	    const the_color_t & color,
	    const bool & use_textures) const;

  void draw(draw_tile_cb_t draw_tile_cb,
	    const void * draw_tile_cb_data,
	    const size_t & tile_index,
	    const the_color_t & color,
	    const bool & use_textures,
	    const GLuint & texture_id,
	    const GLint & mag_filter,
	    const GLint & min_filter) const;

  static void draw_tile(const void * data,
			const size_t & tile_index);

  // virtual:
  void update_bbox(the_bbox_t & bbox) const;

  bool get_texture_info(GLenum & data_type,
                        GLenum & format_internal,
                        GLenum & format) const;

  // the shared display element:
  image_tile_generator_t data_;

  // image bounding box:
  the_bbox_t bbox_;

  // the OpenGL context assiciated with this list:
  mutable the_gl_context_t context_;

  // texture ids:
  mutable std::vector<GLuint> texture_id_;

  // vector of flags indicating whether a tile has
  // a valid texture associated with it:
  mutable std::vector<bool> texture_ok_;

  // a list of regions that must be uploaded onto the graphics card:
  mutable std::list<image_tile_t::quad_t> upload_;

  // min/mag filters:
  GLenum min_filter_;
  GLenum mag_filter_;

private:
  image_tile_dl_elem_t();

#ifndef NDEBUG
  size_t id_;
#endif
};


#endif // IMAGE_TILE_DL_ELEM_HXX_
