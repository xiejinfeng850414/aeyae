// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun May  1 13:23:52 MDT 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>

// Qt includes:
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QPainter>
#include <QColor>
#include <QBrush>
#include <QPen>

// yae includes:
#include <yaePlaybackControls.h>


namespace yae
{

  //----------------------------------------------------------------
  // Marker::Marker
  // 
  Marker::Marker():
    position_(0.0),
    positionAnchor_(0.0)
  {
    hotspot_[0] = 0;
    hotspot_[1] = 0;
  }
  
  //----------------------------------------------------------------
  // Marker::overlaps
  // 
  bool
  Marker::overlaps(const QPoint & coords,
                   
                   // these parameters are used to derive current
                   // marker position:
                   const int & xOrigin,
                   const int & yOrigin,
                   const int & unitLength) const
  {
    int x = coords.x() - (xOrigin +
                          int(0.5 + unitLength * position_) -
                          hotspot_[0]);
    int y = coords.y() - (yOrigin - hotspot_[1]);
    
    QRect bbox = image_.rect();
    if (!bbox.contains(x, y))
    {
      return false;
    }
    
    QRgb rgba = image_.pixel(x, y);
    int alpha = qAlpha(rgba);
    
    return alpha > 0;
  }
  
  //----------------------------------------------------------------
  // Marker::setAnchor
  // 
  void
  Marker::setAnchor()
  {
    positionAnchor_ = position_;
  }
  
  
  //----------------------------------------------------------------
  // TimelineControls::TimelineControls
  // 
  TimelineControls::TimelineControls(QWidget * parent, Qt::WindowFlags f):
    QWidget(parent, f),
    reader_(NULL),
    timelineStart_(0.0),
    timelineDuration_(1.0),
    timerRefreshTimeline_(this)
  {
    padding_ = 16;
    lineWidth_ = 3;
    
    setFixedHeight(padding_ * 2 + lineWidth_);
    setMinimumWidth(padding_ * 2 + 64);
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    // load graphics for direct manipulation handles:
    markerTimeIn_.image_ = QImage(":/images/timeIn.png");
    markerTimeOut_.image_ = QImage(":/images/timeOut.png");
    markerPlayhead_.image_ = QImage(":/images/playHead.png");
    
    // setup hotspots:
    markerTimeIn_.hotspot_[0] = markerTimeIn_.image_.width() - 1;
    markerTimeIn_.hotspot_[1] = 12;
    
    markerTimeOut_.hotspot_[0] = 0;
    markerTimeOut_.hotspot_[1] = 12;
    
    markerPlayhead_.hotspot_[0] = markerPlayhead_.image_.width() / 2;
    markerPlayhead_.hotspot_[1] = 8;
    
    // current state of playback controls:
    currentState_ = TimelineControls::kIdle;
    
    timerRefreshTimeline_.setSingleShot(false);
    timerRefreshTimeline_.setInterval(100);
    
    bool ok = true;
    ok = connect(&timerRefreshTimeline_, SIGNAL(timeout()),
                 this, SLOT(refreshTimeline()));
    YAE_ASSERT(ok);
  }
  
  //----------------------------------------------------------------
  // TimelineControls::~TimelineControls
  // 
  TimelineControls::~TimelineControls()
  {}

  //----------------------------------------------------------------
  // TimelineControls::reset
  // 
  void
  TimelineControls::reset(const SharedClock & sharedClock, IReader * reader)
  {
    sharedClock_ = sharedClock;
    
    TTime start;
    TTime duration;
    if (!reader->getAudioDuration(start, duration))
    {
      reader->getVideoDuration(start, duration);
    }
    
    timelineStart_ = start.toSeconds();
    timelineDuration_ = duration.toSeconds();
    
    markerTimeIn_.position_ = 0.1;
    markerTimeIn_.setAnchor();
    
    markerTimeOut_.position_ = 0.8;
    markerTimeOut_.setAnchor();
    
    timerRefreshTimeline_.start();
  }
  
