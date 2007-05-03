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


// File         : the_qt_input_device_event.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2007/01/05 14:59:00
// Copyright    : (C) 2007
// License      : MIT
// Description  : Qt wrappers for input device events

// local includes:
#include "Qt/the_qt_input_device_event.hxx"
#include "opengl/the_view.hxx"

// Qt includes:
#include <QWidget>
#include <QEvent>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QKeyEvent>


//----------------------------------------------------------------
// the_mouse_event
// 
the_mouse_event_t
the_mouse_event(QWidget * widget, const QMouseEvent * e)
{
  p2x1_t scs_pt(float(e->x()) / float(widget->width()),
		float(e->y()) / float(widget->height()));
  
  int tran = e->type();
  the_mouse_event_t me(dynamic_cast<the_view_t *>(widget),
		       e->button() & Qt::MouseButtonMask,
		       tran,
		       e->modifiers() & Qt::KeyboardModifierMask,
		       tran == QEvent::MouseButtonDblClick,
		       tran == QEvent::MouseMove,
		       scs_pt);
  return me;
}

//----------------------------------------------------------------
// the_wheel_event
// 
the_wheel_event_t
the_wheel_event(QWidget * widget, const QWheelEvent * e)
{
  p2x1_t scs_pt(float(e->x()) / float(widget->width()),
		float(e->y()) / float(widget->height()));
  
  the_wheel_event_t we(dynamic_cast<the_view_t *>(widget),
		       e->buttons() & Qt::MouseButtonMask,
		       e->type(),
		       e->modifiers() & Qt::KeyboardModifierMask,
		       scs_pt,
		       double(e->delta()) / 8.0,
		       e->orientation() == Qt::Vertical);
#if 0
  cerr << "FIXME: wheel: " << we.degrees_rotated_ << endl;
#endif
  
  return we;
}

//----------------------------------------------------------------
// the_keybd_event
// 
the_keybd_event_t
the_keybd_event(QWidget * widget, const QKeyEvent * e)
{
#if 0
  cerr << e->key();
  if (e->key() < 256)
  {
    cerr << "\t\'" << (unsigned char)(e->key()) << '\'' << endl;
  }
  else
  {
    cerr << endl;
  }
#endif
  
  the_keybd_event_t ke(dynamic_cast<the_view_t *>(widget),
		       e->key(),
		       e->type(),
		       e->modifiers() & Qt::KeyboardModifierMask,
		       e->isAutoRepeat());
  return ke;
}

//----------------------------------------------------------------
// the_wacom_event
// 
the_wacom_event_t
the_wacom_event(QWidget * widget, const QTabletEvent * e)
{
  p2x1_t scs_pt(float(e->x()) / float(widget->width()),
		float(e->y()) / float(widget->height()));
  
  p2x1_t tilt(2.0 * (float(e->xTilt() + 60) / 120.0 - 0.5),
	      2.0 * (float(e->yTilt() + 60) / 120.0 - 0.5));
  
  the_tablet_tool_t tool = THE_TABLET_UNKNOWN_E;
  switch (e->pointerType())
  {
    case QTabletEvent::Pen:
      tool = THE_TABLET_PEN_E;
      break;
      
    case QTabletEvent::Eraser:
      tool = THE_TABLET_ERASER_E;
      break;
      
    case QTabletEvent::Cursor:
      tool = THE_TABLET_CURSOR_E;
      break;
      
    default:
      break;
  }
  
  the_wacom_event_t te(dynamic_cast<the_view_t *>(widget),
		       tool,
		       const_cast<QTabletEvent *>(e)->uniqueId(),
		       scs_pt,
		       tilt,
		       e->pressure(),
		       e->tangentialPressure(),
		       e->rotation(),
		       float(e->z()));
  return te;
}
