// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SUBTITLES_TRACK_H_
#define YAE_SUBTITLES_TRACK_H_

// system includes:
#include <vector>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

// ffmpeg includes:
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

// yae includes:
#include "yae/ffmpeg/yae_track.h"
#include "yae/thread/yae_queue.h"
#include "yae/video/yae_video.h"


namespace yae
{

  //----------------------------------------------------------------
  // TSubsPrivate
  //
  class YAE_API TSubsPrivate : public TSubsFrame::IPrivate
  {
    // virtual:
    ~TSubsPrivate();

  public:
    TSubsPrivate(const AVSubtitle & sub,
                 const unsigned char * subsHeader,
                 std::size_t subsHeaderSize);

    // virtual:
    void destroy();

    // virtual:
    std::size_t headerSize() const;

    // virtual:
    const unsigned char * header() const;

    // virtual:
    unsigned int numRects() const;

    // virtual:
    void getRect(unsigned int i, TSubsFrame::TRect & rect) const;

    // helper:
    static TSubtitleType getType(const AVSubtitleRect * r);

    AVSubtitle sub_;
    std::vector<unsigned char> header_;
  };

  //----------------------------------------------------------------
  // TSubsPrivatePtr
  //
  typedef boost::shared_ptr<TSubsPrivate> TSubsPrivatePtr;


  //----------------------------------------------------------------
  // TVobSubSpecs
  //
  struct YAE_API TVobSubSpecs
  {
    TVobSubSpecs();

    void init(const unsigned char * extraData, std::size_t size);

    // reference frame origin and dimensions:
    int x_;
    int y_;
    int w_;
    int h_;

    double scalex_;
    double scaley_;
    double alpha_;

    // color palette:
    std::vector<std::string> palette_;
  };

  //----------------------------------------------------------------
  // TSubsFrameQueue
  //
  typedef Queue<TSubsFrame> TSubsFrameQueue;


  //----------------------------------------------------------------
  // SubtitlesTrack
  //
  struct YAE_API SubtitlesTrack : public Track
  {
    SubtitlesTrack(AVStream * stream = NULL);
    ~SubtitlesTrack();

    void clear();

    // virtual:
    AVCodecContext * open();

    void close();

    void fixupEndTime(double v1, TSubsFrame & prev, const TSubsFrame & next);
    void fixupEndTimes(double v1, const TSubsFrame & last);
    void expungeOldSubs(double v0);
    void get(double v0, double v1, std::list<TSubsFrame> & subs);
    void push(const TSubsFrame & sf, QueueWaitMgr * terminator);

  private:
    SubtitlesTrack(const SubtitlesTrack & given);
    SubtitlesTrack & operator = (const SubtitlesTrack & given);

  public:
    bool render_;
    TSubsFormat format_;

    TIPlanarBufferPtr extraData_;
    TSubsFrameQueue queue_;
    std::list<TSubsFrame> active_;
    TSubsFrame last_;

    TVobSubSpecs vobsub_;
  };

  //----------------------------------------------------------------
  // SubttTrackPtr
  //
  typedef boost::shared_ptr<SubtitlesTrack> SubttTrackPtr;

}


#endif // YAE_SUBTITLES_TRACK_H_
