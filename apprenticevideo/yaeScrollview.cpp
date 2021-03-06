// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local interfaces:
#include "yaeCanvasRenderer.h"
#include "yaeFlickableArea.h"
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"
#include "yaeRoundRect.h"
#include "yaeScrollview.h"


namespace yae
{

  //----------------------------------------------------------------
  // CalcSliderTop::CalcSliderTop
  //
  CalcSliderTop::CalcSliderTop(const Scrollview & view,
                               const Item & scrollbar,
                               const Item & slider):
    view_(view),
    scrollbar_(scrollbar),
    slider_(slider)
  {}

  //----------------------------------------------------------------
  // CalcSliderTop::evaluate
  //
  void
  CalcSliderTop::evaluate(double & result) const
  {
    result = scrollbar_.top();

    double scrollbarHeight = scrollbar_.height();
    double sceneHeight = view_.content_->height();
    double viewHeight = view_.height();
    if (sceneHeight <= viewHeight)
    {
      return;
    }

    double scale = viewHeight / sceneHeight;
    double minHeight = slider_.width() * 5.0;
    double height = minHeight + (scrollbarHeight - minHeight) * scale;
    double y = (scrollbarHeight - height) * view_.position_.y();
    result += y;
  }


  //----------------------------------------------------------------
  // CalcSliderHeight::CalcSliderHeight
  //
  CalcSliderHeight::CalcSliderHeight(const Scrollview & view,
                                     const Item & scrollbar,
                                     const Item & slider):
    view_(view),
    scrollbar_(scrollbar),
    slider_(slider)
  {}

  //----------------------------------------------------------------
  // CalcSliderHeight::evaluate
  //
  void
  CalcSliderHeight::evaluate(double & result) const
  {
    double scrollbarHeight = scrollbar_.height();
    double sceneHeight = view_.content_->height();
    double viewHeight = view_.height();
    if (sceneHeight <= viewHeight)
    {
      result = scrollbarHeight;
      return;
    }

    double scale = viewHeight / sceneHeight;
    double minHeight = slider_.width() * 5.0;
    result = minHeight + (scrollbarHeight - minHeight) * scale;
  }


  //----------------------------------------------------------------
  // CalcSliderLeft::CalcSliderLeft
  //
  CalcSliderLeft::CalcSliderLeft(const Scrollview & view,
                                 const Item & scrollbar,
                                 const Item & slider):
    view_(view),
    scrollbar_(scrollbar),
    slider_(slider)
  {}

  //----------------------------------------------------------------
  // CalcSliderLeft::evaluate
  //
  void
  CalcSliderLeft::evaluate(double & result) const
  {
    result = scrollbar_.left();

    double scrollbarWidth = scrollbar_.width();
    double sceneWidth = view_.content_->width();
    double viewWidth = view_.width();
    if (sceneWidth <= viewWidth)
    {
      return;
    }

    double scale = viewWidth / sceneWidth;
    double minWidth = slider_.height() * 5.0;
    double width = minWidth + (scrollbarWidth - minWidth) * scale;
    double x = (scrollbarWidth - width) * view_.position_.x();
    result += x;
  }


  //----------------------------------------------------------------
  // CalcSliderWidth::CalcSliderWidth
  //
  CalcSliderWidth::CalcSliderWidth(const Scrollview & view,
                                   const Item & scrollbar,
                                   const Item & slider):
    view_(view),
    scrollbar_(scrollbar),
    slider_(slider)
  {}

  //----------------------------------------------------------------
  // CalcSliderWidth::evaluate
  //
  void
  CalcSliderWidth::evaluate(double & result) const
  {
    double scrollbarWidth = scrollbar_.width();
    double sceneWidth = view_.content_->width();
    double viewWidth = view_.width();
    if (sceneWidth <= viewWidth)
    {
      result = scrollbarWidth;
      return;
    }

    double scale = viewWidth / sceneWidth;
    double minWidth = slider_.height() * 5.0;
    result = minWidth + (scrollbarWidth - minWidth) * scale;
  }


  //----------------------------------------------------------------
  // Scrollview::Scrollview
  //
  Scrollview::Scrollview(const char * id,
                         bool clipContent,
                         bool uncacheContent):
    Item(id),
    content_(new Item((std::string(id) + ".content").c_str())),
    clipContent_(clipContent),
    uncacheContent_(uncacheContent)
  {
    content_->self_ = content_;
  }

