// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QColor>
#include <QFontMetricsF>
#include <QImage>
#include <QPainter>
#include <QRectF>
#include <QString>

// local interfaces:
#include "yaeCanvasRenderer.h"
#include "yaeText.h"
#include "yaeTexture.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // GetFontAscent::GetFontAscent
  //
  GetFontAscent::GetFontAscent(const Text & item):
    item_(item)
  {}

  //----------------------------------------------------------------
  // GetFontAscent::evaluate
  //
  void
  GetFontAscent::evaluate(double & result) const
  {
    result = item_.fontAscent();
  }


  //----------------------------------------------------------------
  // GetFontDescent::GetFontDescent
  //
  GetFontDescent::GetFontDescent(const Text & item):
    item_(item)
  {}

  //----------------------------------------------------------------
  // GetFontDescent::evaluate
  //
  void
  GetFontDescent::evaluate(double & result) const
  {
    result = item_.fontDescent();
  }


  //----------------------------------------------------------------
  // GetFontHeight::GetFontHeight
  //
  GetFontHeight::GetFontHeight(const Text & item):
    item_(item)
  {}

  //----------------------------------------------------------------
  // GetFontHeight::evaluate
  //
  void
  GetFontHeight::evaluate(double & result) const
  {
    result = item_.fontHeight();
  }


  //----------------------------------------------------------------
  // getMaxRect
  //
  static void
  getMaxRect(const Text & item, QRectF & maxRect)
  {
    double maxWidth =
      (item.maxWidth_.isValid() ||
       item.maxWidth_.isCached()) ? item.maxWidth_.get() :
      (item.width_.isValid() ||
       (item.anchors_.left_.isValid() &&
        item.anchors_.right_.isValid())) ? item.width() :
      double(std::numeric_limits<short int>::max());

    double maxHeight =
      (item.maxHeight_.isValid() ||
       item.maxHeight_.isCached()) ? item.maxHeight_.get() :
      (item.height_.isValid() ||
       (item.anchors_.top_.isValid() &&
        item.anchors_.bottom_.isValid())) ? item.height() :
      double(std::numeric_limits<short int>::max());

    maxRect = QRectF(qreal(0), qreal(0), qreal(maxWidth), qreal(maxHeight));
  }

  //----------------------------------------------------------------
  // getElidedText
  //
  static QString
  getElidedText(double maxWidth,
                const Text & item,
                const QFontMetricsF & fm,
                int flags)
  {
    QString text = item.text_.get().toString();

    if (item.elide_ != Qt::ElideNone)
    {
      QString textElided = fm.elidedText(text, item.elide_, maxWidth, flags);
#if 0
      if (text != textElided)
      {
        std::cerr
          << "original: " << text.toUtf8().constData() << std::endl
          << "  elided: " << textElided.toUtf8().constData() << std::endl;
        YAE_ASSERT(false);
      }
#endif
      text = textElided;
    }

    return text;
  }

  //----------------------------------------------------------------
  // calcTextBBox
  //
  static void
  calcTextBBox(const Text & item,
               BBox & bbox,
               double maxWidth,
               double maxHeight)
  {
    QFont font = item.font_;
    double fontSize = item.fontSize_.get();
    double supersample = item.supersample_.get();

    font.setPointSizeF(fontSize * supersample);
    QFontMetricsF fm(font);

    QRectF maxRect(0.0, 0.0,
                   maxWidth * supersample,
                   maxHeight * supersample);

    int flags = item.textFlags();
    QString text = getElidedText(maxWidth * supersample, item, fm, flags);

    QRectF rect = fm.boundingRect(maxRect, flags, text);
    bbox.x_ = rect.x() / supersample;
    bbox.y_ = rect.y() / supersample;
    bbox.w_ = rect.width() / supersample;
    bbox.h_ = rect.height() / supersample;
  }

  //----------------------------------------------------------------
  // CalcTextBBox
  //
  struct CalcTextBBox : public TBBoxExpr
  {
    CalcTextBBox(const Text & item):
      item_(item)
    {}

    // virtual:
    void evaluate(BBox & result) const
    {
      QRectF maxRect;
      getMaxRect(item_, maxRect);
      calcTextBBox(item_, result, maxRect.width(), maxRect.height());
    }

    const Text & item_;
  };


  //----------------------------------------------------------------
  // Text::TPrivate
  //
  struct Text::TPrivate
  {
    TPrivate();
    ~TPrivate();

    void uncache();
    bool uploadTexture(const Text & item);
    void paint(const Text & item);

    BoolRef ready_;
    GLuint texId_;
    GLuint iw_;
    GLuint ih_;
    GLuint downsample_;
  };

  //----------------------------------------------------------------
  // Text::TPrivate::TPrivate
  //
  Text::TPrivate::TPrivate():
    texId_(0),
    iw_(0),
    ih_(0),
    downsample_(1)
  {}

  //----------------------------------------------------------------
  // Text::TPrivate::~TPrivate
  //
  Text::TPrivate::~TPrivate()
  {
    uncache();
  }

  //----------------------------------------------------------------
  // Text::TPrivate::uncache
  //
  void
  Text::TPrivate::uncache()
  {
    ready_.uncache();

    YAE_OGL_11_HERE();
    YAE_OGL_11(glDeleteTextures(1, &texId_));
    texId_ = 0;
  }

  //----------------------------------------------------------------
  // Text::TPrivate::uploadTexture
  //
  bool
  Text::TPrivate::uploadTexture(const Text & item)
  {
    QRectF maxRect;
    getMaxRect(item, maxRect);

    double supersample = item.supersample_.get();
    maxRect.setWidth(maxRect.width() * supersample);
    maxRect.setHeight(maxRect.height() * supersample);

    BBox bboxContent;
    item.Item::get(kPropertyBBoxContent, bboxContent);

    iw_ = (int)ceil(bboxContent.w_ * supersample);
    ih_ = (int)ceil(bboxContent.h_ * supersample);

    if (!(iw_ && ih_))
    {
      return true;
    }

    GLsizei widthPowerOfTwo = powerOfTwoGEQ<GLsizei>(iw_);
    GLsizei heightPowerOfTwo = powerOfTwoGEQ<GLsizei>(ih_);
    QImage img(widthPowerOfTwo, heightPowerOfTwo, QImage::Format_ARGB32);
    {
      const Color & color = item.color_.get();

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) && defined(__APPLE__)
      Color bg = color.transparent();
#else
      const Color & bg = item.background_.get();
#endif
      img.fill(QColor(bg).rgba());

      QPainter painter(&img);
      QFont font = item.font_;
      double fontSize = item.fontSize_.get();
      font.setPointSizeF(fontSize * supersample);
      painter.setFont(font);

      QFontMetricsF fm(font);
      int flags = item.textFlags();
      QString text = getElidedText(maxRect.width(), item, fm, flags);

      painter.setPen(QColor(color));
      painter.drawText(maxRect, flags, text);
    }

    // do not upload supersampled texture at full size, scale down first:
    downsample_ = downsampleImage(img, supersample);

    bool ok = yae::uploadTexture2D(img, texId_,
                                   supersample == 1.0 ?
                                   GL_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
    return ok;
  }

  //----------------------------------------------------------------
  // Text::TPrivate::paint
  //
  void
  Text::TPrivate::paint(const Text & item)
  {
    BBox bbox;
    item.Item::get(kPropertyBBoxContent, bbox);

    // avoid rendering at fractional pixel coordinates:
    bbox.x_ = std::floor(bbox.x_);
    bbox.y_ = std::floor(bbox.y_);
    bbox.w_ = std::ceil(bbox.w_);
    bbox.h_ = std::ceil(bbox.h_);

    int iw = iw_ / downsample_;
    int ih = ih_ / downsample_;
    paintTexture2D(bbox, texId_, iw, ih);
  }


  //----------------------------------------------------------------
  // Text::Text
  //
  Text::Text(const char * id):
    Item(id),
    p_(new Text::TPrivate()),
    alignment_(Qt::AlignLeft),
    elide_(Qt::ElideNone),
    color_(ColorRef::constant(Color(0xffffff, 1.0))),
    background_(ColorRef::constant(Color(0x000000, 0.0)))
  {
    fontSize_ = ItemRef::constant(font_.pointSizeF());
    supersample_ = ItemRef::constant(1.0);
    bboxText_ = addExpr(new CalcTextBBox(*this));
    p_->ready_ = addExpr(new UploadTexture<Text>(*this));
  }

  //----------------------------------------------------------------
  // Text::~Text
  //
  Text::~Text()
  {
    delete p_;
  }

  //----------------------------------------------------------------
  // Text::textFlags
  //
  int
  Text::textFlags() const
  {
    Qt::TextFlag textFlags = (elide_ == Qt::ElideNone ?
                              Qt::TextWordWrap :
                              Qt::TextSingleLine);

    int flags = alignment_ | textFlags;
    return flags;
  }

  //----------------------------------------------------------------
  // Text::fontAscent
  //
  double
  Text::fontAscent() const
  {
    QFont font = font_;
    double fontSize = fontSize_.get();
    double supersample = supersample_.get();

    font.setPointSizeF(fontSize * supersample);
    QFontMetricsF fm(font);

    double ascent = fm.ascent() / supersample;
    return ascent;
  }

  //----------------------------------------------------------------
  // Text::fontDescent
  //
  double
  Text::fontDescent() const
  {
    QFont font = font_;
    double fontSize = fontSize_.get();
    double supersample = supersample_.get();

    font.setPointSizeF(fontSize * supersample);
    QFontMetricsF fm(font);

    double descent = fm.descent() / supersample;
    return descent;
  }

  //----------------------------------------------------------------
  // Text::fontHeight
  //
  double
  Text::fontHeight() const
  {
    QFont font = font_;
    double fontSize = fontSize_.get();
    double supersample = supersample_.get();

    font.setPointSizeF(fontSize * supersample);
    QFontMetricsF fm(font);

    double fh = fm.height() / supersample;
    return fh;
  }

  //----------------------------------------------------------------
  // Text::calcContentWidth
  //
  double
  Text::calcContentWidth() const
  {
    const BBox & t = bboxText_.get();
    return t.w_;
  }

  //----------------------------------------------------------------
  // Text::calcContentHeight
  //
  double
  Text::calcContentHeight() const
  {
    if (elide_ != Qt::ElideNone)
    {
      // single line:
      BBox bbox;
      calcTextBBox(*this, bbox,
                   double(std::numeric_limits<short int>::max()),
                   double(std::numeric_limits<short int>::max()));
      return bbox.h_;
    }

    // possible line-wrapping:
    const BBox & t = bboxText_.get();
    return t.h_;
  }

  //----------------------------------------------------------------
  // Text::uncache
  //
  void
  Text::uncache()
  {
    bboxText_.uncache();
    text_.uncache();
    fontSize_.uncache();
    supersample_.uncache();
    maxWidth_.uncache();
    maxHeight_.uncache();
    color_.uncache();
    background_.uncache();
    p_->uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Text::paintContent
  //
  void
  Text::paintContent() const
  {
    if (p_->ready_.get())
    {
      p_->paint(*this);
    }
  }

  //----------------------------------------------------------------
  // Text::unpaintContent
  //
  void
  Text::unpaintContent() const
  {
    p_->uncache();
  }

  //----------------------------------------------------------------
  // Text::text
  //
  QString
  Text::text() const
  {
    return text_.get().toString();
  }

  //----------------------------------------------------------------
  // Text::get
  //
  void
  Text::get(Property property, bool & value) const
  {
    if (property == kPropertyHasText)
    {
      value = !(text().isEmpty());
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // Text::get
  //
  void
  Text::get(Property property, Color & value) const
  {
    if (property == kPropertyColor)
    {
      value = color_.get();
    }
    else if (property == kPropertyColorBg)
    {
      value = background_.get();
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // Text::get
  //
  void
  Text::get(Property property, TVar & value) const
  {
    if (property == kPropertyText)
    {
      value = TVar(text());
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // Text::copySettings
  //
  void
  Text::copySettings(const Text & src)
  {
    font_ = src.font_;
    alignment_ = src.alignment_;
    elide_ = src.elide_;

    text_ = src.text_;
    fontSize_ = src.fontSize_;
    maxWidth_ = src.maxWidth_;
    maxHeight_ = src.maxHeight_;
    color_ = src.color_;
    background_ = src.background_;
  }

}
