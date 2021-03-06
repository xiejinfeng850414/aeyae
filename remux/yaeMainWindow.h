// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan 13 15:53:35 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MAIN_WINDOW_H_
#define YAE_MAIN_WINDOW_H_

// Qt:
#include <QDialog>
#include <QMainWindow>
#include <QMenu>
#include <QSignalMapper>
#include <QShortcut>
#include <QTimer>

// aeyae:
#include "yae/ffmpeg/yae_demuxer.h"
#include "yae/thread/yae_task_runner.h"

// local includes:
#ifdef __APPLE__
#include "yaeAppleUtils.h"
#endif
#include "yaeCanvasWidget.h"
#include "yaeDemuxerView.h"
#include "yaeRemux.h"
#include "yaeSpinnerView.h"

// Qt uic generated files:
#include "ui_yaeAbout.h"
#include "ui_yaeMainWindow.h"


namespace yae
{
  // forward declarations:
  class MainWindow;

  //----------------------------------------------------------------
  // TCanvasWidget
  //
#if defined(YAE_USE_QOPENGL_WIDGET)
  typedef CanvasWidget<QOpenGLWidget> TCanvasWidget;
#else
  typedef CanvasWidget<QGLWidget> TCanvasWidget;
#endif

  //----------------------------------------------------------------
  // AboutDialog
  //
  class AboutDialog : public QDialog,
                      public Ui::AboutDialog
  {
    Q_OBJECT;

  public:
    AboutDialog(QWidget * parent = 0);
  };

  //----------------------------------------------------------------
  // MainWindow
  //
  class MainWindow : public QMainWindow,
                     public Ui::yaeMainWindow
  {
    Q_OBJECT;

  public:
    MainWindow();
    ~MainWindow();

    void initCanvasWidget();
    void initItemViews();

    // accessor to the player widget:
    inline TCanvasWidget * canvasWidget() const
    { return canvasWidget_; }

    // accessor to the OpenGL rendering canvas:
    Canvas * canvas() const;

  protected:
    bool load(const QString & path);

  public:
    void add(const std::set<std::string> & sources,
             const std::list<yae::ClipInfo> & clips =
             std::list<yae::ClipInfo>());

  signals:
    void setInPoint();
    void setOutPoint();

  public slots:
    // file menu:
    void fileOpen();
    void fileOpen(const QString & filename);
    void fileSave();
    void fileSave(const QString & filename);
    void fileSaveAs();
    void fileImport();
    void fileExport();
    void fileExit();

    // help menu:
    void helpAbout();

    // helpers:
    void requestToggleFullScreen();
    void toggleFullScreen();
    void enterFullScreen();
    void exitFullScreen();
    void processDropEventUrls(const QList<QUrl> & urls);
    void swapShortcuts();

  protected:
    // virtual:
    bool event(QEvent * e);
    void closeEvent(QCloseEvent * e);
    void dragEnterEvent(QDragEnterEvent * e);
    void dropEvent(QDropEvent * e);
    void keyPressEvent(QKeyEvent * e);
    void mousePressEvent(QMouseEvent * e);

    // context sensitive menu which includes most relevant actions:
    QMenu * contextMenu_;

    // shortcuts used during full-screen mode (when menubar is invisible)
    QShortcut * shortcutSave_;
    QShortcut * shortcutExit_;
    QShortcut * shortcutFullScreen_;

    // frame canvas:
    TCanvasWidget * canvasWidget_;
    Canvas * canvas_;

    RemuxModel model_;
    RemuxView view_;
    SpinnerView spinner_;

    // load files on a background thread, etc...
    typedef yae::shared_ptr<AsyncTaskQueue::Task> TAsyncTaskPtr;
    std::list<TAsyncTaskPtr> tasks_;
    AsyncTaskQueue async_;

    // for recalling places where files were loaded or saved:
    std::map<std::string, QString> startHere_;

    // remember the filename selected for Open or Save As:
    QString filename_;
  };
}


#endif // YAE_MAIN_WINDOW_H_
