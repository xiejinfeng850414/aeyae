// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun May  1 13:23:52 MDT 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// std includes:
#include <iostream>
#include <sstream>

// Qt includes:
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QPainter>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <QFontMetrics>
#include <QTime>

// yae includes:
#include <yaeTimelineControls.h>


namespace yae
{

  //----------------------------------------------------------------
  // kSeparatorForFrameNumber
  //
  static const char * kSeparatorForFrameNumber = ":";

  //----------------------------------------------------------------
  // kSeparatorForCentiSeconds
  //
  static const char * kSeparatorForCentiSeconds = ".";

  //----------------------------------------------------------------
  // kClockTemplate
  //
  static QString kClockTemplate = QString::fromUtf8("00:00:00:00");

  //----------------------------------------------------------------
  // kUnknownDuration
  //
  static QString kUnknownDuration = QString::fromUtf8("--:--:--:--");

  //----------------------------------------------------------------
  // getTimeStamp
  //
  static QString
  getTimeStamp(double seconds, double frameRate, const char * frameNumSep)
  {
    // round to nearest frame:
    double fpsWhole = ceil(frameRate);
    seconds = (seconds * fpsWhole + 0.5) / fpsWhole;

    double secondsWhole = floor(seconds);
    double remainder = seconds - secondsWhole;
    double frame = remainder * fpsWhole;
    uint64 frameNo = int(frame);

    int sec = int(seconds);
    int min = sec / 60;
    int hour = min / 60;

    sec %= 60;
    min %= 60;

#if 0
    std::cerr << "frame: " << frameNo
              << "\tseconds: " << seconds
              << ", remainder: " << remainder
              << ", fps " << frameRate
              << ", fps (whole): " << fpsWhole
              << ", " << frameRate * remainder
              << ", " << frame
              << std::endl;
#endif

    std::ostringstream os;
    os << std::setw(2) << std::setfill('0') << hour << ':'
       << std::setw(2) << std::setfill('0') << min << ':'
       << std::setw(2) << std::setfill('0') << sec << frameNumSep
       << std::setw(2) << std::setfill('0') << frameNo;

    std::string str(os.str().c_str());
    const char * text = str.c_str();
#if 0
    // crop the leading zeros up to s:ff
    const char * tend = text + 7;

    while (text < tend)
    {
      if (*text != '0' && *text != ':')
      {
        break;
      }

      ++text;
    }
#endif

    return QString::fromUtf8(text);
  }

