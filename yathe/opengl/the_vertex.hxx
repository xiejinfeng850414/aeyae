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


// File         : the_vertex.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun  6 16:07:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Facet vertex representation class.

#ifndef THE_VERTEX_HXX_
#define THE_VERTEX_HXX_

// local includes:
#include "math/v3x1p3x1.hxx"


//----------------------------------------------------------------
// the_interpolation
// 
template <class T>
inline void
the_interpolation(const T & a, const T & b, const float & t_ab, T & result)
{
  result = a + t_ab * (b - a);
}


//----------------------------------------------------------------
// the_vertex_t
// 
class the_vertex_t
{
public:
  the_vertex_t(const p3x1_t & vertex = p3x1_t(FLT_MAX,
					      FLT_MAX,
					      FLT_MAX),
	       const v3x1_t & vertex_normal = v3x1_t(FLT_MAX,
						     FLT_MAX,
						     FLT_MAX),
	       const p2x1_t & vertex_texture_point = p2x1_t(FLT_MAX,
							    FLT_MAX)):
    vx(vertex),
    vn(vertex_normal),
    vt(vertex_texture_point)
  {}
  
  the_vertex_t(const float * vertex,
	       const float * vertex_normal):
    vx(vertex),
    vn(vertex_normal),
    vt(FLT_MAX, FLT_MAX)
  {}
  
  the_vertex_t(const float * vertex,
	       const float * vertex_normal,
	       const float * vertex_texture_point):
    vx(vertex),
    vn(vertex_normal),
    vt(vertex_texture_point)
  {}
  
  static void
  interpolate(const the_vertex_t & a,
	      const the_vertex_t & b,
	      const float &       t_ab,
	      the_vertex_t &       result)
  {
    the_interpolation(a.vx, b.vx, t_ab, result.vx);
    the_interpolation(a.vn, b.vn, t_ab, result.vn);
    the_interpolation(a.vt, b.vt, t_ab, result.vt);
    result.vn.normalize();
  }
  
  inline bool operator == (const the_vertex_t & v) const
  { return vx == v.vx && vn == v.vn && vt == v.vt; }
  
  void dump(std::ostream & ostr) const
  {
    ostr << "vx " << vx[0] << ' ' << vx[1] << ' ' << vx[2];
    
    if (vn[0] != FLT_MAX || vn[1] != FLT_MAX || vn[3] != FLT_MAX)
    {
      ostr << ", vn " << vn[0] << ' ' << vn[1] << ' ' << vn[2];
    }
    
    if (vt[0] != FLT_MAX || vt[1] != FLT_MAX)
    {
      ostr << ", vt " << vt[0] << ' ' << vt[1];
    }
    
    ostr << endl;
  }
  
  p3x1_t vx; // vertex coordinate
  v3x1_t vn; // vertex normal
  p2x1_t vt; // texture coordinate
};

//----------------------------------------------------------------
// operator <<
// 
inline std::ostream &
operator << (std::ostream & ostr, const the_vertex_t & v)
{
  v.dump(ostr);
  return ostr;
}


#endif // THE_VERTEX_HXX_