  //----------------------------------------------------------------
  // Scrollview::uncache
  //
  void
  Scrollview::uncache()
  {
    Item::uncache();

    if (uncacheContent_)
    {
      content_->uncache();
    }
  }

  //----------------------------------------------------------------
  // Scrollview::getContentView
  //
  void
  Scrollview::getContentView(TVec2D & origin,
                             Segment & xView,
                             Segment & yView) const
  {
    double sceneHeight = this->content_->height();
    double viewHeight = this->height();

    double sceneWidth = this->content_->width();
    double viewWidth = this->width();

    const Segment & xExtent = this->xExtent();
    const Segment & yExtent = this->yExtent();

    double dy = 0.0;
    if (sceneHeight > viewHeight)
    {
      double range = sceneHeight - viewHeight;
      dy = this->position_.y() * range;
    }

    double dx = 0.0;
    if (sceneWidth > viewWidth)
    {
      double range = sceneWidth - viewWidth;
      dx = this->position_.x() * range;
    }

    origin.x() = floor(xExtent.origin_ - dx);
    origin.y() = floor(yExtent.origin_ - dy);
    xView = Segment(dx, xExtent.length_);
    yView = Segment(dy, yExtent.length_);
  }

  //----------------------------------------------------------------
  // Scrollview::getInputHandlers
  //
  void
  Scrollview::getInputHandlers(// coordinate system origin of
                               // the input area, expressed in the
                               // coordinate system of the root item:
                               const TVec2D & itemCSysOrigin,

                               // point expressed in the coord.sys. of the item,
                               // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                               const TVec2D & itemCSysPoint,

                               // pass back input areas overlapping above point,
                               // along with its coord. system origin expressed
                               // in the coordinate system of the root item:
                               std::list<InputHandler> & inputHandlers)
  {
    Item::getInputHandlers(itemCSysOrigin, itemCSysPoint, inputHandlers);

    TVec2D origin;
    Segment xView;
    Segment yView;
    getContentView(origin, xView, yView);

    TVec2D ptInViewCoords = itemCSysPoint - origin;
    TVec2D offsetToView = itemCSysOrigin + origin;
    content_->getInputHandlers(offsetToView, ptInViewCoords, inputHandlers);
  }

  //----------------------------------------------------------------
  // Scrollview::getVisibleItems
  //
  void
  Scrollview::getVisibleItems(// coordinate system origin of
                              // the item, expressed in the
                              // coordinate system of the root item:
                              const TVec2D & itemCSysOrigin,

                              // point expressed in the coord.sys. of the item,
                              // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                              const TVec2D & itemCSysPoint,

                              // pass back items overlapping above point,
                              // along with its coord. system origin expressed
                              // in the coordinate system of the root item:
                              std::list<VisibleItem> & visibleItems)
  {
    Item::getVisibleItems(itemCSysOrigin, itemCSysPoint, visibleItems);

    TVec2D origin;
    Segment xView;
    Segment yView;
    getContentView(origin, xView, yView);

    TVec2D ptInViewCoords = itemCSysPoint - origin;
    TVec2D offsetToView = itemCSysOrigin + origin;
    content_->getVisibleItems(offsetToView, ptInViewCoords, visibleItems);
  }

  //----------------------------------------------------------------
  // Scrollview::paint
  //
  bool
  Scrollview::paint(const Segment & xregion,
                    const Segment & yregion,
                    Canvas * canvas) const
  {
    const Item & content = *content_;

    if (!Item::paint(xregion, yregion, canvas))
    {
      content.unpaint();
      return false;
    }

    if (clipContent_)
    {
      BBox bbox;
      Item::get(kPropertyBBox, bbox);

      YAE_OGL_11_HERE();
      YAE_OGL_11(glEnable(GL_SCISSOR_TEST));
      yae_assert_gl_no_error();
      YAE_OGL_11(glScissor(bbox.x_, canvas->canvasHeight() - bbox.bottom(),
                           bbox.w_, bbox.h_));
      yae_assert_gl_no_error();
    }

    TVec2D origin;
    Segment xView;
    Segment yView;
    getContentView(origin, xView, yView);

    TGLSaveMatrixState pushMatrix(GL_MODELVIEW);
    YAE_OGL_11_HERE();
    YAE_OGL_11(glTranslated(origin.x(), origin.y(), 0.0));
    content.paint(xView, yView, canvas);

    if (clipContent_)
    {
      YAE_OGL_11(glDisable(GL_SCISSOR_TEST));
    }

    return true;
  }

