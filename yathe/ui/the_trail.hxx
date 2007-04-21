// File         : the_trail.hxx
// Author       : Paul A. Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : GPL.
// Description  : event trail recoring/playback (regression testing).

#ifndef THE_TRAIL_HXX_
#define THE_TRAIL_HXX_

// local includes:
#include "eh/the_mouse_device.hxx"
#include "eh/the_keybd_device.hxx"
#include "eh/the_wacom_device.hxx"
#include "utils/the_bit_tree.hxx"

// system includes:
#include <fstream>
#include <iostream>

// namespace stuff:
using std::istream;
using std::ostream;
using std::ifstream;
using std::ofstream;


//----------------------------------------------------------------
// THE_TRAIL
// 
#define THE_TRAIL (*the_trail_t::trail_)


//----------------------------------------------------------------
// the_trail_t
//
class the_trail_t
{
public:
  the_trail_t(int & argc, char ** argv, bool record_by_default = false);
  virtual ~the_trail_t();
  
  // accessors to the input devices: 
  inline const the_mouse_t & mouse() const { return mouse_; }
  inline const the_keybd_t & keybd() const { return keybd_; }
  inline const the_wacom_t & wacom() const { return wacom_; }
  
  inline the_mouse_t & mouse() { return mouse_; }
  inline the_keybd_t & keybd() { return keybd_; }
  inline the_wacom_t & wacom() { return wacom_; }
  
  // a timer will call this periodically to let the trail know to load
  // the next event from the trail:
  virtual void timeout() {}
  
protected:
  // default implementations are no-op:
  virtual void replay() {}
  virtual void replay_one() {}
  
  // close the replay stream:
  virtual void replay_done();

public:
  // stop the event trail replay:
  virtual void stop()
  {
    replay_done();
  }
  
  // milestone accessor:
  void next_milestone_achieved();
  
  //----------------------------------------------------------------
  // milestone_t
  // 
  class milestone_t
  {
  public:
    milestone_t()
    {
      THE_TRAIL.next_milestone_achieved();
    }
    
    ~milestone_t()
    {
      THE_TRAIL.next_milestone_achieved();
    }
  };
  
  // This flag controls whether the event recording will be done
  // even when the user didn't ask for it with the -record switch.
  // It may be usefull for debugging purposes
  bool record_by_default_;
  
  // The in/out stream used for trail replay/record:
  ifstream replay_stream;
  ofstream record_stream;
  
  // the input devices (reflect the current state of the device):
  the_mouse_t mouse_;
  the_keybd_t keybd_;
  the_wacom_t wacom_;
  
  // the trail line number that is currently being read/executed:
  unsigned int line_num_;
  
  // the current application milestone marker:
  unsigned int milestone_;
  
  // this flag controls the trail playback interactivity:
  bool single_step_replay_;
  
  // these flags will be set to true periodically:
  bool dont_save_events_;
  bool dont_post_events_;
  
  // maximum number of seconds the trail replay engine should wait for
  // a matching milestone marker before declaring a trail out of sequence;
  // default wait time indefinite (max unsigned int value):
  unsigned int seconds_to_wait_;
  
  // A single instance of the trail object:
  static the_trail_t * trail_;
};


//----------------------------------------------------------------
// load_address
// 
extern bool load_address(istream & si, uint64_t & address);

//----------------------------------------------------------------
// save_address
// 
extern void save_address(ostream & so, uint64_t address);

//----------------------------------------------------------------
// operator >>
// 
extern istream & operator >> (istream & si, uint64_t & address);

//----------------------------------------------------------------
// operator <<
// 
extern ostream & operator << (ostream & so, const uint64_t & address);


#endif // THE_TRAIL_HXX_
