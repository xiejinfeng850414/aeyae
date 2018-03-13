// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan  2 16:32:55 PST 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++ library:
#include <algorithm>
#include <cmath>

// local interfaces:
#include "yaePlaylistViewStyle.h"
#include "yaeText.h"
#include "yaeTexture.h"


namespace yae
{

  //----------------------------------------------------------------
  // calcItemsPerRow
  //
  unsigned int
  calcItemsPerRow(double rowWidth, double cellWidth)
  {
    double n = std::max(1.0, std::floor(rowWidth / cellWidth));
    return (unsigned int)n;
  }

  //----------------------------------------------------------------
  // xbuttonImage
  //
  QImage
  xbuttonImage(unsigned int w,
               const Color & color,
               const Color & background,
               double thickness,
               double rotateAngle)
  {
    QImage img(w, w, QImage::Format_ARGB32);

    // supersample each pixel:
    static const TVec2D sp[] = { TVec2D(0.25, 0.25), TVec2D(0.75, 0.25),
                                 TVec2D(0.25, 0.75), TVec2D(0.75, 0.75) };

    static const unsigned int supersample = sizeof(sp) / sizeof(TVec2D);

    int w2 = w / 2;
    double diameter = double(w);
    double center = diameter * 0.5;
    Segment sa(-center, diameter);
    Segment sb(-diameter * thickness * 0.5, diameter * thickness);

    double rotate = M_PI * (1.0 + (rotateAngle / 180.0));
    TVec2D u_axis(std::cos(rotate), std::sin(rotate));
    TVec2D v_axis(-u_axis.y(), u_axis.x());
    TVec2D origin(0.0, 0.0);

    Vec<double, 4> outerColor(background);
    Vec<double, 4> innerColor(color);
    TVec2D samplePoint;

    for (int y = 0; y < int(w); y++)
    {
      unsigned char * dst = img.scanLine(y);
      samplePoint.set_y(double(y - w2));

      for (int x = 0; x < int(w); x++, dst += 4)
      {
        samplePoint.set_x(double(x - w2));

        double outer = 0.0;
        double inner = 0.0;

        for (unsigned int k = 0; k < supersample; k++)
        {
          TVec2D wcs_pt = samplePoint + sp[k];
          TVec2D pt = wcs_to_lcs(origin, u_axis, v_axis, wcs_pt);
          double oh = sa.pixelOverlap(pt.x()) * sb.pixelOverlap(pt.y());
          double ov = sb.pixelOverlap(pt.x()) * sa.pixelOverlap(pt.y());
          double innerOverlap = std::max<double>(oh, ov);
          double outerOverlap = 1.0 - innerOverlap;

          outer += outerOverlap;
          inner += innerOverlap;
        }

        double outerWeight = outer / double(supersample);
        double innerWeight = inner / double(supersample);
        Color c(outerColor * outerWeight + innerColor * innerWeight);
        memcpy(dst, &(c.argb_), sizeof(c.argb_));
      }
    }

    return img;
  }

