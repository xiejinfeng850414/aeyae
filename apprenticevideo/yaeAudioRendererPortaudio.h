// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb  5 21:57:57 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUDIO_RENDERER_PORTAUDIO_H_
#define YAE_AUDIO_RENDERER_PORTAUDIO_H_

// system includes:
#include <string>

// yae includes:
#include <yaeAPI.h>
#include <yaeAudioRenderer.h>
#include <yaeReader.h>


namespace yae
{
  //----------------------------------------------------------------
  // AudioRenderer
  // 
  struct YAE_API AudioRendererPortaudio : public IAudioRenderer
  {
  private:
    //! intentionally disabled:
    AudioRendererPortaudio(const AudioRendererPortaudio &);
    AudioRendererPortaudio & operator = (const AudioRendererPortaudio &);
    
    //! private implementation details:
    class TPrivate;
    TPrivate * private_;
    
  protected:
    AudioRendererPortaudio();
    ~AudioRendererPortaudio();
    
  public:
    static AudioRendererPortaudio * create();
    virtual void destroy();
    
    //! return a human readable name for this renderer (preferably unique):
    virtual const char * getName() const;

    //! there may be multiple audio rendering devices available:
    virtual unsigned int countAvailableDevices() const;
    
    //! return index of the system default audio rendering device:
    virtual unsigned int getDefaultDeviceIndex() const;
    
    //! get device name and max audio resolution capabilities:
    virtual bool getDeviceName(unsigned int deviceIndex,
                               std::string & deviceName) const;

    //! begin rendering audio frames from a given reader:
    virtual bool open(unsigned int deviceIndex,
                      IReader * reader);

    //! terminate audio rendering:
    virtual void close();
  };
}


#endif // YAE_AUDIO_RENDERER_PORTAUDIO_H_