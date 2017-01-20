// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <map>
#include <set>

// boost:
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

// yae includes:
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/ffmpeg/yae_track.h"

// namespace shortcuts:
namespace al = boost::algorithm;


namespace yae
{

  //----------------------------------------------------------------
  // AvPkt::AvPkt
  //
  AvPkt::AvPkt()
  {
    memset(this, 0, sizeof(AVPacket));
    av_init_packet(this);
  }

  //----------------------------------------------------------------
  // AvPkt::AvPkt
  //
  AvPkt::AvPkt(const AvPkt & pkt)
  {
    memset(this, 0, sizeof(AVPacket));
    av_packet_ref(this, &pkt);
  }

  //----------------------------------------------------------------
  // AvPkt::~AvPkt
  //
  AvPkt::~AvPkt()
  {
    av_packet_unref(this);
  }

  //----------------------------------------------------------------
  // AvPkt::operator =
  //
  AvPkt &
  AvPkt::operator = (const AvPkt & pkt)
  {
    av_packet_unref(this);
    av_packet_ref(this, &pkt);
    return *this;
  }


  //----------------------------------------------------------------
  // AvFrm::AvFrm
  //
  AvFrm::AvFrm()
  {
    memset(this, 0, sizeof(AVFrame));
    av_frame_unref(this);
  }

  //----------------------------------------------------------------
  // AvFrm::AvFrm
  //
  AvFrm::AvFrm(const AvFrm & frame)
  {
    memset(this, 0, sizeof(AVFrame));
    av_frame_ref(this, &frame);
  }

  //----------------------------------------------------------------
  // AvFrm::~AvFrm
  //
  AvFrm::~AvFrm()
  {
    av_frame_unref(this);
  }

  //----------------------------------------------------------------
  // AvFrm::operator
  //
  AvFrm &
  AvFrm::operator = (const AvFrm & frame)
  {
    av_frame_unref(this);
    av_frame_ref(this, &frame);
    return *this;
  }


  //----------------------------------------------------------------
  // is_framerate_valid
  //
  static inline bool
  is_framerate_valid(const AVRational & r)
  {
    return r.num > 0 && r.den > 0;
  }

