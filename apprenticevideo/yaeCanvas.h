// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:37:20 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CANVAS_H_
#define YAE_CANVAS_H_

// boost includes:
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

// Qt includes:
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QDropEvent>
#include <QGLWidget>
#include <QTimer>
#include <QList>
#include <QUrl>

// yae includes:
#include <yaeAPI.h>
#include <yaeVideoCanvas.h>
#include <yaeSynchronous.h>


//----------------------------------------------------------------
// yae_to_opengl
// 
// returns number of sample planes supported by OpenGL,
// passes back parameters to use with glTexImage2D
// 
YAE_API unsigned int
yae_to_opengl(yae::TPixelFormatId yaePixelFormat,
              GLint & internalFormat,
              GLenum & format,
              GLenum & dataType,
              GLint & shouldSwapBytes);

namespace yae
{

  //----------------------------------------------------------------
  // Canvas
  // 
  class Canvas : public QGLWidget,
                 public IVideoCanvas
  {
    Q_OBJECT;
    
  public:
    class TPrivate;
    
    Canvas(const QGLFormat & format,
           QWidget * parent = 0,
           const QGLWidget * shareWidget = 0,
           Qt::WindowFlags f = 0);
    ~Canvas();
    
    // initialize private backend rendering object,
    // should not be called prior to initializing GLEW:
    void initializePrivateBackend();

    // call this after opening a movie so that the timeline
    // could properly render elapsed time:
    void initializeTimeline(double timelineDuration,
                            const SharedClock & sharedClock);
    
    // helper:
    void refresh();
    
    // virtual:
    bool render(const TVideoFramePtr & frame);
    
    // helper:
    bool loadFrame(const TVideoFramePtr & frame);

    // use this to override auto-detected aspect ratio:
    void overrideDisplayAspectRatio(double dar);
    
    // use this to crop letterbox pillars and bars:
    void cropFrame(double darCropped);
    
    // accessors to full resolution frame dimensions
    // after overriding display aspect ratio and cropping:
    double imageWidth() const;
    double imageHeight() const;
    
  signals:
    void togglePause();
    void toggleFullScreen();
    void exitFullScreen();
    void urlsFromDropEvent(const QList<QUrl> & urls);

  public slots:
    void hideCursor();
    void wakeScreenSaver();
    
  protected:
    // virtual:
    bool event(QEvent * event);
    void keyPressEvent(QKeyEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
    void mouseDoubleClickEvent(QMouseEvent * event);
    void dragEnterEvent(QDragEnterEvent * e);
    void dropEvent(QDropEvent * e);
    void paintEvent(QPaintEvent * e);
    
    // virtual: Qt/OpenGL stuff:
    void initializeGL();

    // helpers:
    void drawFrame();
    void drawControls(QPainter & p);
    void drawTimebox(QPainter & p, const QRect & bbox, double seconds);
    
    //----------------------------------------------------------------
    // RenderFrameEvent
    // 
    struct RenderFrameEvent : public QEvent
    {
      //----------------------------------------------------------------
      // TPayload
      // 
      struct TPayload
      {
        bool set(const TVideoFramePtr & frame)
        {
          boost::lock_guard<boost::mutex> lock(mutex_);
          bool postThePayload = !frame_;
          frame_ = frame;
          return postThePayload;
        }
        
        void get(TVideoFramePtr & frame)
        {
          boost::lock_guard<boost::mutex> lock(mutex_);
          frame = frame_;
          frame_ = TVideoFramePtr();
        }
        
      private:
        mutable boost::mutex mutex_;
        TVideoFramePtr frame_;
      };
      
      RenderFrameEvent(TPayload & payload):
        QEvent(QEvent::User),
        payload_(payload)
      {}
      
      TPayload & payload_;
    };
    
    RenderFrameEvent::TPayload payload_;
    TPrivate * private_;

    // a single shot timer for hiding the cursor:
    QTimer timerHideCursor_;
    
    // a single shot timer for preventing screen saver:
    QTimer timerScreenSaver_;
    
    // a flag indicating whether the timeline should be visible:
    bool exposeControls_;
    
    // video duration (used to draw the timeline):
    double timelineDuration_;
    
    // this is used to keep track of current playhead position:
    SharedClock sharedClock_;
  };
}


#endif // YAE_CANVAS_H_