  //----------------------------------------------------------------
  // TimelineControls::setInPoint
  // 
  void
  TimelineControls::setInPoint()
  {
    if (currentState_ == kIdle)
    {
      markerTimeIn_.position_ = markerPlayhead_.position_;
      markerTimeOut_.position_ = std::max(markerTimeIn_.position_,
                                          markerTimeOut_.position_);
      
      refreshTimeline();
    }
  }
  
  //----------------------------------------------------------------
  // TimelineControls::setOutPoint
  // 
  void
  TimelineControls::setOutPoint()
  {
    if (currentState_ == kIdle)
    {
      markerTimeOut_.position_ = markerPlayhead_.position_;
      markerTimeIn_.position_ = std::min(markerTimeIn_.position_,
                                         markerTimeOut_.position_);
      
      refreshTimeline();
    }
  }
  
  //----------------------------------------------------------------
  // TimelineControls::refreshTimeline
  // 
  void
  TimelineControls::refreshTimeline()
  {
    TTime lastUpdate; 	 
    double playheadPosition = 0.0; 	 
    if (sharedClock_.getCurrentTime(lastUpdate, playheadPosition)) 	 
    {
      double t = lastUpdate.toSeconds();
      t -= timelineStart_;
      markerPlayhead_.position_ = t / timelineDuration_;
      markerPlayhead_.setAnchor();
    }
    
    update();
  }
  
  //----------------------------------------------------------------
  // TimelineControls::paintEvent
  // 
  void
  TimelineControls::paintEvent(QPaintEvent * e)
  {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    int xOrigin = 0;
    int yOriginInOut = 0;
    int yOriginPlayhead = 0;
    int unitLength = 0;
    getMarkerCSys(xOrigin, yOriginInOut, yOriginPlayhead, unitLength);
    
    int inExt = int(0.5 + double(unitLength) * markerTimeIn_.position_);
    int outExt = int(0.5 + double(unitLength) * markerTimeOut_.position_);
    int playExt = int(0.5 + double(unitLength) * markerPlayhead_.position_);
    
    p.setPen(Qt::NoPen);
    
    p.setBrush(QColor(0x80, 0x80, 0x80));
    p.drawRect(xOrigin,
               yOriginPlayhead,
               inExt,
               lineWidth_);
    
    p.setBrush(QColor(0x40, 0x80, 0xff));
    p.drawRect(xOrigin + inExt,
               yOriginPlayhead,
               outExt - inExt,
               lineWidth_);
  
    p.setBrush(QColor(0x80, 0x80, 0x80));
    p.drawRect(xOrigin + outExt,
               yOriginPlayhead,
               unitLength - outExt,
               lineWidth_);
    
    p.drawImage(xOrigin + inExt - markerTimeIn_.hotspot_[0],
                yOriginInOut - markerTimeIn_.hotspot_[1],
                markerTimeIn_.image_);
    
    p.drawImage(xOrigin + outExt - markerTimeOut_.hotspot_[0],
                yOriginInOut - markerTimeOut_.hotspot_[1],
                markerTimeOut_.image_);
    
    p.drawImage(xOrigin + playExt - markerPlayhead_.hotspot_[0],
                yOriginPlayhead - markerPlayhead_.hotspot_[1],
                markerPlayhead_.image_);
  }

  //----------------------------------------------------------------
  // TimelineControls::wheelEvent
  // 
  void
  TimelineControls::wheelEvent(QWheelEvent * e)
  {
    // seek back and forth here:
  }
  
  //----------------------------------------------------------------
  // TimelineControls::mousePressEvent
  // 
  void
  TimelineControls::mousePressEvent(QMouseEvent * e)
  {
    QPoint pt = e->pos();
    
    int xOrigin = 0;
    int yOriginInOut = 0;
    int yOriginPlayhead = 0;
    int unitLength = 0;
    getMarkerCSys(xOrigin, yOriginInOut, yOriginPlayhead, unitLength);

    dragStart_ = pt;
    markerTimeIn_.setAnchor();
    markerTimeOut_.setAnchor();
    markerPlayhead_.setAnchor();
    
    if (markerPlayhead_.overlaps(pt, xOrigin, yOriginPlayhead, unitLength))
    {
      // std::cout << "PLAYHEAD" << std::endl;
      currentState_ = kDraggingPlayheadMarker;
      activeMarker_ = &markerPlayhead_;
    }
    else if (markerTimeOut_.overlaps(pt, xOrigin, yOriginInOut, unitLength))
    {
      // std::cout << "OUT POINT" << std::endl;
      currentState_ = kDraggingTimeOutMarker;
      activeMarker_ = &markerTimeOut_;
    }
    else if (markerTimeIn_.overlaps(pt, xOrigin, yOriginInOut, unitLength))
    {
      // std::cout << "IN POINT" << std::endl;
      currentState_ = kDraggingTimeInMarker;
      activeMarker_ = &markerTimeIn_;
    }
    else
    {
      currentState_ = kIdle;
      activeMarker_ = NULL;
    }
  }
  
