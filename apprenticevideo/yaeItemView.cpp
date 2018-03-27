// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Dec 18 22:52:30 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++:
#include <iostream>
#include <limits>
#include <list>

// Qt library:
#include <QApplication>
#include <QFontInfo>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QWheelEvent>

// local interfaces:
#include "yaeCanvasRenderer.h"
#include "yaeInputArea.h"
#include "yaeItemFocus.h"
#include "yaeItemRef.h"
#include "yaeItemView.h"
#include "yaeSegment.h"
#include "yaeUtilsQt.h"


//----------------------------------------------------------------
// YAE_DEBUG_ITEM_VIEW_REPAINT
//
#define YAE_DEBUG_ITEM_VIEW_REPAINT 0


namespace yae
{

  //----------------------------------------------------------------
  // PostponeEvent::TPrivate
  //
  struct PostponeEvent::TPrivate
  {
    //----------------------------------------------------------------
    // TPrivate
    //
    TPrivate():
      target_(NULL),
      event_(NULL)
    {}

    //----------------------------------------------------------------
    // ~PostponeEvent
    //
    ~TPrivate()
    {
      delete event_;
    }

    //----------------------------------------------------------------
    // postpone
    //
    void
    postpone(QObject * target, QEvent * event)
    {
      delete event_;
      target_ = target;
      event_ = event;
    }

    //----------------------------------------------------------------
    // onTimeout
    //
    void
    onTimeout()
    {
      qApp->postEvent(target_, event_, Qt::HighEventPriority);
      target_ = NULL;
      event_ = NULL;
    }

    QObject * target_;
    QEvent * event_;
  };

  //----------------------------------------------------------------
  // PostponeEvent::PostponeEvent
  //
  PostponeEvent::PostponeEvent():
    private_(new TPrivate())
  {}

  //----------------------------------------------------------------
  // PostponeEvent::~PostponeEvent
  //
  PostponeEvent::~PostponeEvent()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // PostponeEvent::postpone
  //
  void
  PostponeEvent::postpone(int msec, QObject * target, QEvent * event)
  {
    private_->postpone(target, event);
    QTimer::singleShot(msec, this, SLOT(onTimeout()));
  }

  //----------------------------------------------------------------
  // PostponeEvent::onTimeout
  //
  void
  PostponeEvent::onTimeout()
  {
    private_->onTimeout();
  }


  //----------------------------------------------------------------
  // ItemView::ItemView
  //
  ItemView::ItemView(const char * name):
    Canvas::ILayer(),
    root_(new Item(name)),
    devicePixelRatio_(1.0),
    w_(0.0),
    h_(0.0),
    pressed_(NULL),
    dragged_(NULL),
    startPt_(std::numeric_limits<double>::max(),
             std::numeric_limits<double>::max()),
    mousePt_(std::numeric_limits<double>::max(),
             std::numeric_limits<double>::max())
  {
    root_->self_ = root_;
    Item & root = *root_;
    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);

    repaintTimer_.setSingleShot(true);
    animateTimer_.setInterval(16);

    bool ok = true;
    ok = connect(&repaintTimer_, SIGNAL(timeout()), this, SLOT(repaint()));
    YAE_ASSERT(ok);

