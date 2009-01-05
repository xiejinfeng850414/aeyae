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


// File         : the_rational_bezier.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Oct 25 16:08:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : A rational Bezier curve.

#ifndef THE_RATIONAL_BEZIER_HXX_
#define THE_RATIONAL_BEZIER_HXX_

// local includes:
#include "doc/the_primitive.hxx"
#include "doc/the_reference.hxx"
#include "geom/the_curve.hxx"
#include "math/v3x1p3x1.hxx"
#include "opengl/the_disp_list.hxx"
#include "sel/the_pick_filter.hxx"

// system includes:
#include <vector>
#include <list>

// forward declarations:
class the_polyline_t;


//----------------------------------------------------------------
// the_rational_bezier_geom_t
// 
class the_rational_bezier_geom_t : public the_curve_geom_t
{
public:
  void reset(const std::vector<p3x1_t> & pts,
	     const std::vector<float> & wts);
  
  // virtual: evaluate this curve at a given parameter:
  bool eval(const float & t,
	    p3x1_t & P0, // position
	    v3x1_t & P1, // first derivative
	    v3x1_t & P2, // second derivative
	    float & curvature,
	    float & torsion) const;
  
  // virtual:
  unsigned int
  init_slope_signs(const the_curve_deviation_t & deviation,
		   const unsigned int & steps_per_segment,
		   std::list<the_slope_sign_t> & slope_signs,
		   float & s0,
		   float & s1) const;
  
  // virtual: calculate the bounding box of the rational bezier curve:
  void calc_bbox(the_bbox_t & bbox) const;
  
  // virtual:
  float t_min() const { return 0.0; }
  float t_max() const { return 1.0; }
  
private:
  std::vector<p3x1_t> pt_;
  std::vector<float> wt_;
};


//----------------------------------------------------------------
// the_rational_bezier_t
// 
// Rational Bezier curve
// 
class the_rational_bezier_t : public the_curve_t
{
public:
  // virtual:
  the_primitive_t * clone() const
  { return new the_rational_bezier_t(*this); }
  
  // virtual:
  const char * name() const
  { return "the_rational_bezier_t"; }
  
  // virtual: rebuild the display list according to the current point list:
  bool regenerate();
  
  // accessor to the polyline that supports (defines) this curve:
  the_polyline_t * polyline() const;
  
  // virtual: For debugging, dumps all segments
  void dump(ostream & strm, unsigned int indent = 0) const;
  
  // virtual:
  const the_rational_bezier_geom_t & geom() const
  { return geom_; }
  
private:
  // the actual rational bezier curve:
  the_rational_bezier_geom_t geom_;
};


//----------------------------------------------------------------
// the_rational_bezier_pick_filter_t
// 
class the_rational_bezier_pick_filter_t : public the_pick_filter_t
{
public:
  // virtual:
  bool allow(const the_registry_t * registry, const unsigned int & id) const
  {
    the_rational_bezier_t * rational_bezier =
      registry->elem<the_rational_bezier_t>(id);
    return (rational_bezier != NULL);
  }
};


#endif // THE_RATIONAL_BEZIER_HXX_