  //----------------------------------------------------------------
  // Scrollview::unpaint
  //
  void
  Scrollview::unpaint() const
  {
    Item::unpaint();
    content_->unpaint();
  }

#ifndef NDEBUG
  //----------------------------------------------------------------
  // Scrollview::dump
  //
  void
  Scrollview::dump(std::ostream & os, const std::string & indent) const
  {
    Item::dump(os, indent);
    content_->dump(os, indent + "  ");
  }
#endif


  //----------------------------------------------------------------
  // SliderDrag::SliderDrag
  //
  SliderDrag::SliderDrag(const char * id,
                         const Canvas::ILayer & canvasLayer,
                         Scrollview & scrollview,
                         Item & scrollbar):
    InputArea(id),
    canvasLayer_(canvasLayer),
    scrollview_(scrollview),
    scrollbar_(scrollbar),
    startPos_(0.0)
  {}

  //----------------------------------------------------------------
  // SliderDrag::onPress
  //
  bool
  SliderDrag::onPress(const TVec2D & itemCSysOrigin,
                      const TVec2D & rootCSysPoint)
  {
    bool vertical = scrollbar_.attr<bool>("vertical", true);
    startPos_ = vertical ?
      scrollview_.position_.y() :
      scrollview_.position_.x();
    return true;
  }

  //----------------------------------------------------------------
  // SliderDrag::onDrag
  //
  bool
  SliderDrag::onDrag(const TVec2D & itemCSysOrigin,
                     const TVec2D & rootCSysDragStart,
                     const TVec2D & rootCSysDragEnd)
  {
    bool vertical = scrollbar_.attr<bool>("vertical", true);
    double bz = vertical ? scrollbar_.height() : scrollbar_.width();
    double sz = vertical ? this->height() : this->width();
    double range = bz - sz;

    double dz = vertical ?
      rootCSysDragEnd.y() - rootCSysDragStart.y() :
      rootCSysDragEnd.x() - rootCSysDragStart.x();

    double ds = dz / range;
    double t = std::min<double>(1.0, std::max<double>(0.0, startPos_ + ds));

    if (vertical)
    {
      scrollview_.position_.set_y(t);
    }
    else
    {
      scrollview_.position_.set_x(t);
    }

    parent_->uncache();
    canvasLayer_.delegate()->requestRepaint();

    return true;
  }

  //----------------------------------------------------------------
  // scrollbars_required
  //
  ScrollbarId
  scrollbars_required(const Item & content,
                      const ItemRef & left,
                      const ItemRef & right,
                      const ItemRef & top,
                      const ItemRef & bottom,
                      const ItemRef & vscrollbarWidth,
                      const ItemRef & hscrollbarWidth)
  {
    double zv = vscrollbarWidth.get();
    double zh = hscrollbarWidth.get();

    double sceneWidth = 0.0;
    try { sceneWidth = zh ? content.width() : 0.0; } catch (...) {}

    double sceneHeight = 0.0;
    try { sceneHeight = zv ? content.height() : 0.0; } catch (...) {}

    double x0 = left.get();
    double x1 = right.get();
    double viewWidth = x1 - x0;

    double y0 = top.get();
    double y1 = bottom.get();
    double viewHeight = y1 - y0;

    bool horizontal = viewWidth < sceneWidth;
    bool vertical = viewHeight < sceneHeight;

    if (horizontal && zh > 0.0)
    {
      viewHeight -= zh;
    }

    if (vertical && zv > 0.0)
    {
      viewWidth -= zv;
    }

    vertical = viewHeight < sceneHeight;
    horizontal = viewWidth < sceneWidth;

    int required = kScrollbarNone;

    if (vertical)
    {
      required |= kScrollbarVertical;
    }

    if (horizontal)
    {
      required |= kScrollbarHorizontal;
    }

    return (ScrollbarId)required;
  }