  //----------------------------------------------------------------
  // TimelineControls::mouseReleaseEvent
  // 
  void
  TimelineControls::mouseReleaseEvent(QMouseEvent * e)
  {
    currentState_ = kIdle;
    activeMarker_ = NULL;
  }
  
  //----------------------------------------------------------------
  // TimelineControls::mouseMoveEvent
  // 
  void
  TimelineControls::mouseMoveEvent(QMouseEvent * e)
  {
    if (currentState_ == kIdle || !activeMarker_)
    {
      return;
    }
    
    QPoint pt = e->pos();
    int dx = pt.x() - dragStart_.x();
    
    int xOrigin = 0;
    int yOriginInOut = 0;
    int yOriginPlayhead = 0;
    int unitLength = 0;
    getMarkerCSys(xOrigin, yOriginInOut, yOriginPlayhead, unitLength);
    
    double t =
      activeMarker_->positionAnchor_ +
      double(dx) / double(unitLength);
    
    t = std::max(0.0, std::min(1.0, t));
    activeMarker_->position_ = t;
    
    if (currentState_ == kDraggingTimeInMarker)
    {
      markerTimeOut_.position_ = std::max(activeMarker_->position_,
                                          markerTimeOut_.positionAnchor_);
    }
    
    if (currentState_ == kDraggingTimeOutMarker)
    {
      markerTimeIn_.position_ = std::min(activeMarker_->position_,
                                         markerTimeIn_.positionAnchor_);
    }
    
    refreshTimeline();
  }

  //----------------------------------------------------------------
  // TimelineControls::keyPressEvent
  // 
  void
  TimelineControls::keyPressEvent(QKeyEvent * e)
  {
    int key = e->key();
    
    if (activeMarker_ &&
        currentState_ != kIdle &&
        key == Qt::Key_Escape)
    {
      activeMarker_->position_ = activeMarker_->positionAnchor_;
      currentState_ = kIdle;
      activeMarker_ = NULL;
      
      refreshTimeline();
    }
    else
    {
      e->ignore();
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::getMarkerCSys
  // 
  void
  TimelineControls::getMarkerCSys(int & xOrigin,
                                  int & yOriginInOut,
                                  int & yOriginPlayhead,
                                  int & unitLength) const
  {
    xOrigin = padding_;
    yOriginInOut = height() - padding_;
    yOriginPlayhead = height() - lineWidth_ - padding_;
    unitLength = width() - padding_ * 2;
  }
  
  
  //----------------------------------------------------------------
  // PlaybackControls::PlaybackControls
  // 
  PlaybackControls::PlaybackControls(QWidget * parent, Qt::WindowFlags f):
    QWidget(parent, f)
  {
    QVBoxLayout * layout = new QVBoxLayout(this);
  }
  
  //----------------------------------------------------------------
  // PlaybackControls::~PlaybackControls
  // 
  PlaybackControls::~PlaybackControls()
  {}

  //----------------------------------------------------------------
  // PlaybackControls::closeEvent
  // 
  void
  PlaybackControls::closeEvent(QCloseEvent * e)
  {}

  //----------------------------------------------------------------
  // PlaybackControls::keyPressEvent
  // 
  void
  PlaybackControls::keyPressEvent(QKeyEvent * e)
  {}
  
  //----------------------------------------------------------------
  // PlaybackControls::mouseDoubleClickEvent
  // 
  void
  PlaybackControls::mouseDoubleClickEvent(QMouseEvent * e)
  {}
  
}