  //----------------------------------------------------------------
  // triangleImage
  //
  QImage
  triangleImage(unsigned int w,
                const Color & color,
                const Color & background,
                double rotateAngle)
  {
    QImage img(w, w, QImage::Format_ARGB32);

    // supersample each pixel:
    static const TVec2D sp[] = { TVec2D(0.25, 0.25), TVec2D(0.75, 0.25),
                                 TVec2D(0.25, 0.75), TVec2D(0.75, 0.75) };

    static const unsigned int supersample = sizeof(sp) / sizeof(TVec2D);
    static const double sqrt_3 = 1.732050807568877;

    int w2 = w / 2;
    double diameter = double(w);
    double radius = diameter * 0.5;
    double half_r = diameter * 0.25;
    double base_w = radius * sqrt_3;
    double half_b = base_w * 0.5;
    double height = radius + half_r;

    double rotate = M_PI * (1.0 + (rotateAngle / 180.0));
    TVec2D u_axis(std::cos(rotate), std::sin(rotate));
    TVec2D v_axis(-u_axis.y(), u_axis.x());
    TVec2D origin(0.0, 0.0);

    Vec<double, 4> outerColor(background);
    Vec<double, 4> innerColor(color);
    TVec2D samplePoint;

    for (int y = 0; y < int(w); y++)
    {
      unsigned char * dst = img.scanLine(y);
      samplePoint.set_y(double(y - w2));

      for (int x = 0; x < int(w); x++, dst += 4)
      {
        samplePoint.set_x(double(x - w2));

        double outer = 0.0;
        double inner = 0.0;

        for (unsigned int k = 0; k < supersample; k++)
        {
          TVec2D wcs_pt = samplePoint + sp[k];
          TVec2D pt = wcs_to_lcs(origin, u_axis, v_axis, wcs_pt);

          double ty = pt.y() + half_r;
          double t = 1.0 - ty / height;
          double tb = (t > 1.0) ? 0.0 : (t * half_b);
          double tx = (tb <= 0.0) ? -1.0 : (1.0 - fabs(pt.x()) / tb);
          double innerOverlap = (tx >= 0.0);
          double outerOverlap = 1.0 - innerOverlap;

          outer += outerOverlap;
          inner += innerOverlap;
        }

        double outerWeight = outer / double(supersample);
        double innerWeight = inner / double(supersample);
        Color c(outerColor * outerWeight + innerColor * innerWeight);
        memcpy(dst, &(c.argb_), sizeof(c.argb_));
      }
    }

    return img;
  }

  //----------------------------------------------------------------
  // twobarsImage
  //
  QImage
  barsImage(unsigned int w,
            const Color & color,
            const Color & background,
            unsigned int nbars,
            double thickness,
            double rotateAngle)
  {
    QImage img(w, w, QImage::Format_ARGB32);

    // supersample each pixel:
    static const TVec2D sp[] = { TVec2D(0.25, 0.25), TVec2D(0.75, 0.25),
                                 TVec2D(0.25, 0.75), TVec2D(0.75, 0.75) };

    static const unsigned int supersample = sizeof(sp) / sizeof(TVec2D);

    YAE_ASSERT(nbars > 0 && nbars < w && thickness < 1.0);

    int w2 = w / 2;
    double diameter = double(w);
    double radius = diameter * 0.5;
    Segment sv(-radius, diameter);
    double band_w = diameter / double(nbars);
    double bar_w = thickness * band_w;
    double spacing = band_w - bar_w;
    Segment sh(0.5 * spacing, bar_w);
    double offset = diameter + ((nbars & 1) ? (0.5 * band_w) : 0.0);

    double rotate = M_PI * (1.0 + (rotateAngle / 180.0));
    TVec2D u_axis(std::cos(rotate), std::sin(rotate));
    TVec2D v_axis(-u_axis.y(), u_axis.x());
    TVec2D origin(0.0, 0.0);

    Vec<double, 4> outerColor(background);
    Vec<double, 4> innerColor(color);
    TVec2D samplePoint;

    for (int y = 0; y < int(w); y++)
    {
      unsigned char * dst = img.scanLine(y);
      samplePoint.set_y(double(y - w2));

      for (int x = 0; x < int(w); x++, dst += 4)
      {
        samplePoint.set_x(double(x - w2));

        double outer = 0.0;
        double inner = 0.0;

        for (unsigned int k = 0; k < supersample; k++)
        {
          TVec2D wcs_pt = samplePoint + sp[k];
          TVec2D pt = wcs_to_lcs(origin, u_axis, v_axis, wcs_pt);

          // the image is periodic and symmetric:
          double px = fmod(pt.x() + offset, band_w);
          double py = fabs(pt.y());
          double innerOverlap = sh.pixelOverlap(px) * sv.pixelOverlap(py);
          double outerOverlap = 1.0 - innerOverlap;

          outer += outerOverlap;
          inner += innerOverlap;
        }

        double outerWeight = outer / double(supersample);
        double innerWeight = inner / double(supersample);
        Color c(outerColor * outerWeight + innerColor * innerWeight);
        memcpy(dst, &(c.argb_), sizeof(c.argb_));
      }
    }

    return img;
  }