  //----------------------------------------------------------------
  // layout_scrollview
  //
  Scrollview &
  layout_scrollview(ScrollbarId scrollbars,
                    ItemView & view,
                    const ItemViewStyle & style,
                    Item & root,
                    ScrollbarId inset,
                    bool clipContent)
  {
    bool inset_h = (kScrollbarHorizontal & inset) == kScrollbarHorizontal;
    bool inset_v = (kScrollbarVertical & inset) == kScrollbarVertical;

    Scrollview & sview = root.
      addNew<Scrollview>((std::string(root.id_) + ".scrollview").c_str());
    sview.clipContent_ = clipContent;

    Item & scrollbar = root.addNew<Item>("scrollbar");
    Item & hscrollbar = root.addNew<Item>("hscrollbar");

    scrollbar.anchors_.top_ = ItemRef::reference(sview, kPropertyTop);
    scrollbar.anchors_.bottom_ = ItemRef::reference(hscrollbar, kPropertyTop);
    scrollbar.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
    scrollbar.visible_ = scrollbar.
      addExpr(new ScrollbarRequired
              (sview,
               kScrollbarVertical,

               // vertical scrollbar width:
               (scrollbars & kScrollbarVertical) == kScrollbarVertical ?
               ItemRef::uncacheable(style.row_height_, 0.41667) :
               ItemRef::constant(inset_v ? -1.0 : 0.0),

               // horizontal scrollbar width:
               (scrollbars & kScrollbarHorizontal) == kScrollbarHorizontal ?
               ItemRef::uncacheable(style.row_height_, 0.41667) :
               ItemRef::constant(inset_h ? -1.0 : 0.0),

               ItemRef::uncacheable(sview, kPropertyLeft),
               ItemRef::uncacheable(root, kPropertyRight),
               ItemRef::uncacheable(sview, kPropertyTop),
               ItemRef::uncacheable(root, kPropertyBottom)));

    scrollbar.width_ = scrollbar.addExpr
      (new Conditional<ItemRef>
       (scrollbar.visible_,
        ItemRef::uncacheable(style.row_height_, 0.41667),
        ItemRef::constant(0.0)));

    hscrollbar.setAttr("vertical", false);
    hscrollbar.anchors_.left_ = ItemRef::reference(sview, kPropertyLeft);
    hscrollbar.anchors_.right_ = ItemRef::reference(scrollbar, kPropertyLeft);
    hscrollbar.anchors_.bottom_ = ItemRef::reference(root, kPropertyBottom);

    hscrollbar.visible_ = hscrollbar.
      addExpr(new ScrollbarRequired
              (sview,
               kScrollbarHorizontal,

               (scrollbars & kScrollbarVertical) == kScrollbarVertical ?
               ItemRef::uncacheable(style.row_height_, 0.41667) :
               ItemRef::constant(inset_v ? -1.0 : 0.0),

               (scrollbars & kScrollbarHorizontal) == kScrollbarHorizontal ?
               ItemRef::uncacheable(style.row_height_, 0.41667) :
               ItemRef::constant(inset_h ? -1.0 : 0.0),

               ItemRef::uncacheable(sview, kPropertyLeft),
               ItemRef::uncacheable(root, kPropertyRight),
               ItemRef::uncacheable(sview, kPropertyTop),
               ItemRef::uncacheable(root, kPropertyBottom)));

    hscrollbar.height_ = hscrollbar.addExpr
      (new Conditional<ItemRef>
       (hscrollbar.visible_,
        ItemRef::uncacheable(style.row_height_, 0.41667),
        ItemRef::constant(0.0)));

    sview.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
    sview.anchors_.top_ = ItemRef::reference(root, kPropertyTop);

    sview.anchors_.right_ =
      inset_v ?
      ItemRef::reference(root, kPropertyRight) :
      ItemRef::reference(scrollbar, kPropertyLeft);

    sview.anchors_.bottom_ =
      inset_h ?
      ItemRef::reference(root, kPropertyBottom) :
      ItemRef::reference(hscrollbar, kPropertyTop);

    Item & content = *(sview.content_);
    content.anchors_.left_ = ItemRef::constant(0.0);
    content.anchors_.top_ = ItemRef::constant(0.0);

    if ((scrollbars & kScrollbarHorizontal) != kScrollbarHorizontal)
    {
      content.width_ = ItemRef::reference(sview, kPropertyWidth);
    }

    FlickableArea & maScrollview =
      sview.add(new FlickableArea("ma_sview",
                                  view,
                                  &scrollbar,
                                  &hscrollbar));
    maScrollview.anchors_.fill(sview);

    InputArea & maScrollbar = scrollbar.addNew<InputArea>("ma_scrollbar");
    maScrollbar.anchors_.fill(scrollbar);

    // configure scrollbar slider:
    RoundRect & slider = scrollbar.addNew<RoundRect>("slider");
    slider.anchors_.top_ = slider.
      addExpr(new CalcSliderTop(sview, scrollbar, slider));
    slider.anchors_.left_ = ItemRef::offset(scrollbar, kPropertyLeft, 2);
    slider.anchors_.right_ = ItemRef::offset(scrollbar, kPropertyRight, -2);
    slider.height_ = slider.
      addExpr(new CalcSliderHeight(sview, scrollbar, slider));
    slider.radius_ = ItemRef::scale(slider, kPropertyWidth, 0.5);
    slider.background_ = slider.
      addExpr(style_color_ref(view, &ItemViewStyle::bg_, 0));
    slider.color_ = slider.
      addExpr(style_color_ref(view, &ItemViewStyle::scrollbar_));

    SliderDrag & maSlider =
      slider.add(new SliderDrag("ma_slider", view, sview, scrollbar));
    maSlider.anchors_.fill(slider);

    InputArea & maHScrollbar = scrollbar.addNew<InputArea>("ma_hscrollbar");
    maHScrollbar.anchors_.fill(hscrollbar);

    // configure horizontal scrollbar slider:
    RoundRect & hslider = hscrollbar.addNew<RoundRect>("hslider");
    hslider.anchors_.top_ = ItemRef::offset(hscrollbar, kPropertyTop, 2);
    hslider.anchors_.bottom_ = ItemRef::offset(hscrollbar, kPropertyBottom, -2);
    hslider.anchors_.left_ =
      hslider.addExpr(new CalcSliderLeft(sview, hscrollbar, hslider));
    hslider.width_ =
      hslider.addExpr(new CalcSliderWidth(sview, hscrollbar, hslider));
    hslider.radius_ = ItemRef::scale(hslider, kPropertyHeight, 0.5);
    hslider.background_ = hslider.
      addExpr(style_color_ref(view, &ItemViewStyle::bg_, 0));
    hslider.color_ = hslider.
      addExpr(style_color_ref(view, &ItemViewStyle::scrollbar_));

    SliderDrag & maHSlider =
      hslider.add(new SliderDrag("ma_hslider", view, sview, hscrollbar));
    maHSlider.anchors_.fill(hslider);

    return sview;
  }

  //----------------------------------------------------------------
  // layout_scrollview
  //
  Scrollview &
  layout_scrollview(ItemView & view,
                    Item & root,
                    ScrollbarId scroll,
                    bool clipContent)
  {
    Scrollview & sview = root.
      addNew<Scrollview>((std::string(root.id_) + ".scrollview").c_str());
    sview.clipContent_ = clipContent;

    sview.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
    sview.anchors_.top_ = ItemRef::reference(root, kPropertyTop);
    sview.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
    sview.anchors_.bottom_ = ItemRef::reference(root, kPropertyBottom);

    Item & content = *(sview.content_);
    content.anchors_.left_ = ItemRef::constant(0.0);
    content.anchors_.top_ = ItemRef::constant(0.0);

    if ((scroll & kScrollbarHorizontal) != kScrollbarHorizontal)
    {
      content.width_ = ItemRef::reference(sview, kPropertyWidth);
    }

    if ((scroll & kScrollbarVertical) != kScrollbarVertical)
    {
      content.height_ = ItemRef::reference(sview, kPropertyHeight);
    }

    FlickableArea & maScrollview =
      sview.add(new FlickableArea("ma_sview", view));
    maScrollview.anchors_.fill(sview);

    return sview;
  }
}
