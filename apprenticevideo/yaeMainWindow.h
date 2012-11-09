// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Dec 18 17:50:01 MST 2010
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MAIN_WINDOW_H_
#define YAE_MAIN_WINDOW_H_

// Qt includes:
#include <QDialog>
#include <QMainWindow>
#include <QMenu>
#include <QSignalMapper>
#include <QShortcut>
#include <QTimer>

// yae includes:
#include <yaeAPI.h>
#include <yaeCanvas.h>
#include <yaeReader.h>
#include <yaeAudioRenderer.h>
#include <yaeVideoRenderer.h>
#include <yaeTimelineControls.h>
#ifdef __APPLE__
#include <yaeAppleRemoteControl.h>
#endif

// local includes:
#include "ui_yaeAbout.h"
#include "ui_yaeAspectRatioDialog.h"
#include "ui_yaeMainWindow.h"
#include "ui_yaeOpenUrlDialog.h"


namespace yae
{

  //----------------------------------------------------------------
  // AboutDialog
  //
  class AboutDialog : public QDialog,
                      public Ui::AboutDialog
  {
    Q_OBJECT;

  public:
    AboutDialog(QWidget * parent = 0, Qt::WFlags f = 0);
  };


  //----------------------------------------------------------------
  // AspectRatioDialog
  //
  class AspectRatioDialog : public QDialog,
                            public Ui::AspectRatioDialog
  {
    Q_OBJECT;

  public:
    AspectRatioDialog(QWidget * parent = 0, Qt::WFlags f = 0);
  };


