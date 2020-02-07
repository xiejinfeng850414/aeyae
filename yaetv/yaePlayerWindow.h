// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Feb  2 21:27:04 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYER_WINDOW_H_
#define YAE_PLAYER_WINDOW_H_

// Qt:
#include <QWidget>

// local:
#include "yaeAppView.h"
#include "yaePlayerWidget.h"

// uic:
#include "ui_yaePlayerWindow.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlayerWindow
  //
  class PlayerWindow : public QWidget,
                       public Ui::yaePlayerWindow
  {
    Q_OBJECT;

  public:
    PlayerWindow(QWidget * parent = NULL,
                 Qt::WindowFlags f = Qt::WindowFlags());

    void playback(const TRecordingPtr & rec_ptr,
                  const IReaderPtr & reader,
                  TCanvasWidget * sharedCtx = NULL);

  protected:
    // virtual:
    void changeEvent(QEvent * e);
    void closeEvent(QCloseEvent * e);
    void keyPressEvent(QKeyEvent * e);

    PlayerWidget * playerWidget_;
  };
}


#endif // YAE_PLAYER_WINDOW_H_