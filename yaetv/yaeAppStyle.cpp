// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Feb  7 08:52:17 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local:
#include "yaeAppStyle.h"


namespace yae
{

  //----------------------------------------------------------------
  // AppStyle::AppStyle
  //
  AppStyle::AppStyle(const char * id, const ItemView & view):
    ItemViewStyle(id, view)
  {
    font_.setFamily(QString::fromUtf8("Lucida Grande"));

    unit_size_ = addExpr(new UnitSize(view));

    bg_sidebar_ = ColorRef::constant(Color(0xE0E9F5, 1.0));
    bg_splitter_ = ColorRef::constant(Color(0x4F4F4f, 1.0));
    bg_epg_ = ColorRef::constant(Color(0xFFFFFF, 1.0));
    fg_epg_ = ColorRef::constant(Color(0x000000, 1.0));
    fg_epg_chan_ = ColorRef::constant(Color(0x3D3D3D, 1.0));
    bg_epg_tile_ = ColorRef::constant(Color(0xE1E1E1, 1.0));
    bg_epg_scrollbar_ = ColorRef::constant(Color(0xF9F9F9, 1.0));
    fg_epg_scrollbar_ = ColorRef::constant(Color(0xC0C0C0, 1.0));
    bg_epg_cancelled_ = ColorRef::constant(Color(0x000000, 1.0));
    bg_epg_rec_ = ColorRef::constant(Color(0xFF0000, 1.0));

    // FIXME: maybe use Qt to lookup system highlight color for selection,
    // and wrap it in an expression:
    bg_epg_sel_ = ColorRef::constant(Color(0x004080, 0.1));

    bg_epg_header_.reset(new TGradient());
    {
      TGradient & gradient = *bg_epg_header_;
      gradient[0.00] = Color(0xFFFFFF, 1.00);
      gradient[0.05] = Color(0xF0F0F0, 1.00);
      gradient[0.50] = Color(0xE1E1E1, 1.00);
      gradient[0.55] = Color(0xD0D0D0, 1.00);
      gradient[0.95] = Color(0xB9B9B9, 1.00);
      gradient[1.00] = Color(0x8E8E8E, 1.00);
    }

    bg_epg_shadow_.reset(new TGradient());
    {
      TGradient & gradient = *bg_epg_shadow_;
      gradient[0.00] = Color(0x000000, 0.50);
      gradient[0.33] = Color(0x000000, 0.20);
      gradient[1.00] = Color(0x000000, 0.00);
    }

    bg_epg_channel_.reset(new TGradient());
    {
      TGradient & gradient = *bg_epg_channel_;
      gradient[0.00] = Color(0xE9E9ED, 1.00);
      gradient[1.00] = Color(0xDCDBDF, 1.00);
    }

    // generate collapsed group button texture:
    collapsed_ = Item::addHidden<Texture>
      (new Texture("collapsed", QImage())).sharedPtr<Texture>();

    // generate expanded group button texture:
    expanded_ = Item::addHidden<Texture>
      (new Texture("expanded", QImage())).sharedPtr<Texture>();

    // generate trashcan texture:
    trashcan_ = Item::addHidden<Texture>
      (new Texture("trashcan", QImage())).sharedPtr<Texture>();
  }

  //----------------------------------------------------------------
  // AppStyle::uncache
  //
  void
  AppStyle::uncache()
  {
    bg_sidebar_.uncache();
    bg_splitter_.uncache();
    bg_epg_.uncache();
    fg_epg_.uncache();
    fg_epg_chan_.uncache();
    bg_epg_tile_.uncache();
    bg_epg_scrollbar_.uncache();
    fg_epg_scrollbar_.uncache();
    bg_epg_cancelled_.uncache();
    bg_epg_rec_.uncache();
    bg_epg_sel_.uncache();

    collapsed_->uncache();
    expanded_->uncache();
    trashcan_->uncache();

    ItemViewStyle::uncache();
  }

}