    ok = connect(&animateTimer_, SIGNAL(timeout()), this, SLOT(repaint()));
    YAE_ASSERT(ok);
   }

  //----------------------------------------------------------------
  // ItemView::setEnabled
  //
  void
  ItemView::setEnabled(bool enable)
  {
#if YAE_DEBUG_ITEM_VIEW_REPAINT
    std::cerr << "FIXME: ItemView::setEnabled " << root_->id_
              << " " << enable << std::endl;
#endif

    if (!enable)
    {
      // remove item focus for this view:
      const ItemFocus::Target * focus = ItemFocus::singleton().focus();

      if (focus && focus->view_ == this)
      {
        std::string focusItemId;
        ItemPtr itemPtr = focus->item_.lock();
        if (itemPtr)
        {
          focusItemId = itemPtr->id_;
        }

        TMakeCurrentContext currentContext(*context());
        ItemFocus::singleton().clearFocus(focusItemId);
      }
    }
    else
    {
      // make sure next repaint request gets posted:
      requestRepaintEvent_.setDelivered(true);
    }

    Canvas::ILayer::setEnabled(enable);

    Item & root = *root_;
    TMakeCurrentContext currentContext(*context());
    root.notifyObservers(Item::kOnToggleItemView);
  }

  //----------------------------------------------------------------
  // ItemView::event
  //
  bool
  ItemView::event(QEvent * e)
  {
    QEvent::Type et = e->type();
    if (et == QEvent::User)
    {
      RequestRepaintEvent * repaintEvent =
        dynamic_cast<RequestRepaintEvent *>(e);

      if (repaintEvent)
      {
        YAE_BENCHMARK(benchmark, "ItemView::event repaintEvent");

#if YAE_DEBUG_ITEM_VIEW_REPAINT
        std::cerr << "FIXME: ItemView::repaintEvent " << root_->id_
                  << std::endl;
#endif

        Canvas::ILayer::delegate_->requestRepaint();

        repaintEvent->accept();
        return true;
      }

      CancelableEvent * cancelableEvent =
        dynamic_cast<CancelableEvent *>(e);

      if (cancelableEvent)
      {
        cancelableEvent->accept();
        return (!cancelableEvent->ticket_->isCanceled() &&
                cancelableEvent->execute());
      }
    }

    return QObject::event(e);
  }

  //----------------------------------------------------------------
  // ItemView::requestRepaint
  //
  void
  ItemView::requestRepaint()
  {
    bool alreadyRequested = repaintTimer_.isActive();

#if YAE_DEBUG_ITEM_VIEW_REPAINT
    std::cerr << "FIXME: ItemView::requestRepaint " << root_->id_
              << " " << !alreadyRequested << std::endl;
#endif

    if (!alreadyRequested)
    {
      repaintTimer_.start(16);
    }
  }

  //----------------------------------------------------------------
  // ItemView::repaint
  //
  void
  ItemView::repaint()
  {
    bool postThePayload = requestRepaintEvent_.setDelivered(false);

#if YAE_DEBUG_ITEM_VIEW_REPAINT
    std::cerr << "FIXME: ItemView::repaint " << root_->id_
              << " " << postThePayload << std::endl;
#endif

    if (postThePayload)
    {
      // send an event:
      qApp->postEvent(this,
                      new RequestRepaintEvent(requestRepaintEvent_),
                      Qt::HighEventPriority);
    }
  }

  //----------------------------------------------------------------
  // ItemView::resizeTo
  //
  bool
  ItemView::resizeTo(const Canvas * canvas)
  {
    double devicePixelRatio = canvas->devicePixelRatio();
    int w = canvas->canvasWidth();
    int h = canvas->canvasHeight();

    if (devicePixelRatio == devicePixelRatio_ && w == w_ && h_ == h)
    {
      return false;
    }

    devicePixelRatio_ = devicePixelRatio;
    w_ = w;
    h_ = h;

    Item & root = *root_;
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);

    requestUncache(&root);
    return true;
  }

  //----------------------------------------------------------------
  // ItemView::paint
  //
  void
  ItemView::paint(Canvas * canvas)
  {
    YAE_BENCHMARK(benchmark, "ItemView::paint ");

    requestRepaintEvent_.setDelivered(true);

    animate();

    // uncache prior to painting:
    while (!uncache_.empty())
    {
      std::map<Item *, boost::weak_ptr<Item> >::iterator i = uncache_.begin();
      ItemPtr itemPtr = i->second.lock();
      uncache_.erase(i);

      if (!itemPtr)
      {
        continue;
      }

      Item & item = *itemPtr;

      YAE_BENCHMARK(benchmark, (std::string("uncache ") + item.id_).c_str());
      item.uncache();
    }

#if YAE_DEBUG_ITEM_VIEW_REPAINT
    std::cerr << "FIXME: ItemView::paint " << root_->id_ << std::endl;
#endif

    double x = 0.0;
    double y = 0.0;
    double w = double(canvas->canvasWidth());
    double h = double(canvas->canvasHeight());

    YAE_OGL_11_HERE();
    YAE_OGL_11(glViewport(GLint(x + 0.5), GLint(y + 0.5),
                          GLsizei(w + 0.5), GLsizei(h + 0.5)));

    TGLSaveMatrixState pushMatrix0(GL_MODELVIEW);
    YAE_OGL_11(glLoadIdentity());
    TGLSaveMatrixState pushMatrix1(GL_PROJECTION);
    YAE_OGL_11(glLoadIdentity());
    YAE_OGL_11(glOrtho(0.0, w, h, 0.0, -1.0, 1.0));

    YAE_OGL_11(glDisable(GL_LIGHTING));
    YAE_OGL_11(glEnable(GL_LINE_SMOOTH));
    YAE_OGL_11(glHint(GL_LINE_SMOOTH_HINT, GL_NICEST));
    YAE_OGL_11(glDisable(GL_POLYGON_SMOOTH));
    YAE_OGL_11(glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST));
    YAE_OGL_11(glLineWidth(1.0));

    YAE_OGL_11(glEnable(GL_BLEND));
    YAE_OGL_11(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    YAE_OGL_11(glShadeModel(GL_SMOOTH));

    const Segment & xregion = root_->xExtent();
    const Segment & yregion = root_->yExtent();

    Item & root = *root_;
    root.paint(xregion, yregion, canvas);
  }

  //----------------------------------------------------------------
  // ItemView::addAnimator
  //
  void
  ItemView::addAnimator(const ItemView::TAnimatorPtr & animator)
  {
    animators_.insert(animator);

    if (!animateTimer_.isActive())
    {
      animateTimer_.start();
    }
  }

  //----------------------------------------------------------------
  // ItemView::delAnimator
  //
  void
  ItemView::delAnimator(const ItemView::TAnimatorPtr & animator)
  {
    if (animators_.erase(animator) && animators_.empty())
    {
      animateTimer_.stop();
    }
  }

  //----------------------------------------------------------------
  // ItemView::animate
  //
  void
  ItemView::animate()
  {
    typedef std::set<boost::weak_ptr<IAnimator> >::iterator TIter;
    TIter i = animators_.begin();
    while (i != animators_.end())
    {
      TIter i0 = i;
      ++i;

      TAnimatorPtr animatorPtr = i0->lock();
      if (animatorPtr)
      {
        IAnimator & animator = *animatorPtr;
        animator.animate(*this, animatorPtr);
      }
      else
      {
        // remove expired animators:
        animators_.erase(i0);
      }
    }
  }

  //----------------------------------------------------------------
  // ItemView::requestUncache
  //
  void
  ItemView::requestUncache(Item * root)
  {
    if (!root)
    {
      root = root_.get();
    }

    uncache_[root] = root->self_;
  }

  //----------------------------------------------------------------
  // ItemView::processEvent
  //
  bool
  ItemView::processEvent(Canvas * canvas, QEvent * event)
  {
    YAE_BENCHMARK(benchmark, "ItemView::processEvent");

    QEvent::Type et = event->type();
    if (et != QEvent::Paint &&
        et != QEvent::Wheel &&
        et != QEvent::MouseButtonPress &&
        et != QEvent::MouseButtonRelease &&
        et != QEvent::MouseButtonDblClick &&
        et != QEvent::MouseMove &&
        et != QEvent::CursorChange &&
        et != QEvent::Resize &&
        et != QEvent::MacGLWindowChange &&
        et != QEvent::Leave &&
        et != QEvent::Enter &&
        et != QEvent::WindowDeactivate &&
        et != QEvent::WindowActivate &&
        et != QEvent::FocusOut &&
        et != QEvent::FocusIn &&
#ifdef YAE_USE_QT5
        et != QEvent::UpdateRequest &&
#endif
        et != QEvent::ShortcutOverride)
    {
#if 0 // ndef NDEBUG
      std::cerr
        << "ItemView::processEvent: "
        << yae::toString(et)
        << std::endl;
#endif
    }

    if (et == QEvent::Leave)
    {
      mousePt_.set_x(std::numeric_limits<double>::max());
      mousePt_.set_y(std::numeric_limits<double>::max());
      if (processMouseTracking(mousePt_))
      {
        requestRepaint();
      }
    }

    if (et == QEvent::MouseButtonPress ||
        et == QEvent::MouseButtonRelease ||
        et == QEvent::MouseButtonDblClick ||
        et == QEvent::MouseMove)
    {
      TMakeCurrentContext currentContext(*context());
      QMouseEvent * e = static_cast<QMouseEvent *>(event);
      bool processed = processMouseEvent(canvas, e);

#if 0 // ndef NDEBUG
      if (et != QEvent::MouseMove)
      {
        std::cerr
          << root_->id_ << ": "
          << "processed mouse event == " << processed << ", "
          << e->button() << " button, "
          << e->buttons() << " buttons, "
          << yae::toString(et)
          << std::endl;
      }
#endif

      if (processed)
      {
        requestRepaint();
        return true;
      }

      return false;
    }

    if (et == QEvent::Wheel)
    {
      TMakeCurrentContext currentContext(*context());
      QWheelEvent * e = static_cast<QWheelEvent *>(event);
      if (processWheelEvent(canvas, e))
      {
        requestRepaint();
        return true;
      }

      return false;
    }

    if (et == QEvent::KeyPress)
    {
      QKeyEvent & ke = *(static_cast<QKeyEvent *>(event));

      if (ke.key() == Qt::Key_Tab && !ke.modifiers())
      {
        TMakeCurrentContext currentContext(*context());
        if (ItemFocus::singleton().focusNext())
        {
          requestRepaint();
          return true;
        }
      }
      else if (ke.key() == Qt::Key_Backtab ||
               (ke.key() == Qt::Key_Tab &&
                ke.modifiers() == Qt::Key_Shift))
      {
        TMakeCurrentContext currentContext(*context());
        if (ItemFocus::singleton().focusPrevious())
        {
          requestRepaint();
          return true;
        }
      }
    }

    ItemPtr focus = ItemFocus::singleton().focusedItem();
    if (focus && focus->processEvent(*this, canvas, event))
    {
      requestRepaint();
      return true;
    }

    if (et == QEvent::KeyPress ||
        et == QEvent::KeyRelease)
    {
      TMakeCurrentContext currentContext(*context());
      QKeyEvent * e = static_cast<QKeyEvent *>(event);
      if (processKeyEvent(canvas, e))
      {
        requestRepaint();
        return true;
      }

      return false;
    }

    return false;
  }

  //----------------------------------------------------------------
  // ItemView::processKeyEvent
  //
  bool
  ItemView::processKeyEvent(Canvas * canvas, QKeyEvent * e)
  {
    e->ignore();
    return false;
  }

  //----------------------------------------------------------------
  // has
  //
  static bool
  has(const std::list<InputHandler> & handlers, const Item * item)
  {
    for (std::list<InputHandler>::const_iterator
           i = handlers.begin(), end = handlers.end(); i != end; ++i)
    {
      const InputHandler & handler = *i;
      const InputArea * ia = handler.inputArea();

      if (ia == item)
      {
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // ItemView::processMouseEvent
  //
  bool
  ItemView::processMouseEvent(Canvas * canvas, QMouseEvent * e)
  {
    if (!e)
    {
      return false;
    }

    TVec2D pt = device_pixel_pos(canvas, e);
    mousePt_ = pt;

    root_->getVisibleItems(mousePt_, mouseOverItems_);

    QEvent::Type et = e->type();
    if (!((et == QEvent::MouseMove && (e->buttons() & Qt::LeftButton)) ||
          (e->button() == Qt::LeftButton)))
    {
      if (!e->buttons())
      {
        if (processMouseTracking(pt))
        {
          requestRepaint();
        }
      }

      return false;
    }

    if (et == QEvent::MouseButtonPress)
    {
      pressed_ = NULL;
      dragged_ = NULL;
      startPt_ = pt;

      bool foundHandlers = root_->getInputHandlers(pt, inputHandlers_);

      // check for focus loss/transfer:
      ItemPtr focus = ItemFocus::singleton().focusedItem();
      if (focus && !has(inputHandlers_, focus.get()))
      {
        ItemFocus::singleton().clearFocus(focus->id_);
        requestRepaint();
      }

      if (!foundHandlers)
      {
        return false;
      }

      for (TInputHandlerRIter i = inputHandlers_.rbegin();
           i != inputHandlers_.rend(); ++i)
      {
        InputHandler & handler = *i;
        InputArea * ia = handler.inputArea();

        if (ia && ia->onPress(handler.csysOrigin_, pt))
        {
          pressed_ = &handler;
          return true;
        }
      }

      return false;
    }
    else if (et == QEvent::MouseMove)
    {
      if (inputHandlers_.empty())
      {
        std::list<InputHandler> handlers;
        if (!root_->getInputHandlers(pt, handlers))
        {
          return false;
        }

        for (TInputHandlerRIter i = handlers.rbegin();
             i != handlers.rend(); ++i)
        {
          InputHandler & handler = *i;
          InputArea * ia = handler.inputArea();

          if (ia && ia->onMouseOver(handler.csysOrigin_, pt))
          {
            return true;
          }
        }

        return false;
      }

      // FIXME: must add DPI-aware drag threshold to avoid triggering
      // spurious drag events:
      for (TInputHandlerRIter i = inputHandlers_.rbegin();
           i != inputHandlers_.rend(); ++i)
      {
        InputHandler & handler = *i;
        InputArea * ia = handler.inputArea();

        if (!dragged_ && pressed_ != &handler &&
            ia && ia->onPress(handler.csysOrigin_, startPt_))
        {
          // previous handler didn't handle the drag event, try another:
          InputArea * pi = pressed_->inputArea();
          if (pi)
          {
            pi->onCancel();
          }

          pressed_ = &handler;
        }

        if (ia && ia->onDrag(handler.csysOrigin_, startPt_, pt))
        {
          dragged_ = &handler;
          return true;
        }
      }

      return false;
    }
    else if (et == QEvent::MouseButtonRelease)
    {
      bool accept = false;
      if (pressed_)
      {
        InputArea * ia =
          dragged_ ? dragged_->inputArea() : pressed_->inputArea();

        if (ia)
        {
          if (dragged_ && ia->draggable())
          {
            accept = ia->onDragEnd(dragged_->csysOrigin_, startPt_, pt);
          }
          else
          {
            accept = ia->onClick(pressed_->csysOrigin_, pt);
          }
        }
      }

      pressed_ = NULL;
      dragged_ = NULL;
      inputHandlers_.clear();

      return accept;
    }
    else if (et == QEvent::MouseButtonDblClick)
    {
      pressed_ = NULL;
      dragged_ = NULL;
      inputHandlers_.clear();

      std::list<InputHandler> handlers;
      if (!root_->getInputHandlers(pt, handlers))
      {
        return false;
      }

      for (TInputHandlerRIter i = handlers.rbegin();
           i != handlers.rend(); ++i)
      {
        InputHandler & handler = *i;
        InputArea * ia = handler.inputArea();

        if (ia && ia->onDoubleClick(handler.csysOrigin_, pt))
        {
          return true;
        }
      }

      return false;
    }

    return false;
  }

  //----------------------------------------------------------------
  // ItemView::processWheelEvent
  //
  bool
  ItemView::processWheelEvent(Canvas * canvas, QWheelEvent * e)
  {
    if (!e)
    {
      return false;
    }

    TVec2D pt = device_pixel_pos(canvas, e);
    std::list<InputHandler> handlers;
    if (!root_->getInputHandlers(pt, handlers))
    {
      return false;
    }

    // Quoting from QWheelEvent docs:
    //
    //  " Most mouse types work in steps of 15 degrees,
    //    in which case the delta value is a multiple of 120;
    //    i.e., 120 units * 1/8 = 15 degrees. "
    //
    int delta = e->delta();
    double degrees = double(delta) * 0.125;

#if 0
    std::cerr
      << "FIXME: wheel: delta: " << delta
      << ", degrees: " << degrees
      << std::endl;
#endif

    bool processed = false;
    for (TInputHandlerRIter i = handlers.rbegin(); i != handlers.rend(); ++i)
    {
      InputHandler & handler = *i;
      InputArea * ia = handler.inputArea();

      if (ia && ia->onScroll(handler.csysOrigin_, pt, degrees))
      {
        processed = true;
        break;
      }
    }

    return processed;
  }

  //----------------------------------------------------------------
  // ItemView::processMouseTracking
  //
  bool
  ItemView::processMouseTracking(const TVec2D & mousePt)
  {
    (void)mousePt;
    return isEnabled();
  }

  //----------------------------------------------------------------
  // ItemView::addImageProvider
  //
  void
  ItemView::addImageProvider(const QString & providerId,
                             const TImageProviderPtr & p)
  {
    imageProviders_[providerId] = p;
  }


  //----------------------------------------------------------------
  // ItemViewStyle::ItemViewStyle
  //
  ItemViewStyle::ItemViewStyle(const char * id, const ItemView & view):
    Item(id),
    view_(view)
  {
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    font_small_.setHintingPreference(QFont::PreferFullHinting);
#endif

    font_small_.setStyleHint(QFont::SansSerif);
    font_small_.setStyleStrategy((QFont::StyleStrategy)
                                 (QFont::PreferOutline |
                                  QFont::PreferAntialias |
                                  QFont::OpenGLCompatible));

    // main font:
    font_ = font_small_;
    font_large_ = font_small_;

    static bool hasImpact =
      QFontInfo(QFont("impact")).family().
      contains(QString::fromUtf8("impact"), Qt::CaseInsensitive);

    if (hasImpact)
    {
      font_large_.setFamily("impact");

#if !(defined(_WIN32) ||                        \
      defined(__APPLE__) ||                     \
      QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
      font_large_.setStretch(QFont::Condensed);
#endif
    }
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) || !defined(__APPLE__)
    else
#endif
    {
      font_large_.setStretch(QFont::Condensed);
      font_large_.setWeight(QFont::Black);
    }

#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    font_fixed_.setHintingPreference(QFont::PreferFullHinting);
#endif

    font_fixed_.setFamily("Menlo, "
                          "Monaco, "
                          "Droid Sans Mono, "
                          "DejaVu Sans Mono, "
                          "Bitstream Vera Sans Mono, "
                          "Consolas, "
                          "Lucida Sans Typewriter, "
                          "Lucida Console, "
                          "Courier New");
    font_fixed_.setStyleHint(QFont::Monospace);
    font_fixed_.setFixedPitch(true);
    font_fixed_.setStyleStrategy((QFont::StyleStrategy)
                                 (QFont::PreferOutline |
                                  QFont::PreferAntialias |
                                  QFont::OpenGLCompatible));

    anchors_.top_ = ItemRef::constant(0);
    anchors_.left_ = ItemRef::constant(0);
    width_ = ItemRef::constant(0);
    height_ = ItemRef::constant(0);

    title_height_ = addExpr(new CalcTitleHeight(view, 50.0));
    font_size_ = ItemRef::reference(title_height_, 0.15);

    // color palette:
    bg_ = ColorRef::constant(Color(0x1f1f1f, 0.87));
    fg_ = ColorRef::constant(Color(0xffffff, 1.0));

    border_ = ColorRef::constant(Color(0x7f7f7f, 1.0));
    cursor_ = ColorRef::constant(Color(0xf12b24, 1.0));
    scrollbar_ = ColorRef::constant(Color(0x7f7f7f, 0.5));
    separator_ = ColorRef::constant(scrollbar_.get());
    underline_ = ColorRef::constant(cursor_.get());

    bg_timecode_ = ColorRef::constant(Color(0x7f7f7f, 0.25));
    fg_timecode_ = ColorRef::constant(Color(0xFFFFFF, 0.5));

    bg_controls_ =
      ColorRef::constant(bg_timecode_.get());
    fg_controls_ =
      ColorRef::constant(fg_timecode_.get().opaque(0.75));

    bg_focus_ = ColorRef::constant(Color(0x7f7f7f, 0.5));
    fg_focus_ = ColorRef::constant(Color(0xffffff, 1.0));

    bg_edit_selected_ =
      ColorRef::constant(Color(0xffffff, 1.0));
    fg_edit_selected_ =
      ColorRef::constant(Color(0x000000, 1.0));

    timeline_excluded_ =
      ColorRef::constant(Color(0xFFFFFF, 0.2));
    timeline_included_ =
      ColorRef::constant(Color(0xFFFFFF, 0.5));
  }

  //----------------------------------------------------------------
  // ItemViewStyle::uncache
  //
  void
  ItemViewStyle::uncache()
  {
    title_height_.uncache();
    font_size_.uncache();

    bg_.uncache();
    fg_.uncache();

    border_.uncache();
    cursor_.uncache();
    scrollbar_.uncache();
    separator_.uncache();
    underline_.uncache();

    bg_controls_.uncache();
    fg_controls_.uncache();

    bg_focus_.uncache();
    fg_focus_.uncache();

    bg_edit_selected_.uncache();
    fg_edit_selected_.uncache();

    bg_timecode_.uncache();
    fg_timecode_.uncache();

    timeline_excluded_.uncache();
    timeline_included_.uncache();

    Item::uncache();
  }

}