  //----------------------------------------------------------------
  // TimelineControls::TimelineControls
  //
  TimelineControls::TimelineControls():
    ignoreClockStopped_(false),
    unknownDuration_(false),
    timelineStart_(0.0),
    timelineDuration_(0.0),
    timelinePosition_(0.0),
    frameRate_(100.0),
    slideshowTimer_(this),
    auxPlayhead_(kClockTemplate),
    auxDuration_(kUnknownDuration)
  {
    frameNumberSeparator_ = kSeparatorForCentiSeconds;

    padding_ = 8;
    lineWidth_ = 3;

    // setup marker positions:
    markerTimeIn_ = timelineStart_;
    markerTimeOut_ = timelineStart_ + timelineDuration_;
    markerPlayhead_ = timelineStart_;

    // current state of playback controls:
    currentState_ = TimelineControls::kIdle;

    slideshowTimer_.setSingleShot(true);
    slideshowTimer_.setInterval(1000);

    bool ok = connect(&slideshowTimer_, SIGNAL(timeout()),
                      this, SLOT(slideshowTimerExpired()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // TimelineControls::~TimelineControls
  //
  TimelineControls::~TimelineControls()
  {}

  //----------------------------------------------------------------
  // TimelineControls::timelineStart
  //
  double
  TimelineControls::timelineStart() const
  {
    return timelineStart_;
  }

  //----------------------------------------------------------------
  // TimelineControls::timelineDuration
  //
  double
  TimelineControls::timelineDuration() const
  {
    return timelineDuration_;
  }

  //----------------------------------------------------------------
  // TimelineControls::timeIn
  //
  double
  TimelineControls::timeIn() const
  {
    double seconds = markerTimeIn_ * timelineDuration_ + timelineStart_;
    return seconds;
  }

  //----------------------------------------------------------------
  // TimelineControls::timeOut
  //
  double
  TimelineControls::timeOut() const
  {
    double seconds = markerTimeOut_ * timelineDuration_ + timelineStart_;
    return seconds;
  }

  //----------------------------------------------------------------
  // TimelineControls::currentTime
  //
  double
  TimelineControls::currentTime() const
  {
      double seconds = markerPlayhead_ * timelineDuration_ + timelineStart_;
      return seconds;
  }

  //----------------------------------------------------------------
  // TimelineControls::noteCurrentTimeChanged
  //
  void
  TimelineControls::noteCurrentTimeChanged(const SharedClock & c,
                                           const TTime & currentTime)
  {
    (void) c;

    bool postThePayload = payload_.set(currentTime);
    if (postThePayload)
    {
      // send an event:
      qApp->postEvent(this, new TimelineEvent(payload_));
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::noteTheClockHasStopped
  //
  void
  TimelineControls::noteTheClockHasStopped(const SharedClock & c)
  {
    if (!ignoreClockStopped_)
    {
      qApp->postEvent(this, new ClockStoppedEvent(c));
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::ignoreClockStoppedEvent
  //
  void
  TimelineControls::ignoreClockStoppedEvent(bool ignore)
  {
    ignoreClockStopped_ = ignore;
  }

  //----------------------------------------------------------------
  // TimelineControls::observe
  //
  void
  TimelineControls::observe(const SharedClock & sharedClock)
  {
    sharedClock_.setObserver(NULL);
    sharedClock_ = sharedClock;
    sharedClock_.setObserver(this);
  }

  //----------------------------------------------------------------
  // TimelineControls::resetFor
  //
  void
  TimelineControls::resetFor(IReader * reader)
  {
    TTime start;
    TTime duration;
    if (!reader->getAudioDuration(start, duration))
    {
      reader->getVideoDuration(start, duration);
    }

    frameRate_ = 100.0;
    frameNumberSeparator_ = kSeparatorForCentiSeconds;

    VideoTraits videoTraits;
    if (reader->getVideoTraits(videoTraits) &&
        videoTraits.frameRate_ < 100.0)
    {
      frameRate_ = videoTraits.frameRate_;
      frameNumberSeparator_ = kSeparatorForFrameNumber;
    }

    timelineStart_ = start.toSeconds();
    timelinePosition_ = timelineStart_;

    unknownDuration_ = (!reader->isSeekable() ||
                        duration.time_ == std::numeric_limits<int64>::max());

    timelineDuration_ = (unknownDuration_ ?
                         std::numeric_limits<double>::max() :
                         duration.toSeconds());

    QString ts = getTimeStamp(timelineStart_,
                              frameRate_,
                              frameNumberSeparator_);
    updateAuxPlayhead(ts);

    ts = getTimeStamp(timelineStart_ + timelineDuration_,
                      frameRate_,
                      frameNumberSeparator_);
    updateAuxDuration(ts);

    updateMarkerPlayhead(0.0);
    updateMarkerTimeIn(0.0);
    updateMarkerTimeOut(1.0);

    // update();
  }

  //----------------------------------------------------------------
  // TimelineControls::adjustTo
  //
  void
  TimelineControls::adjustTo(IReader * reader)
  {
    // get current in/out/playhead positions in seconds:
    double t0 = timeIn();
    double t1 = timeOut();
    double t = currentTime();

    TTime start;
    TTime duration;
    if (!reader->getAudioDuration(start, duration))
    {
      reader->getVideoDuration(start, duration);
    }

    frameRate_ = 100.0;
    frameNumberSeparator_ = kSeparatorForCentiSeconds;

    VideoTraits videoTraits;
    if (reader->getVideoTraits(videoTraits) &&
        videoTraits.frameRate_ < 100.0)
    {
      frameRate_ = videoTraits.frameRate_;
      frameNumberSeparator_ = kSeparatorForFrameNumber;
    }

    timelineStart_ = start.toSeconds();
    timelineDuration_ = duration.toSeconds();
    timelinePosition_ = timelineStart_;

    // shortcuts:
    double dT = timelineDuration_;
    double T0 = timelineStart_;
    double T1 = T0 + dT;

    t0 = std::max<double>(T0, std::min<double>(T1, t0));
    t1 = std::max<double>(T0, std::min<double>(T1, t1));
    t = std::max<double>(T0, std::min<double>(T1, t));

    QString ts_p = getTimeStamp(t, frameRate_, frameNumberSeparator_);
    updateAuxPlayhead(ts_p);

    QString ts_d = getTimeStamp(T1, frameRate_, frameNumberSeparator_);
    updateAuxDuration(ts_d);

    updateMarkerPlayhead((t - T0) / dT);
    updateMarkerTimeIn((t0 - T0) / dT);
    updateMarkerTimeOut((t1 - T0) / dT);

    // update();
  }

  //----------------------------------------------------------------
  // TimelineControls::setAuxPlayhead
  //
  void
  TimelineControls::setAuxPlayhead(const QString & playhead)
  {
    seekTo(playhead);
  }

  //----------------------------------------------------------------
  // TimelineControls::setMarkerTimeIn
  //
  void
  TimelineControls::setMarkerTimeIn(double marker)
  {
    if (updateMarkerTimeIn(marker))
    {
      if (markerTimeOut_ < markerTimeIn_)
      {
        setMarkerTimeOut(markerTimeIn_);
      }

      double seconds = (timelineDuration_ * markerTimeIn_ + timelineStart_);
      emit moveTimeIn(seconds);
     }
  }

  //----------------------------------------------------------------
  // TimelineControls::setMarkerTimeOut
  //
  void
  TimelineControls::setMarkerTimeOut(double marker)
  {
    if (updateMarkerTimeOut(marker))
    {
      if (markerTimeIn_ > markerTimeOut_)
      {
        setMarkerTimeIn(markerTimeOut_);
      }

      double seconds = (timelineDuration_ * markerTimeOut_ + timelineStart_);
      emit moveTimeOut(seconds);
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::setMarkerPlayhead
  //
  void
  TimelineControls::setMarkerPlayhead(double marker)
  {
    if (!timelineDuration_ || unknownDuration_)
    {
      return;
    }

    if (updateMarkerPlayhead(marker))
    {
      double seconds = markerPlayhead_ * timelineDuration_ + timelineStart_;
      emit movePlayHead(seconds);
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::setInPoint
  //
  void
  TimelineControls::setInPoint()
  {
    if (!unknownDuration_ && currentState_ == kIdle)
    {
      setMarkerTimeIn(markerPlayhead_);
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::setOutPoint
  //
  void
  TimelineControls::setOutPoint()
  {
    if (!unknownDuration_ && currentState_ == kIdle)
    {
      setMarkerTimeOut(markerPlayhead_);
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::seekFromCurrentTime
  //
  void
  TimelineControls::seekFromCurrentTime(double secOffset)
  {
    double t0 = currentTime();
    if (t0 > 1e-1)
    {
      double seconds = std::max<double>(0.0, t0 + secOffset);
#if 0
      std::cerr << "seek from " << TTime(t0).to_hhmmss_usec(":")
                << " " << secOffset << " seconds to "
                << TTime(seconds).to_hhmmss_usec(":")
                << std::endl;
#endif
      seekTo(seconds);
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::seekTo
  //
  void
  TimelineControls::seekTo(double seconds)
  {
    if (!timelineDuration_ || unknownDuration_)
    {
      return;
    }

    double marker = (seconds - timelineStart_) / timelineDuration_;
    setMarkerPlayhead(marker);
  }

  //----------------------------------------------------------------
  // parseTimeCode
  //
  // NOTE: returns the number of fields parsed
  //
  static unsigned int
  parseTimeCode(QString timecode,
                int & hh,
                int & mm,
                int & ss,
                int & ff)
  {
    // parse the timecode right to left 'ff' 1st, 'ss' 2nd, 'mm' 3rd, 'hh' 4th:
    unsigned int parsed = 0;

    // default separator:
    const char separator = ':';

    // convert white-space to default separator:
    timecode = timecode.simplified();
    timecode.replace(' ', separator);

    QStringList	hh_mm_ss_ff = timecode.split(':', QString::SkipEmptyParts);
    int nc = hh_mm_ss_ff.size();

    if (!nc)
    {
      return 0;
    }

    // the right-most token is a frame number:
    QString last = hh_mm_ss_ff.back();
    hh_mm_ss_ff.pop_back();
    ff = last.toInt();
    parsed++;

    int hhmmss[3];
    hhmmss[0] = 0;
    hhmmss[1] = 0;
    hhmmss[2] = 0;

    for (int i = 0; i < 3 && !hh_mm_ss_ff.empty(); i++)
    {
      QString t = hh_mm_ss_ff.back();
      hh_mm_ss_ff.pop_back();

      hhmmss[2 - i] = t.toInt();
      parsed++;
    }

    hh = hhmmss[0];
    mm = hhmmss[1];
    ss = hhmmss[2];

    return parsed;
  }

  //----------------------------------------------------------------
  // TimelineControls::seekTo
  //
  bool
  TimelineControls::seekTo(const QString & hhmmssff)
  {
    int hh = 0;
    int mm = 0;
    int ss = 0;
    int ff = 0;

    unsigned int parsed = parseTimeCode(hhmmssff, hh, mm, ss, ff);
    if (!parsed)
    {
      return false;
    }

    double seconds = ss + 60.0 * (mm + 60.0 * hh);
    double msec = double(ff) / frameRate_;

    seekTo(seconds + msec);
    return true;
  }

  //----------------------------------------------------------------
  // TimelineControls::slideshowTimerExpired
  //
  void
  TimelineControls::slideshowTimerExpired()
  {
    while (!stoppedClock_.empty())
    {
      const SharedClock & c = stoppedClock_.front();
      if (sharedClock_.sharesCurrentTimeWith(c))
      {
#ifndef NDEBUG
        std::cerr
          << "NOTE: clock stopped"
          << std::endl;
#endif
        emit clockStopped(c);
      }
#ifndef NDEBUG
      else
      {
        std::cerr
          << "NOTE: ignoring stale delayed stopped clock"
          << std::endl;
      }
#endif

      stoppedClock_.pop_front();
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::event
  //
  bool
  TimelineControls::event(QEvent * e)
  {
    if (e->type() == QEvent::User)
    {
      TimelineEvent * timeChangedEvent = dynamic_cast<TimelineEvent *>(e);
      if (timeChangedEvent)
      {
        timeChangedEvent->accept();

        TTime currentTime;
        timeChangedEvent->payload_.get(currentTime);
        timelinePosition_ = currentTime.toSeconds();

        double t = timelinePosition_ - timelineStart_;
        updateMarkerPlayhead(t / timelineDuration_);

        return true;
      }

      ClockStoppedEvent * clockStoppedEvent =
        dynamic_cast<ClockStoppedEvent *>(e);
      if (clockStoppedEvent)
      {
        const SharedClock & c = clockStoppedEvent->clock_;
        if (sharedClock_.sharesCurrentTimeWith(c))
        {
          stoppedClock_.push_back(c);

          if (unknownDuration_ || timelineDuration_ * frameRate_ < 2.0)
          {
            // this is probably a slideshow, delay it:
            if (!slideshowTimer_.isActive())
            {
              slideshowTimer_.start();
              slideshowTimerStart_.start();
            }
            else if (slideshowTimerStart_.elapsed() >
                     slideshowTimer_.interval() * 2)
            {
#ifndef NDEBUG
              std::cerr << "STOPPED CLOCK TIMEOUT WAS LATE" << std::endl;
#endif
              slideshowTimerExpired();
            }
          }
          else
          {
            slideshowTimerExpired();
          }

          return true;
        }

#ifndef NDEBUG
        std::cerr
          << "NOTE: ignoring stale ClockStoppedEvent"
          << std::endl;
#endif
        return true;
      }
    }

    return QQuickItem::event(e);
  }

  //----------------------------------------------------------------
  // TimelineControls::updateAuxPlayhead
  //
  bool
  TimelineControls::updateAuxPlayhead(const QString & playhead)
  {
    if (playhead == auxPlayhead_)
    {
      return false;
    }

    auxPlayhead_ = playhead;
    emit auxPlayheadChanged();

    return true;
  }

  //----------------------------------------------------------------
  // TimelineControls::updateAuxDuration
  //
  bool
  TimelineControls::updateAuxDuration(const QString & duration)
  {
    if (duration == auxDuration_)
    {
      return false;
    }

    auxDuration_ = duration;
    emit auxDurationChanged();

    return true;
  }

  //----------------------------------------------------------------
  // TimelineControls::updateMarkerTimeIn
  //
  bool
  TimelineControls::updateMarkerTimeIn(double marker)
  {
    double t = std::min(1.0, std::max(0.0, marker));
    if (t == markerTimeIn_)
    {
      return false;
    }

    markerTimeIn_ = t;
    emit markerTimeInChanged();

    return true;
  }

  //----------------------------------------------------------------
  // TimelineControls::updateMarkerTimeOut
  //
  bool
  TimelineControls::updateMarkerTimeOut(double marker)
  {
    double t = std::min(1.0, std::max(0.0, marker));
    if (t == markerTimeOut_)
    {
      return false;
    }

    markerTimeOut_ = t;
    emit markerTimeOutChanged();

    return true;
  }

  //----------------------------------------------------------------
  // TimelineControls::updateMarkerPlayhead
  //
  bool
  TimelineControls::updateMarkerPlayhead(double marker)
  {
    double t = std::min(1.0, std::max(0.0, marker));
    if (t == markerPlayhead_)
    {
      return false;
    }

    markerPlayhead_ = t;
    emit markerPlayheadChanged();

    double seconds = markerPlayhead_ * timelineDuration_ + timelineStart_;
    QString ts = getTimeStamp(seconds, frameRate_, frameNumberSeparator_);
    updateAuxPlayhead(ts);

    return true;
  }

}