  //----------------------------------------------------------------
  // PlaylistViewStyle::PlaylistViewStyle
  //
  PlaylistViewStyle::PlaylistViewStyle(const char * id,
                                       PlaylistView & playlist):
    ItemViewStyle(id, playlist),
    playlist_(playlist),
    title_height_(Item::addNewHidden<Item>("title_height")),
    cell_width_(Item::addNewHidden<Item>("cell_width")),
    cell_height_(Item::addNewHidden<Item>("cell_height")),
    font_size_(Item::addNewHidden<Item>("font_size")),
    now_playing_(Item::addNewHidden<Text>("now_playing")),
    eyetv_badge_(Item::addNewHidden<Text>("eyetv_badge")),
    font_fixed_("")
  {
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

    xbutton_ = Item::addHidden<Texture>
      (new Texture("xbutton", QImage())).sharedPtr<Texture>();

    collapsed_ = Item::addHidden<Texture>
      (new Texture("collapsed", QImage())).sharedPtr<Texture>();

    expanded_ = Item::addHidden<Texture>
      (new Texture("expanded", QImage())).sharedPtr<Texture>();

    pause_ = Item::addHidden<Texture>
      (new Texture("pause", QImage())).sharedPtr<Texture>();

    play_ = Item::addHidden<Texture>
      (new Texture("play", QImage())).sharedPtr<Texture>();

    grid_on_ = Item::addHidden<Texture>
      (new Texture("grid_on", QImage())).sharedPtr<Texture>();

    grid_off_ = Item::addHidden<Texture>
      (new Texture("grid_off", QImage())).sharedPtr<Texture>();
  }

  //----------------------------------------------------------------
  // PlaylistViewStyle::color
  //
  const Color &
  PlaylistViewStyle::color(PlaylistViewStyle::ColorId id) const
  {
    switch (id)
    {
      case kBg:
        return bg_;

      case kFg:
        return fg_;

      case kBorder:
        return border_;

      case kCursor:
        return cursor_;

      case kScrollbar:
        return scrollbar_;

      case kSeparator:
        return separator_;

      case kUnderline:
        return underline_;

      case kBgControls:
        return bg_controls_;

      case kFgControls:
        return fg_controls_;

      case kBgXButton:
        return bg_xbutton_;

      case kFgXButton:
        return fg_xbutton_;

      case kBgFocus:
        return bg_focus_;

      case kFgFocus:
        return fg_focus_;

      case kBgEditSelected:
        return bg_edit_selected_;

      case kFgEditSelected:
        return fg_edit_selected_;

      case kBgTimecode:
        return bg_timecode_;

      case kFgTimecode:
        return fg_timecode_;

      case kBgHint:
        return bg_hint_;

      case kFgHint:
        return fg_hint_;

      case kBgBadge:
        return bg_badge_;

      case kFgBadge:
        return fg_badge_;

      case kBgLabel:
        return bg_label_;

      case kFgLabel:
        return fg_label_;

      case kBgLabelSelected:
        return bg_label_selected_;

      case kFgLabelSelected:
        return fg_label_selected_;

      case kBgGroup:
        return bg_group_;

      case kFgGroup:
        return fg_group_;

      case kBgItem:
        return bg_item_;

      case kBgItemPlaying:
        return bg_item_playing_;

      case kBgItemSelected:
        return bg_item_selected_;

      case kTimelineExcluded:
        return timeline_excluded_;

      case kTimelineIncluded:
        return timeline_included_;

      case kTimelinePlayed:
        return timeline_played_;

      default:
        break;
    }

    YAE_ASSERT(false);
    return bg_;
  }

}
