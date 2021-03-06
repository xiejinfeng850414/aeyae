// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_qt_view.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A Qt based port of the OpenGL view widget.

#ifndef THE_QT_VIEW_HXX_
#define THE_QT_VIEW_HXX_

// local includes:
#include "opengl/the_view.hxx"

// QT includes:
#include <qgl.h>
#include <qevent.h>


//----------------------------------------------------------------
// the_qt_view_t
//
class the_qt_view_t : public QGLWidget,
		      public the_view_t
{
  Q_OBJECT

public:
  // Constructor for the main view:
  the_qt_view_t(QWidget * parent,
		const char * name,
		QGLWidget * shared = NULL,
		const the_view_mgr_orientation_t & o = THE_ISOMETRIC_VIEW_E);

  // virtual:
  bool is_hidden()
  { return QGLWidget::isHidden(); }

  // virtual:
  void set_focus()
  { QGLWidget::setFocus(); }

  // virtual:
  void refresh()
  {
    QGLWidget::updateGL();
    QGLWidget::doneCurrent();
  }

  // virtual:
  bool gl_context_is_valid() const
  { return QGLWidget::isValid(); }

  // virtual:
  void gl_make_current()
  { QGLWidget::makeCurrent(); }

  // virtual:
  void gl_done_current()
  { QGLWidget::doneCurrent(); }

  // virtual:
  void change_cursor(const the_cursor_id_t & cursor_id);

protected:
  // virtual: QT/OpenGL stuff:
  void initializeGL();
  void resizeGL(int width, int height);
  void paintGL();


  // virtual: Mouse/Keyboard/View events will be passed on to another handler:
  void showEvent(QShowEvent * e);

  void mousePressEvent(QMouseEvent * e);
  void mouseReleaseEvent(QMouseEvent * e);
  void mouseDoubleClickEvent(QMouseEvent * e);
  void mouseMoveEvent(QMouseEvent * e);
  void wheelEvent(QWheelEvent * e);

  void keyPressEvent(QKeyEvent * e);
  void keyReleaseEvent(QKeyEvent * e);

  void tabletEvent(QTabletEvent * e);

protected:
  // disable default constructor:
  the_qt_view_t();
};


#endif // THE_QT_VIEW_HXX_
