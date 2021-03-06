// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_qt_view.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A Qt based port of the OpenGL view widget.

// system includes:
#include <assert.h>

// local includes:
#include "Qt3/the_qt_view.hxx"
#include "Qt3/the_qt_input_device_event.hxx"
#include "eh/the_input_device_eh.hxx"

// Qt includes:
#include <qwidget.h>
#include <qevent.h>
#include <qbitmap.h>
#include <qimage.h>
#include <qcursor.h>


//----------------------------------------------------------------
// the_qt_view_t::the_qt_view_t
//
the_qt_view_t::the_qt_view_t(QWidget * parent,
			     const char * name,
			     QGLWidget * shared,
			     const the_view_mgr_orientation_t & orientation):
  QGLWidget(parent, name, shared, 0),
  the_view_t(name, orientation)
{
  setName(name);

  if (shared == NULL)
  {
    // FIXME: this may not be necessary:
    setFocus();
  }
  else
  {
    // FIXME: is this redundant?
    setFormat(shared->context()->format());
  }

  setFocusPolicy(QWidget::StrongFocus);
  setBackgroundMode(QWidget::NoBackground);
  setMouseTracking(true);
}

//----------------------------------------------------------------
// the_qt_view_t::initializeGL
//
// QT/OpenGL stuff:
void
the_qt_view_t::initializeGL()
{
  QGLWidget::initializeGL();
  gl_setup();
}

//----------------------------------------------------------------
// the_qt_view_t::resizeGL
//
void
the_qt_view_t::resizeGL(int w, int h)
{
  gl_resize(w, h);
  QGLWidget::resizeGL(w, h);
}

//----------------------------------------------------------------
// the_qt_view_t::paintGL
//
void
the_qt_view_t::paintGL()
{
  gl_paint();
}

//----------------------------------------------------------------
// the_qt_view_t::change_cursor
//
void
the_qt_view_t::change_cursor(const the_cursor_id_t & cursor_id)
{
  the_cursor_t c(cursor_id);
  setCursor(QCursor(QBitmap(c.w_, c.h_, c.icon_),
		    QBitmap(c.w_, c.h_, c.mask_),
		    c.x_,
		    c.y_));
}

//----------------------------------------------------------------
// the_qt_view_t::showEvent
//
void
the_qt_view_t::showEvent(QShowEvent * e)
{
  eh_stack().view_cb(this);
}

//----------------------------------------------------------------
// the_qt_view_t::mousePressEvent
//
void
the_qt_view_t::mousePressEvent(QMouseEvent * e)
{
  if (!eh_stack_->mouse_cb(the_mouse_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::mouseReleaseEvent
//
void
the_qt_view_t::mouseReleaseEvent(QMouseEvent * e)
{
  if (!eh_stack_->mouse_cb(the_mouse_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::mouseDoubleClickEvent
//
void
the_qt_view_t::mouseDoubleClickEvent(QMouseEvent * e)
{
  if (!eh_stack_->mouse_cb(the_mouse_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::mouseMoveEvent
//
void
the_qt_view_t::mouseMoveEvent(QMouseEvent * e)
{
  if (!eh_stack_->mouse_cb(the_mouse_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::wheelEvent
//
void
the_qt_view_t::wheelEvent(QWheelEvent * e)
{
  if (!eh_stack_->wheel_cb(the_wheel_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::keyPressEvent
//
void
the_qt_view_t::keyPressEvent(QKeyEvent * e)
{
  if (!eh_stack_->keybd_cb(the_keybd_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::keyReleaseEvent
//
void
the_qt_view_t::keyReleaseEvent(QKeyEvent * e)
{
  if (!eh_stack_->keybd_cb(the_keybd_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::tabletEvent
//
void
the_qt_view_t::tabletEvent(QTabletEvent * e)
{
  if (eh_stack_->wacom_cb(the_wacom_event(this, e)))
  {
    e->accept();
  }
  else
  {
    e->ignore();
  }
}