  //----------------------------------------------------------------
  // OpenUrlDialog
  //
  class OpenUrlDialog : public QDialog,
                        public Ui::OpenUrlDialog
  {
    Q_OBJECT;

  public:
    OpenUrlDialog(QWidget * parent = 0, Qt::WFlags f = 0);
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

    // accessor to the OpenGL rendering canvas:
    Canvas * canvas() const;

  protected:
    // open a movie file for playback:
    bool load(const QString & path);

  public:
    // specify a playlist of files to load:
    void setPlaylist(const std::list<QString> & playlist,
                     bool beginPlaybackImmediately = true);

  signals:
    void setInPoint();
    void setOutPoint();

  public slots:
    // file menu:
    void fileOpen();
    void fileOpenURL();
    void fileOpenFolder();
    void fileExit();

    // playback menu:
    void playbackAspectRatioAuto();
    void playbackAspectRatioOther();
    void playbackAspectRatio2_40();
    void playbackAspectRatio2_35();
    void playbackAspectRatio1_85();
    void playbackAspectRatio1_78();
    void playbackAspectRatio1_60();
    void playbackAspectRatio1_33();
    void playbackCropFrameNone();
    void playbackCropFrame2_40();
    void playbackCropFrame2_35();
    void playbackCropFrame1_85();
    void playbackCropFrame1_78();
    void playbackCropFrame1_60();
    void playbackCropFrame1_33();
    void playbackCropFrameAutoDetect();
    void playbackNext();
    void playbackPrev();
    void playbackLoop();
    void playbackVerticalScaling();
    void playbackColorConverter();
    void playbackLoopFilter();
    void playbackNonReferenceFrames();
    void playbackShowPlaylist();
    void playbackShowTimeline();
    void playbackShrinkWrap();
    void playbackFullScreen();
    void playbackSetTempo(int percent);

    // helper:
    void exitFullScreen();
    void togglePlayback();

    // audio/video menus:
    void audioSelectDevice(const QString & audioDevice);
    void audioSelectTrack(int index);
    void videoSelectTrack(int index);
    void subsSelectTrack(int index);
    void skipToChapter(int index);
    void playlistItemChanged(std::size_t index);

    // window menu:
    void windowHalfSize();
    void windowFullSize();
    void windowDoubleSize();

    // help menu:
    void helpAbout();

    // helpers:
    void processDropEventUrls(const QList<QUrl> & urls);
    void userIsSeeking(bool seeking);
    void moveTimeIn(double seconds);
    void moveTimeOut(double seconds);
    void movePlayHead(double seconds);
    void populateAudioDeviceMenu();
    void focusChanged(QWidget * prev, QWidget * curr);
    void playbackFinished();
    void playbackStop();
    void playback(bool forward = true);
    void scrollWheelTimerExpired();
    void playlistVisibilityChanged(bool visible);
    void fixupNextPrev();
    void adjustCanvasHeight();
    void canvasSizeBackup();
    void canvasSizeRestore();

  protected:
    // virtual:
    bool event(QEvent * e);
    void wheelEvent(QWheelEvent * e);
    void closeEvent(QCloseEvent * e);
    void dragEnterEvent(QDragEnterEvent * e);
    void dropEvent(QDropEvent * e);
    void keyPressEvent(QKeyEvent * e);
    void mousePressEvent(QMouseEvent * e);

    // helpers:
    void canvasSizeSet(double xexpand, double yexpand);
    void stopRenderers();
    void prepareReaderAndRenderers(IReader * reader, bool oneFrame = false);
    void resumeRenderers();
    void renderOneFrame();
    void selectVideoTrack(IReader * reader, std::size_t videoTrackIndex);
    void selectAudioTrack(IReader * reader, std::size_t audioTrackIndex);
    void selectSubsTrack(IReader * reader, std::size_t subsTrackIndex);
    void adjustMenues(IReader * reader);
    unsigned int adjustAudioTraitsOverride(IReader * reader);

    static void autoCropCallback(void * context, const TCropFrame & cf);

#ifdef __APPLE__
    // for Apple Remote:
    static void appleRemoteControlObserver(void * observerContext,
                                           TRemoteControlButtonId buttonId,
                                           bool pressedDown,
                                           unsigned int clickCount,
                                           bool heldDown);
    void * appleRemoteControl_;
#endif

    // context sensitive menu which includes most relevant actions:
    QMenu * contextMenu_;

    // shortcuts used during full-screen mode (when menubar is invisible)
    QShortcut * shortcutExit_;
    QShortcut * shortcutFullScreen_;
    QShortcut * shortcutShowPlaylist_;
    QShortcut * shortcutShowTimeline_;
    QShortcut * shortcutPlay_;
    QShortcut * shortcutNext_;
    QShortcut * shortcutPrev_;
    QShortcut * shortcutLoop_;
    QShortcut * shortcutCropNone_;
    QShortcut * shortcutAutoCrop_;

    // playlist shortcuts:
    QAction * actionRemove_;
    QAction * actionSelectAll_;
    QShortcut * shortcutRemove_;
    QShortcut * shortcutSelectAll_;

    // audio device selection widget:
    QActionGroup * audioDeviceGroup_;
    QSignalMapper * audioDeviceMapper_;

    // audio/video track selection widgets:
    QActionGroup * audioTrackGroup_;
    QActionGroup * videoTrackGroup_;
    QActionGroup * subsTrackGroup_;

    QSignalMapper * audioTrackMapper_;
    QSignalMapper * videoTrackMapper_;
    QSignalMapper * subsTrackMapper_;
    QSignalMapper * chapterMapper_;

    // file reader:
    IReader * reader_;
    unsigned int readerId_;

    // frame canvas:
    Canvas * canvas_;

    // audio device:
    std::string audioDevice_;

    // audio renderer:
    IAudioRenderer * audioRenderer_;

    // video renderer:
    VideoRenderer * videoRenderer_;

    // a flag indicating whether playback is paused:
    bool playbackPaused_;
    bool playbackInterrupted_;

    // scroll-wheel timer:
    QTimer scrollWheelTimer_;
    double scrollStart_;
    double scrollOffset_;

    // dialog for opening a URL resource:
    OpenUrlDialog * openUrl_;

    // shrink wrap stretch factors:
    double xexpand_;
    double yexpand_;

    // desired playback tempo:
    double tempo_;

    // selected track info is used to select matching track(s)
    // after loading next file in the playlist:
    TTrackInfo selVideo_;
    VideoTraits selVideoTraits_;

    TTrackInfo selAudio_;
    AudioTraits selAudioTraits_;

    TTrackInfo selSubs_;
    TSubsFormat selSubsFormat_;

    // auto-crop single shot timer:
    QTimer autocropTimer_;
  };
}


#endif // YAE_MAIN_WINDOW_H_