  //----------------------------------------------------------------
  // is_frame_duration_plausible
  //
  static bool
  is_frame_duration_plausible(const TTime & dt, const AVRational & frame_rate)
  {
    int64 d = dt.getTime(frame_rate.num) / frame_rate.den;
    int64 err = d - frame_rate.den;

    if (frame_rate.den < std::abs(d))
    {
      // error is more than the expected frame duration:
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // verify_pts
  //
  // verify that presentation timestamps are monotonically increasing
  //
  bool
  verify_pts(bool hasPrevPTS,
             const TTime & prevPTS,
             const TTime & nextPTS,
             const AVStream * stream,
             const char * debugMessage)
  {
    bool ok = (nextPTS.time_ != AV_NOPTS_VALUE &&
               nextPTS.base_ != AV_NOPTS_VALUE &&
               nextPTS.base_ != 0 &&
               (!hasPrevPTS ||
                (prevPTS.base_ == nextPTS.base_ ?
                 prevPTS.time_ < nextPTS.time_ :
                 prevPTS.toSeconds() < nextPTS.toSeconds())));
#if 0
    if (ok && debugMessage)
    {
      std::cerr << "PTS OK: "
                << nextPTS.time_ << "/" << nextPTS.base_
                << ", " << debugMessage << std::endl;
    }
#else
    (void)debugMessage;
#endif

    if (!ok || !stream)
    {
      return ok;
    }

    // sanity check -- verify the timestamp difference
    // is close to frame duration:
    if (hasPrevPTS)
    {
      bool has_avg_frame_rate = is_framerate_valid(stream->avg_frame_rate);
      bool has_r_frame_rate = is_framerate_valid(stream->r_frame_rate);

      if (has_avg_frame_rate || has_r_frame_rate)
      {
        TTime dt = nextPTS - prevPTS;

        if (!((has_avg_frame_rate &&
               is_frame_duration_plausible(dt, stream->avg_frame_rate)) ||
              (has_r_frame_rate &&
               is_frame_duration_plausible(dt, stream->r_frame_rate))))
        {
          // error is more than the expected and estimated frame duration:
#ifndef NDEBUG
          std::cerr
            << "\nNOTE: detected large error in frame duration, dt: "
            << dt.toSeconds() << " sec";

          if (has_avg_frame_rate)
          {
            std::cerr
              << ", expected (avg): "
              << TTime(stream->avg_frame_rate.den,
                       stream->avg_frame_rate.num).toSeconds()
              << " sec";
          }

          if (has_r_frame_rate)
          {
            std::cerr
              << ", expected (r): "
              << TTime(stream->r_frame_rate.den,
                       stream->r_frame_rate.num).toSeconds()
              << " sec";
          }

          if (debugMessage)
          {
            std::cerr << ", " << debugMessage;
          }

          std::cerr << std::endl;
#endif
          return false;
        }
      }
    }

    // another possible sanity check -- verify that timestamp is
    // within [start, end] range, although that might be too strict
    // because duration is often estimated from bitrate
    // and is therefore inaccurate

    return true;
  }



  //----------------------------------------------------------------
  // Track::Track
  //
  Track::Track(AVFormatContext * context, AVStream * stream):
    thread_(this),
    context_(context),
    stream_(stream),
    codec_(NULL),
    codecContext_(NULL),
    packetQueue_(kQueueSizeLarge),
    timeIn_(0.0),
    timeOut_(kMaxDouble),
    playbackEnabled_(false),
    startTime_(0),
    tempo_(1.0),
    discarded_(0)
  {
    if (context_ && stream_)
    {
      YAE_ASSERT(context_->streams[stream_->index] == stream_);
    }
  }

  //----------------------------------------------------------------
  // Track::Track
  //
  Track::Track(Track & track):
    thread_(this),
    context_(NULL),
    stream_(NULL),
    codec_(NULL),
    codecContext_(NULL),
    packetQueue_(kQueueSizeLarge),
    timeIn_(0.0),
    timeOut_(kMaxDouble),
    playbackEnabled_(false),
    startTime_(0),
    tempo_(1.0),
    discarded_(0)
  {
    std::swap(context_, track.context_);
    std::swap(stream_, track.stream_);
    std::swap(codec_, track.codec_);
    std::swap(codecContext_, track.codecContext_);
  }

  //----------------------------------------------------------------
  // Track::~Track
  //
  Track::~Track()
  {
    threadStop();
    close();
  }

  //----------------------------------------------------------------
  // TDecoderMap
  //
  typedef std::map<AVCodecID, std::set<const AVCodec *> > TDecoderMap;

  //----------------------------------------------------------------
  // TDecoders
  //
  struct TDecoders : public TDecoderMap
  {

    //----------------------------------------------------------------
    // TDecoders
    //
    TDecoders()
    {
      for (const AVCodec * c = av_codec_next(NULL); c; c = av_codec_next(c))
      {
        if (av_codec_is_decoder(c))
        {
          TDecoderMap::operator[](c->id).insert(c);
        }
      }
    }

    //----------------------------------------------------------------
    // AVCodecContextDeallocator
    //
    struct AVCodecContextDeallocator
    {
      inline static void destroy(AVCodecContext * ctx)
      {
        avcodec_close(ctx);
        avcodec_free_context(&ctx);
      }
    };

    //----------------------------------------------------------------
    // find
    //
    const AVCodec *
    find(const AVCodecParameters & params) const
    {
      TDecoderMap::const_iterator found = TDecoderMap::find(params.codec_id);
      if (found == TDecoderMap::end())
      {
        return NULL;
      }

      const AVCodec * experimental = NULL;
      const AVCodec * software = NULL;
      const AVCodec * hardware = NULL;

      typedef std::set<const AVCodec *> TCodecs;
      const TCodecs & codecs = found->second;
      for (TCodecs::const_iterator i = codecs.begin(); i != codecs.end(); ++i)
      {
        const AVCodec * c = *i;
        if (c->capabilities & AV_CODEC_CAP_EXPERIMENTAL)
        {
          experimental || (experimental = c);
        }
        else if (al::ends_with(c->name, "_cuvid"))
        {
          if (hardware)
          {
            continue;
          }

          if (params.format != AV_PIX_FMT_YUV420P &&
              params.codec_id != AV_CODEC_ID_MJPEG)
          {
            // 4:2:0 is the only one that works with h264_cuvud and mpeg2_cuvid
            // however, 4:2:2 and 4:4:4 work with mjpeg_cuvid
            continue;
          }

          // verify that the GPU can handle this stream:
          boost::shared_ptr<AVCodecContext>
            ctx(avcodec_alloc_context3(c), AVCodecContextDeallocator::destroy);
          avcodec_parameters_to_context(ctx.get(), &params);

          int err = avcodec_open2(ctx.get(), c, NULL);
          if (err)
          {
            continue;
          }

          hardware = c;
        }
        else
        {
          software || (software = c);
        }
      }

      return (hardware ? hardware :
              software ? software :
              experimental);
    }
  };

  //----------------------------------------------------------------
  // find_best_decoder_for
  //
  const AVCodec *
  find_best_decoder_for(const AVCodecParameters & params)
  {
    static const TDecoders decoders;
    const AVCodec * c = decoders.find(params);

#ifdef NDEBUG
    if (c)
    {
      std::cerr << "\n\nUSING DECODER: " << c->name << "\n\n"
                << std::endl;
    }
#endif

    return c;
  }


  //----------------------------------------------------------------
  // Track::open
  //
  bool
  Track::open()
  {
    if (!stream_)
    {
      return false;
    }

    threadStop();
    close();

    codec_ = find_best_decoder_for(*(stream_->codecpar));
    if (!codec_ && stream_->codecpar->codec_id != AV_CODEC_ID_TEXT)
    {
      // unsupported codec:
      return false;
    }

    // maybe try opening the decoder on-demand when we have a packet
    int err = 0;
    if (codec_)
    {
      YAE_ASSERT(!codecContext_);
      codecContext_ = avcodec_alloc_context3(codec_);

      avcodec_parameters_to_context(codecContext_, stream_->codecpar);

      err = avcodec_open2(codecContext_, codec_, NULL);
    }

    if (err < 0)
    {
      // unsupported codec:
      avcodec_free_context(&codecContext_);
      codec_ = NULL;
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // Track::close
  //
  void
  Track::close()
  {
    if (stream_ && codecContext_)
    {
      avcodec_close(codecContext_);
      avcodec_free_context(&codecContext_);
      codec_ = NULL;
    }
  }

  //----------------------------------------------------------------
  // Track::getName
  //
  const char *
  Track::getName() const
  {
    return stream_ ? getTrackName(stream_->metadata) : NULL;
  }

  //----------------------------------------------------------------
  // Track::getLang
  //
  const char *
  Track::getLang() const
  {
    return stream_ ? getTrackLang(stream_->metadata) : NULL;
  }

  //----------------------------------------------------------------
  // Track::getDuration
  //
  bool
  Track::getDuration(TTime & start, TTime & duration) const
  {
    if (!stream_)
    {
      YAE_ASSERT(false);
      return false;
    }

    if (stream_->duration != int64_t(AV_NOPTS_VALUE))
    {
      // return track duration:
      start.base_ = stream_->time_base.den;
      start.time_ =
        stream_->start_time != int64_t(AV_NOPTS_VALUE) ?
        stream_->time_base.num * stream_->start_time : 0;

      duration.time_ = stream_->time_base.num * stream_->duration;
      duration.base_ = stream_->time_base.den;
      return true;
    }

    if (!context_)
    {
      YAE_ASSERT(false);
      return false;
    }

    if (context_->duration != int64_t(AV_NOPTS_VALUE))
    {
      // track duration is unknown, return movie duration instead:
      start.base_ = AV_TIME_BASE;
      start.time_ =
        context_->start_time != int64_t(AV_NOPTS_VALUE) ?
        context_->start_time : 0;

      duration.time_ = context_->duration;
      duration.base_ = AV_TIME_BASE;
      return true;
    }

    int64 fileSize = avio_size(context_->pb);
    int64 fileBits = fileSize * 8;

    start.base_ = AV_TIME_BASE;
    start.time_ = 0;

    if (context_->bit_rate)
    {
      double t =
        double(fileBits / context_->bit_rate) +
        double(fileBits % context_->bit_rate) /
        double(context_->bit_rate);

      duration.time_ = int64_t(0.5 + t * double(AV_TIME_BASE));
      duration.base_ = AV_TIME_BASE;
      return true;
    }

    if (context_->nb_streams == 1 && codecContext_->bit_rate)
    {
      double t =
        double(fileBits / codecContext_->bit_rate) +
        double(fileBits % codecContext_->bit_rate) /
        double(codecContext_->bit_rate);

      duration.time_ = int64_t(0.5 + t * double(AV_TIME_BASE));
      duration.base_ = AV_TIME_BASE;
      return true;
    }

    // unknown duration:
    duration.time_ = std::numeric_limits<int64>::max();
    duration.base_ = AV_TIME_BASE;
    return false;
  }

  //----------------------------------------------------------------
  // Track::threadStart
  //
  bool
  Track::threadStart()
  {
    terminator_.stopWaiting(false);
    packetQueue_.open();
    return thread_.run();
  }

  //----------------------------------------------------------------
  // Track::threadStop
  //
  bool
  Track::threadStop()
  {
    terminator_.stopWaiting(true);
    packetQueue_.close();
    thread_.stop();
    return thread_.wait();
  }

  //----------------------------------------------------------------
  // Track::setTempo
  //
  bool
  Track::setTempo(double tempo)
  {
    boost::lock_guard<boost::mutex> lock(tempoMutex_);
    tempo_ = tempo;
    return true;
  }
}
