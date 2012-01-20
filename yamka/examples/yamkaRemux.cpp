// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Nov  6 22:05:29 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaElt.h>
#include <yamkaPayload.h>
#include <yamkaStdInt.h>
#include <yamkaFileStorage.h>
#include <yamkaEBML.h>
#include <yamkaMatroska.h>

// system includes:
#include <sstream>
#include <iostream>
#include <string.h>
#include <string>
#include <time.h>
#include <map>

// namespace access:
using namespace Yamka;

// shortcuts:
typedef MatroskaDoc::TSegment TSegment;
typedef Segment::TInfo TSegInfo;
typedef Segment::TTracks TTracks;
typedef Segment::TSeekHead TSeekHead;
typedef SeekHead::TSeekEntry TSeekEntry;
typedef Segment::TCluster TCluster;
typedef Segment::TCues TCues;
typedef Segment::TTags TTags;
typedef Segment::TChapters TChapters;
typedef Segment::TAttachment TAttachment;
typedef Cues::TCuePoint TCuePoint;
typedef CuePoint::TCueTrkPos TCueTrkPos;
typedef Tags::TTag TTag;
typedef Tag::TTargets TTagTargets;
typedef Tag::TSimpleTag TSimpleTag;
typedef TagTargets::TTrackUID TTagTrackUID;
typedef Tracks::TTrack TTrack;
typedef Cluster::TBlockGroup TBlockGroup;
typedef Cluster::TSimpleBlock TSimpleBlock;
typedef Cluster::TEncryptedBlock TEncryptedBlock;
typedef Cluster::TSilent TSilentTracks;
typedef Chapters::TEdition TEdition;
typedef Edition::TChapAtom TChapAtom;
typedef ChapAtom::TDisplay TChapDisplay;

typedef std::deque<TSeekHead>::iterator TSeekHeadIter;
typedef std::list<TCluster>::iterator TClusterIter;
typedef std::list<TSeekEntry>::iterator TSeekEntryIter;
typedef std::list<TCuePoint>::iterator TCuePointIter;
typedef std::list<TCueTrkPos>::iterator TCueTrkPosIter;

typedef std::deque<TSeekHead>::const_iterator TSeekHeadConstIter;
typedef std::list<TCluster>::const_iterator TClusterConstIter;
typedef std::list<TSeekEntry>::const_iterator TSeekEntryConstIter;
typedef std::list<TCuePoint>::const_iterator TCuePointConstIter;
typedef std::list<TCueTrkPos>::const_iterator TCueTrkPosConstIter;

//----------------------------------------------------------------
// kMinShort
// 
static const short int kMinShort = std::numeric_limits<short int>::min();

//----------------------------------------------------------------
// kMaxShort
// 
static const short int kMaxShort = std::numeric_limits<short int>::max();

//----------------------------------------------------------------
// BE_QUIET
// 
#define BE_QUIET

//----------------------------------------------------------------
// NANOSEC_PER_SEC
// 
// 1 second, in nanoseconds
//
static const Yamka::uint64 NANOSEC_PER_SEC = 1000000000;

//----------------------------------------------------------------
// has
// 
template <typename TDataContainer, typename TData>
bool
has(const TDataContainer & container, const TData & value)
{
  typename TDataContainer::const_iterator found =
    std::find(container.begin(), container.end(), value);

  return found != container.end();
}

//----------------------------------------------------------------
// has
// 
template <typename TKey, typename TValue>
bool
has(const std::map<TKey, TValue> & keyValueMap, const TValue & key)
{
  typename std::map<TKey, TValue>::const_iterator found =
    keyValueMap.find(key);
  
  return found != keyValueMap.end();
}

//----------------------------------------------------------------
// endsWith
// 
static bool
endsWith(const std::string & str, const char * suffix)
{
  std::size_t strSize = str.size();
  std::string suffixStr(suffix);
  std::size_t suffixStrSize = suffixStr.size();
  std::size_t found = str.rfind(suffixStr);
  return (found == strSize - suffixStrSize);
}

//----------------------------------------------------------------
// toScalar
// 
template <typename TScalar>
static TScalar
toScalar(const char * text)
{
  std::istringstream iss;
  iss.str(std::string(text));
  
  TScalar v = (TScalar)0;
  iss >> v;
  
  return v;
}

//----------------------------------------------------------------
// toText
// 
template <typename TScalar>
static std::string
toText(TScalar v)
{
  std::ostringstream oss;
  oss << v;
  std::string text = oss.str().c_str();
  return text;
}

//----------------------------------------------------------------
// printCurrentTime
// 
static void
printCurrentTime(const char * msg)
{
  time_t rawtime = 0;
  time(&rawtime);
  
  struct tm * timeinfo = localtime(&rawtime);
  std::string s(asctime(timeinfo));
  s[s.size() - 1] = '\0';
  std::cout << "\n\n" << s.c_str() << " -- " << msg << std::endl;
}

//----------------------------------------------------------------
// usage
// 
static void
usage(char ** argv, const char * message = NULL)
{
  std::cerr << "NOTE: remuxing input files containing multiple segments "
            << "with mismatched tracks will not work correctly"
            << std::endl;
  
  std::cerr << "USAGE: " << argv[0]
            << " -i input.mkv -o output.mkv [-t trackNo | +t trackNo]* "
            << "[-t0 hh mm ss] [-t1 hh mm ss]"
            << std::endl;
  
  std::cerr << "EXAMPLE: " << argv[0]
            << " -i input.mkv -o output.mkv -t 1 -t 2"
            << " -t0 00 04 00 -t1 00 08 00"
            << std::endl;

  if (message != NULL)
  {
    std::cerr << "ERROR: " << message << std::endl;
  }
  
  ::exit(1);
}

//----------------------------------------------------------------
// usage
// 
inline static void
usage(char ** argv, const std::string & message)
{
  usage(argv, message.c_str());
}

//----------------------------------------------------------------
// getTrack
// 
static const Track *
getTrack(const std::deque<TTrack> & tracks, uint64 trackNo)
{
  for (std::deque<TTrack>::const_iterator i = tracks.begin();
       i != tracks.end(); ++i)
  {
    const Track & track = i->payload_;
    uint64 tn = track.trackNumber_.payload_.get();
    if (tn == trackNo)
    {
      return &track;
    }
  }

  return NULL;
}

//----------------------------------------------------------------
// ScopedVar
// 
template <typename T>
struct ScopedVar
{
  T & valRef_;
  T valOutOfScope_;
  
  ScopedVar(T & valRef, T valInScope):
    valRef_(valRef),
    valOutOfScope_(valRef)
  {
    valRef_ = valInScope;
  }

  ~ScopedVar()
  {
    valRef_ = valOutOfScope_;
  }
};

//----------------------------------------------------------------
// printProgress
// 
static void
printProgress(FileStorage & storage, uint64 storageSize)
{
  // print progress:
  uint64 pos = storage.file_.absolutePosition();
  double pct = 100.0 * (double(pos) / double(storageSize));
  printf("\r%3.6f%%  ", pct);
}

//----------------------------------------------------------------
// LoadWithProgress
// 
struct LoadWithProgress : public IDelegateLoad
{
  uint64 srcSize_;
  
  LoadWithProgress(uint64 srcSize = 1):
    srcSize_(srcSize)
  {}
  
  uint64 load(FileStorage & storage,
              uint64 payloadBytesToRead,
              uint64 eltId,
              IPayload & payload)
  {
    printProgress(storage, srcSize_);
    
    // let the generic load mechanism handle the actual loading:
    return 0;
  }
  
  void loaded(IElement & elt)
  {
    if (elt.getId() == TSilentTracks::kId)
    {
      // if the SilentTracks element was present in the stream
      // it must be saved to the output stream too,
      // even if it contained no tracks at all:
      elt.alwaysSave();
    }
  }
};

//----------------------------------------------------------------
// ClusterSkipReader
// 
// Load the first cluster, skip everything else.
// 
struct ClusterSkipReader : public LoadWithProgress
{
  ClusterSkipReader(uint64 srcSize):
    LoadWithProgress(srcSize)
  {}
  
  // virtual:
  uint64 load(FileStorage & storage,
              uint64 payloadBytesToRead,
              uint64 eltId,
              IPayload & payload)
  {
    LoadWithProgress::load(storage, payloadBytesToRead, eltId, payload);
    
    const bool payloadIsCluster = (eltId == TCluster::kId);
    const bool payloadIsCues = (eltId == TCues::kId);
    if (payloadIsCluster || payloadIsCues)
    {
      storage.file_.seek(payloadBytesToRead, File::kRelativeToCurrent);
      return payloadBytesToRead;
    }
    
    // let the generic load mechanism handle the actual loading:
    return 0;
  }
};

//----------------------------------------------------------------
// TTrackMap
// 
typedef std::map<uint64, uint64> TTrackMap;

//----------------------------------------------------------------
// TFifo
//
template <typename TLink>
struct TFifo
{
  TFifo():
    head_(NULL),
    tail_(NULL),
    size_(0)
  {}
  
  ~TFifo()
  {
    assert(!head_);
  }
  
  void push(TLink * link)
  {
    assert(!link->next_);
    
    if (!head_)
    {
      assert(!tail_);
      assert(!size_);
      head_ = link;
      tail_ = link;
      size_ = 1;
    }
    else
    {
      assert(tail_);
      assert(size_);
      tail_->next_ = link;
      tail_ = link;
      size_++;
    }
  }
  
  TLink * pop()
  {
    if (!head_)
    {
      return NULL;
    }
    
    TLink * link = head_;
    head_ = link->next_;
    size_--;
    
    if (!head_)
    {
      tail_ = NULL;
    }
    
    return link;
  }
  
  TLink * head_;
  TLink * tail_;
  std::size_t size_;
};

//----------------------------------------------------------------
// TBlockInfo
// 
struct TBlockInfo
{
  TBlockInfo():
    pts_(0),
    trackNo_(0),
    next_(NULL)
  {}
  
  inline HodgePodge * getBlockData()
  {
    if (sblockElt_.mustSave())
    {
      return &(sblockElt_.payload_.data_);
    }
    else if (bgroupElt_.mustSave())
    {
      return &(bgroupElt_.payload_.block_.payload_.data_);
    }
    else if (eblockElt_.mustSave())
    {
      return &(eblockElt_.payload_.data_);
    }

    assert(false);
    return NULL;
  }

  IStorage::IReceiptPtr save(FileStorage & storage)
  {
    if (sblockElt_.mustSave())
    {
      return sblockElt_.save(storage);
    }
    else if (bgroupElt_.mustSave())
    {
      return bgroupElt_.save(storage);
    }
    else if (eblockElt_.mustSave())
    {
      return eblockElt_.save(storage);
    }

    assert(false);
    return IStorage::IReceiptPtr();
  }

  TSilentTracks silentElt_;
  TSimpleBlock sblockElt_;
  TBlockGroup bgroupElt_;
  TEncryptedBlock eblockElt_;
  
  SimpleBlock block_;
  IStorage::IReceiptPtr header_;
  IStorage::IReceiptPtr frames_;
  
  uint64 pts_;
  uint64 trackNo_;
  bool keyframe_;
  
  TBlockInfo * next_;
};

//----------------------------------------------------------------
// TBlockFifo
// 
typedef TFifo<TBlockInfo> TBlockFifo;

//----------------------------------------------------------------
// TLace
// 
struct TLace
{
  TLace(std::size_t numTracks):
    tracks_(numTracks + 1),
    size_(0)
  {}
  
  void push(TBlockInfo * binfo)
  {
    TBlockFifo & track = tracks_[binfo->trackNo_];
    track.push(binfo);
    size_++;
  }

  TBlockInfo * pop()
  {
    TBlockFifo * nextTrack = NULL;
    uint64 earliestPTS = (uint64)(~0);

    std::size_t numTracks = tracks_.size();
    for (std::size_t i = 0; i < numTracks; i++)
    {
      TBlockFifo & track = tracks_[i];
      if (track.size_ && track.head_->pts_ < earliestPTS)
      {
        nextTrack = &track;
        earliestPTS = track.head_->pts_;
      }
    }

    if (!nextTrack)
    {
      return NULL;
    }

    size_--;
    return nextTrack->pop();
  }
  
  std::vector<TBlockFifo> tracks_;
  std::size_t size_;
};

//----------------------------------------------------------------
// TRemuxer
// 
struct TRemuxer : public LoadWithProgress
{
  TRemuxer(const TTrackMap & trackSrcDst,
           const TTrackMap & trackDstSrc,
           const TSegment & srcSeg,
           TSegment & dstSeg,
           FileStorage & src,
           FileStorage & dst,
           FileStorage & tmp);

  void remux();
  void flush();

  // Returns true if the given Block/SimpleBlock/EncryptedBlock
  // should be kept, returns false if the block should be discarded:
  bool isRelevant(uint64 clusterTime, TBlockInfo & binfo);
  
  // if necessary remaps the track number, adjusts blockTime,
  // keyframe flag, and stores the updated block header without
  // altering the (possibly) laced (or encrypted) block data:
  bool updateHeader(uint64 clusterTime, TBlockInfo & binfo);
  
  void push(uint64 clusterTime, TSilentTracks & silentElt);
  void push(uint64 clusterTime, TSimpleBlock & sblockElt);
  void push(uint64 clusterTime, TBlockGroup & bgroupElt);
  void push(uint64 clusterTime, TEncryptedBlock & eblockElt);
  
  void mux(std::size_t minLaceSize = 50);
  void startNextCluster(TBlockInfo * binfo);
  void finishCurrentCluster();
  void addCuePoint(TCluster * clusterElt, TBlockInfo * binfo);
  
  const TTrackMap & trackSrcDst_;
  const TTrackMap & trackDstSrc_;
  const TSegment & srcSeg_;
  TSegment & dstSeg_;
  FileStorage & src_;
  FileStorage & dst_;
  FileStorage & tmp_;
  uint64 clusterBlocks_;
  uint64 cuesTrackNo_;
  
  TLace lace_;
};

//----------------------------------------------------------------
// TRemuxer::TRemuxer
// 
TRemuxer::TRemuxer(const TTrackMap & trackSrcDst,
                   const TTrackMap & trackDstSrc,
                   const TSegment & srcSeg,
                   TSegment & dstSeg,
                   FileStorage & src,
                   FileStorage & dst,
                   FileStorage & tmp):
  LoadWithProgress(src.file_.size()),
  trackSrcDst_(trackSrcDst),
  trackDstSrc_(trackDstSrc),
  srcSeg_(srcSeg),
  dstSeg_(dstSeg),
  src_(src),
  dst_(dst),
  tmp_(tmp),
  lace_(trackSrcDst.size()),
  clusterBlocks_(0),
  cuesTrackNo_(0)
{
  TTracks & tracksElt = dstSeg.payload_.tracks_;
  std::deque<TTrack> & tracks = tracksElt.payload_.tracks_;
  
  std::size_t numTracks = tracks.size();
  for (std::size_t i = 0; i < numTracks; i++)
  {
    Track & track = tracks[i].payload_;
    uint64 trackType = track.trackType_.payload_.get();
    uint64 trackNo = track.trackNumber_.payload_.get();
    
    if (trackType == Track::kTrackTypeVideo)
    {
      cuesTrackNo_ = trackNo;
      break;
    }
    else if (trackType == Track::kTrackTypeAudio && !cuesTrackNo_)
    {
      cuesTrackNo_ = trackNo;
    }
  }

  if (!cuesTrackNo_ && numTracks)
  {
    cuesTrackNo_ = tracks[0].payload_.trackNumber_.payload_.get();
  }

  // add 2nd SeekHead:
  dstSeg_.payload_.seekHeads_.push_back(TSeekHead());
  TSeekHead & seekHeadElt = dstSeg_.payload_.seekHeads_.back();
  
  // index the 2nd SeekHead and Cues:
  SeekHead & seekHead1 = dstSeg_.payload_.seekHeads_.front().payload_;
  seekHead1.indexThis(&dstSeg_, &seekHeadElt, tmp_);
  seekHead1.indexThis(&dstSeg_, &(dstSeg_.payload_.cues_), tmp_);

  TAttachment & attachments = dstSeg_.payload_.attachments_;
  attachments = srcSeg_.payload_.attachments_;
  if (attachments.mustSave())
  {
    seekHead1.indexThis(&dstSeg_, &attachments, tmp_);
  }
  
  TChapters & chapters = dstSeg_.payload_.chapters_;
  chapters = srcSeg_.payload_.chapters_;
  if (chapters.mustSave())
  {
    seekHead1.indexThis(&dstSeg_, &chapters, tmp_);
  }
}

//----------------------------------------------------------------
// TRemuxer::remux
// 
void
TRemuxer::remux()
{
  const std::list<TCluster> & clusters = srcSeg_.payload_.clusters_;
  for (std::list<TCluster>::const_iterator i = clusters.begin();
       i != clusters.end(); ++i)
  {
    TCluster clusterElt = *i;

    uint64 position = clusterElt.storageReceipt()->position();
    uint64 numBytes = clusterElt.storageReceipt()->numBytes();
    clusterElt.discardReceipts();
    
    src_.file_.seek(position);
    uint64 bytesRead = clusterElt.load(src_, numBytes, this);
    assert(bytesRead == numBytes);

    Cluster & cluster = clusterElt.payload_;
    uint64 clusterTime = cluster.timecode_.payload_.get();

    // use SilentTracks as a cluster delimiter:
    if (cluster.silent_.mustSave())
    {
      push(clusterTime, cluster.silent_);
    }
    
    // iterate through simple blocks and push them into a lace:
    std::deque<TSimpleBlock> & sblocks = cluster.simpleBlocks_;
    for (std::deque<TSimpleBlock>::iterator i = sblocks.begin();
         i != sblocks.end(); ++i)
    {
      TSimpleBlock & sblockElt = *i;
      push(clusterTime, sblockElt);
    }
    
    // iterate through block groups and push them into a lace:
    std::deque<TBlockGroup> & bgroups = cluster.blockGroups_;
    for (std::deque<TBlockGroup>::iterator i = bgroups.begin();
         i != bgroups.end(); ++i)
    {
      TBlockGroup & bgroupElt = *i;
      push(clusterTime, bgroupElt);
    }
    
    // iterate through encrypted blocks and push them into a lace:
    std::deque<TEncryptedBlock> & eblocks = cluster.encryptedBlocks_;
    for (std::deque<TEncryptedBlock>::iterator i = eblocks.begin();
         i != eblocks.end(); ++i)
    {
      TEncryptedBlock & eblockElt = *i;
      push(clusterTime, eblockElt);
    }
  }
  
  flush();

  TSeekHead & seekHead2 = dstSeg_.payload_.seekHeads_.back();
  seekHead2.save(dst_);

  TCues & cues = dstSeg_.payload_.cues_;
  cues.save(dst_);

  TAttachment & attachments = dstSeg_.payload_.attachments_;
  if (attachments.mustSave())
  {
    attachments.save(dst_);
  }
  
  TChapters & chapters = dstSeg_.payload_.chapters_;
  if (chapters.mustSave())
  {
    chapters.save(dst_);
  }
}

//----------------------------------------------------------------
// TRemuxer::flush
// 
void
TRemuxer::flush()
{
  while (lace_.size_)
  {
    mux(lace_.size_);
  }
  
  finishCurrentCluster();
}

//----------------------------------------------------------------
// TRemuxer::isRelevant
// 
bool
TRemuxer::isRelevant(uint64 clusterTime, TBlockInfo & binfo)
{
  HodgePodge * blockData = binfo.getBlockData();
  if (!blockData)
  {
    return false;
  }
                     
  // check whether the given block is for a track we are interested in:
  uint64 bytesRead = binfo.block_.importData(*blockData);
  if (!bytesRead)
  {
    assert(false);
    return false;
  }
  
  uint64 srcTrackNo = binfo.block_.getTrackNumber();
  TTrackMap::const_iterator found = trackSrcDst_.find(srcTrackNo);
  if (found == trackSrcDst_.end())
  {
    return false;
  }
  
  const uint64 blockSize = blockData->numBytes();
  HodgePodgeConstIter blockDataIter(*blockData);
  binfo.header_ = blockDataIter.receipt(0, bytesRead);
  binfo.frames_ = blockDataIter.receipt(bytesRead, blockSize - bytesRead);
  
  binfo.trackNo_ = found->second;
  binfo.pts_ = clusterTime + binfo.block_.getRelativeTimecode();
  binfo.keyframe_ = binfo.block_.isKeyframe();
  
  return true;
}

//----------------------------------------------------------------
// TRemuxer::updateHeader
// 
bool
TRemuxer::updateHeader(uint64 clusterTime, TBlockInfo & binfo)
{
  int64 dstBlockTime = binfo.pts_ - clusterTime;
  assert(dstBlockTime >= kMinShort &&
         dstBlockTime <= kMaxShort);
  
  uint64 srcTrackNo = binfo.block_.getTrackNumber();
  short int srcBlockTime = binfo.block_.getRelativeTimecode();
  bool srcIsKeyframe = binfo.block_.isKeyframe();
  
  if (srcTrackNo == binfo.trackNo_ &&
      srcBlockTime == dstBlockTime &&
      srcIsKeyframe == binfo.keyframe_)
  {
    // nothing changed:
    return true;
  }
  
  HodgePodge * blockData = binfo.getBlockData();
  if (!blockData)
  {
    return false;
  }
  
  binfo.block_.setTrackNumber(binfo.trackNo_);
  binfo.block_.setRelativeTimecode((short int)(dstBlockTime));
  binfo.block_.setKeyframe(binfo.keyframe_);
  
  binfo.header_ = binfo.block_.writeHeader(tmp_);
  
  blockData->set(binfo.header_);
  blockData->add(binfo.frames_);
  
  return true;
}

//----------------------------------------------------------------
// TRemuxer::push
// 
void
TRemuxer::push(uint64 clusterTime, TSilentTracks & silentElt)
{
  TBlockInfo * info = new TBlockInfo();
  info->silentElt_ = silentElt;
  info->pts_ = clusterTime;
  lace_.push(info);
}

//----------------------------------------------------------------
// TRemuxer::push
// 
void
TRemuxer::push(uint64 clusterTime, TSimpleBlock & sblockElt)
{
  TBlockInfo * info = new TBlockInfo();
  info->sblockElt_ = sblockElt;
  
  if (!isRelevant(clusterTime, *info))
  {
    delete info;
    return;
  }

  lace_.push(info);
  mux();
}

//----------------------------------------------------------------
// TRemuxer::push
// 
void
TRemuxer::push(uint64 clusterTime, TBlockGroup & bgroupElt)
{
  TBlockInfo * info = new TBlockInfo();
  info->bgroupElt_ = bgroupElt;
  
  if (!isRelevant(clusterTime, *info))
  {
    delete info;
    return;
  }

  lace_.push(info);
  mux();
}

//----------------------------------------------------------------
// TRemuxer::push
// 
void
TRemuxer::push(uint64 clusterTime, TEncryptedBlock & eblockElt)
{
  TBlockInfo * info = new TBlockInfo();
  info->eblockElt_ = eblockElt;
  
  if (!isRelevant(clusterTime, *info))
  {
    delete info;
    return;
  }

  lace_.push(info);
  mux();
}

//----------------------------------------------------------------
// TRemuxer::mux
// 
void
TRemuxer::mux(std::size_t minLaceSize)
{
  if (lace_.size_ < minLaceSize)
  {
    return;
  }

  std::list<TCluster> & clusters = dstSeg_.payload_.clusters_;
  
  TBlockInfo * binfo = lace_.pop();
  if (!binfo->trackNo_ || clusters.empty())
  {
    startNextCluster(binfo);
  }

  TCluster * clusterElt = &(clusters.back());
  uint64 clusterTime = clusterElt->payload_.timecode_.payload_.get();
  int64 blockTime = binfo->pts_ - clusterTime;
  
  if (blockTime > kMaxShort)
  {
    startNextCluster(binfo);

    clusterElt = &(clusters.back());
    clusterTime = clusterElt->payload_.timecode_.payload_.get();
    blockTime = binfo->pts_ - clusterTime;
  }

  if (binfo->trackNo_ && updateHeader(clusterTime, *binfo))
  {
    Cluster & cluster = clusterElt->payload_;
    
    if (binfo->sblockElt_.mustSave())
    {
      cluster.simpleBlocks_.push_back(binfo->sblockElt_);
      binfo->sblockElt_ = cluster.simpleBlocks_.back();
    }
    else if (binfo->bgroupElt_.mustSave())
    {
      cluster.blockGroups_.push_back(binfo->bgroupElt_);
      binfo->bgroupElt_ = cluster.blockGroups_.back();
    }
    else if (binfo->eblockElt_.mustSave())
    {
      cluster.encryptedBlocks_.push_back(binfo->eblockElt_);
      binfo->eblockElt_ = cluster.encryptedBlocks_.back();
    }
    
    binfo->save(dst_);
    
    clusterBlocks_++;
    addCuePoint(clusterElt, binfo);
  }
  
  delete binfo;
}

//----------------------------------------------------------------
// TRemuxer::startNextCluster
// 
void
TRemuxer::startNextCluster(TBlockInfo * binfo)
{
  finishCurrentCluster();
  
  std::list<TCluster> & clusters = dstSeg_.payload_.clusters_;

  // start a new cluster:
  clusterBlocks_ = 0;
  clusters.push_back(TCluster());
  TCluster & clusterElt = clusters.back();
  
  // index the new cluster with the 2nd SeekHead:
  SeekHead & seekHead2 = dstSeg_.payload_.seekHeads_.back().payload_;
  seekHead2.indexThis(&dstSeg_, &clusterElt, tmp_);
  
  // set the start time:
  clusterElt.payload_.timecode_.payload_.set(binfo->pts_);

  if (!binfo->trackNo_ && binfo->silentElt_.mustSave())
  {
    // copy the SilentTracks, update track numbers to match:
    typedef SilentTracks::TTrack TTrkNumElt;
    typedef std::list<TTrkNumElt> TTrkNumElts;

    clusterElt.payload_.silent_.alwaysSave();
    
    const TTrkNumElts & srcSilentTracks =
      binfo->silentElt_.payload_.tracks_;

    TTrkNumElts & dstSilentTracks =
      clusterElt.payload_.silent_.payload_.tracks_;

    for (TTrkNumElts::const_iterator i = srcSilentTracks.begin();
         i != srcSilentTracks.end(); ++i)
    {
      const TTrkNumElt & srcTnElt = *i;
      uint64 srcTrackNo = srcTnElt.payload_.get();
      
      TTrackMap::const_iterator found = trackSrcDst_.find(srcTrackNo);
      if (found != trackSrcDst_.end())
      {
        dstSilentTracks.push_back(TTrkNumElt());
        TTrkNumElt & dstTnElt = dstSilentTracks.back();

        uint64 dstTrackNo = found->second;
        dstTnElt.payload_.set(dstTrackNo);
      }
    }
  }
  
  clusterElt.savePaddedUpToSize(dst_, uintMax[8]);
}

//----------------------------------------------------------------
// TRemuxed::finishCurrentCluster
// 
void
TRemuxer::finishCurrentCluster()
{
  std::list<TCluster> & clusters = dstSeg_.payload_.clusters_;
  if (clusters.empty())
  {
    return;
  }

  // fix the cluster size:
  TCluster & clusterElt = clusters.back();

  IStorage::IReceiptPtr receipt = clusterElt.storageReceipt();
  IStorage::IReceiptPtr payload = clusterElt.payloadReceipt();
  IStorage::IReceiptPtr payloadEnd = dst_.receipt();

  uint64 payloadSize = payloadEnd->position() - payload->position();
  uint64 elementIdSize = uintNumBytes(TCluster::kId);

  IStorage::IReceiptPtr payloadSizeReceipt =
    receipt->receipt(elementIdSize, 8);
  
  TByteVec v = vsizeEncode(payloadSize, 8);
  payloadSizeReceipt->save(Bytes(v));
}

//----------------------------------------------------------------
// TRemuxer::addCuePoint
// 
void
TRemuxer::addCuePoint(TCluster * clusterElt, TBlockInfo * binfo)
{
  if (!binfo->keyframe_ || binfo->trackNo_ != cuesTrackNo_)
  {
    return;
  }
  
  Cues & cues = dstSeg_.payload_.cues_.payload_;
  
  // add to cues:
  cues.points_.push_back(TCuePoint());
  TCuePoint & cuePoint = cues.points_.back();
  
  // set cue timepoint:
  cuePoint.payload_.time_.payload_.set(binfo->pts_);
  
  // add a track position for this timepoint:
  cuePoint.payload_.trkPosns_.resize(1);
  CueTrkPos & pos = cuePoint.payload_.trkPosns_.back().payload_;
  
  pos.block_.payload_.set(clusterBlocks_);
  pos.track_.payload_.set(binfo->trackNo_);
  
  pos.cluster_.payload_.setOrigin(&dstSeg_);
  pos.cluster_.payload_.setElt(clusterElt);
}

//----------------------------------------------------------------
// main
// 
int
main(int argc, char ** argv)
{
  printCurrentTime("start");
  
  std::string srcPath;
  std::string dstPath;
  std::string tmpPath;
  std::list<uint64> tracksToKeep;
  std::list<uint64> tracksDelete;

  uint64 t0 = 0;
  uint64 t1 = (uint64)(~0);
  
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-i") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "could not parse -i parameter");
      i++;
      srcPath.assign(argv[i]);
    }
    else if (strcmp(argv[i], "-o") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "could not parse -o parameter");
      i++;
      dstPath.assign(argv[i]);
      tmpPath = dstPath + std::string(".yamka");
    }
    else if (strcmp(argv[i], "-t") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "could not parse -t parameter");
      i++;

      if (!tracksToKeep.empty())
      {
        usage(argv, "-t and +t parameter can not be used together");
      }
      
      uint64 trackNo = toScalar<uint64>(argv[i]);
      if (trackNo == 0)
      {
        usage(argv, "could not parse -t parameter value");
      }
      else
      {
        tracksDelete.push_back(trackNo);
      }
    }
    else if (strcmp(argv[i], "+t") == 0)
    {
      if ((argc - i) <= 1) usage(argv, "could not parse +t parameter");
      i++;

      if (!tracksDelete.empty())
      {
        usage(argv, "-t and +t parameter can not be used together");
      }
      
      uint64 trackNo = toScalar<uint64>(argv[i]);
      if (trackNo == 0)
      {
        usage(argv, "could not parse +t parameter value");
      }
      else
      {
        tracksToKeep.push_back(trackNo);
      }
    }
    else if (strcmp(argv[i], "-t0") == 0)
    {
      if ((argc - i) <= 3) usage(argv, "could not parse -t0 parameter");
      
      i++;
      uint64 hh = toScalar<uint64>(argv[i]);
      
      i++;
      uint64 mm = toScalar<uint64>(argv[i]);
      
      i++;
      uint64 ss = toScalar<uint64>(argv[i]);
      
      t0 = ss + 60 * (mm + 60 * hh);
    }
    else if (strcmp(argv[i], "-t1") == 0)
    {
      if ((argc - i) <= 3) usage(argv, "could not parse -t1 parameter");
      
      i++;
      uint64 hh = toScalar<uint64>(argv[i]);
      
      i++;
      uint64 mm = toScalar<uint64>(argv[i]);
      
      i++;
      uint64 ss = toScalar<uint64>(argv[i]);
      
      t1 = ss + 60 * (mm + 60 * hh);
    }
    else
    {
      usage(argv, (std::string("unknown option: ") +
                   std::string(argv[i])).c_str());
    }
  }
  
  if (t0 > t1)
  {
    usage(argv,
          "start time (-t0 hh mm ss) is greater than "
          "finish time (-t1 hh mm ss)");
  }
  
  bool keepAllTracks = tracksToKeep.empty() && tracksDelete.empty();
  
  FileStorage src(srcPath, File::kReadOnly);
  if (!src.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 srcPath +
                 std::string(" for reading")).c_str());
  }
  
  uint64 srcSize = src.file_.size();
  MatroskaDoc doc;
  ClusterSkipReader skipClusters(srcSize);

  // attempl to load via SeekHead(s):
  bool ok = doc.loadSeekHead(src, srcSize);
  printCurrentTime("doc.loadSeekHead finished");
  
  if (ok)
  {
    ok = doc.loadViaSeekHead(src, &skipClusters, true);
    printCurrentTime("doc.loadViaSeekHead finished");
  }

  if (!ok ||
      !doc.segments_.empty() &&
      doc.segments_.front().payload_.clusters_.empty())
  {
    std::cout << "failed to find Clusters via SeekHead, "
              << "attempting brute force"
              << std::endl;
    
    doc = MatroskaDoc();
    src.file_.seek(0);
    
    doc.loadAndKeepReceipts(src, srcSize, &skipClusters);
    printCurrentTime("doc.loadAndKeepReceipts finished");
  }

  if (doc.segments_.empty())
  {
    usage(argv, (std::string("failed to load any matroska segments").c_str()));
  }
  
  std::size_t numSegments = doc.segments_.size();
  std::vector<std::map<uint64, uint64> > trackSrcDst(numSegments);
  std::vector<std::map<uint64, uint64> > trackDstSrc(numSegments);
  std::vector<std::vector<std::list<Frame> > > segmentTrackFrames(numSegments);
  
  // verify that the specified tracks exist:
  std::size_t segmentIndex = 0;
  for (std::list<TSegment>::iterator i = doc.segments_.begin();
       i != doc.segments_.end(); ++i, ++segmentIndex)
  {
    std::map<uint64, uint64> & trackInOut = trackSrcDst[segmentIndex];
    std::map<uint64, uint64> & trackOutIn = trackDstSrc[segmentIndex];
    
    const Segment & segment = i->payload_;
    
    const std::deque<TTrack> & tracks = segment.tracks_.payload_.tracks_;
    for (std::deque<TTrack>::const_iterator j = tracks.begin();
         j != tracks.end(); ++j)
    {
      const Track & track = j->payload_;
      uint64 trackNo = track.trackNumber_.payload_.get();
      uint64 trackNoOut = trackInOut.size() + 1;
      
      if (!tracksToKeep.empty() && has(tracksToKeep, trackNo))
      {
        trackInOut[trackNo] = trackNoOut;
        trackOutIn[trackNoOut] = trackNo;
        std::cout
          << "segment " << segmentIndex + 1
          << ", mapping output track " << trackNoOut
          << " to input track " << trackNo
          << std::endl;
      }
      else if (!tracksDelete.empty() && !has(tracksDelete, trackNo))
      {
        trackInOut[trackNo] = trackNoOut;
        trackOutIn[trackNoOut] = trackNo;
        std::cout
          << "segment " << segmentIndex + 1
          << ", mapping output track " << trackNoOut
          << " to input track " << trackNo
          << std::endl;
      }
      else if (keepAllTracks)
      {
        trackInOut[trackNo] = trackNoOut;
        trackOutIn[trackNoOut] = trackNo;
        std::cout
          << "segment " << segmentIndex + 1
          << ", mapping output track " << trackNoOut
          << " to input track " << trackNo
          << std::endl;
      }
    }
    
    if (trackOutIn.empty())
    {
      usage(argv,
            std::string("segment ") +
            toText(segmentIndex + 1) +
            std::string(", none of the specified input tracks exist"));
    }
    
    std::size_t numTracks = trackInOut.size();
    segmentTrackFrames[segmentIndex].resize(numTracks);
  }
  
  FileStorage dst(dstPath, File::kReadWrite);
  if (!dst.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 dstPath +
                 std::string(" for writing")).c_str());
  }
  
  FileStorage tmp(tmpPath, File::kReadWrite);
  if (!tmp.file_.isOpen())
  {
    usage(argv, (std::string("failed to open ") +
                 tmpPath +
                 std::string(" for writing")).c_str());
  }
  else
  {
    tmp.file_.setSize(0);
  }
  
  MatroskaDoc out;
  dst.file_.setSize(0);
  out.save(dst);
  
  segmentIndex = 0;
  for (std::list<TSegment>::iterator i = doc.segments_.begin();
       i != doc.segments_.end(); ++i, ++segmentIndex)
  {
    printCurrentTime("parse next segment");

    TSegment segmentElt;
    segmentElt.setFixedSize(uintMax[8]);
    // out.segments_.push_back(TSegment());
    // TSegment & segmentElt = out.segments_.back();
    
    Segment & segment = segmentElt.payload_;
    const Segment & segmentIn = i->payload_;
    
    // add first SeekHead, written before clusters:
    segment.seekHeads_.push_back(TSeekHead());
    TSeekHead & seekHeadElt = segment.seekHeads_.back();
    seekHeadElt.setFixedSize(1024);
    SeekHead & seekHead = seekHeadElt.payload_;
    
    // copy segment info:
    TSegInfo & segInfoElt = segment.info_;
    SegInfo & segInfo = segInfoElt.payload_;
    const SegInfo & segInfoIn = segmentIn.info_.payload_;
    
    segInfo = segInfoIn;
    segInfo.muxingApp_.payload_.set(segInfo.muxingApp_.payload_.getDefault());
    segInfo.writingApp_.payload_.set("yamkaRemux");

    // segment timecode scale, such that
    // timeInNanosec := timecodeScale * (clusterTime + blockTime):
    uint64 timecodeScale = segInfo.timecodeScale_.payload_.get();
    
    // copy track info:
    TTracks & tracksElt = segment.tracks_;
    std::deque<TTrack> & tracks = tracksElt.payload_.tracks_;
    const std::deque<TTrack> & tracksIn = segmentIn.tracks_.payload_.tracks_;

    std::map<uint64, uint64> & trackInOut = trackSrcDst[segmentIndex];
    std::map<uint64, uint64> & trackOutIn = trackDstSrc[segmentIndex];
    uint64 trackMapSize = trackOutIn.size();

    std::vector<Track::MatroskaTrackType> trackType((std::size_t)trackMapSize);
    uint64 videoTrackNo = 0;
    
    for (uint64 trackNo = 1; trackNo <= trackMapSize; trackNo++)
    {
      tracks.push_back(TTrack());
      Track & track = tracks.back().payload_;
      
      uint64 trackNoIn = trackOutIn[trackNo];
      const Track * trackIn = getTrack(tracksIn, trackNoIn);
      if (trackIn)
      {
        track = *trackIn;
      }
      else
      {
        assert(false);
      }
      
      track.trackNumber_.payload_.set(trackNo);
      track.flagLacing_.payload_.set(1);

      trackType[(std::size_t)(trackNo - 1)] =
        (Track::MatroskaTrackType)(track.trackType_.payload_.get());
      
      if (!videoTrackNo &&
          trackType[(std::size_t)(trackNo - 1)] == Track::kTrackTypeVideo)
      {
        videoTrackNo = trackNo;
      }
    }

    // index the first set of top-level elements:
    seekHead.indexThis(&segmentElt, &segInfoElt, tmp);
    seekHead.indexThis(&segmentElt, &tracksElt, tmp);
    
    // minor cleanup prior to saving:
    RemoveVoids().eval(segmentElt);
    
    // discard previous storage receipts:
    DiscardReceipts().eval(segmentElt);
    
    // save a placeholder:
    IStorage::IReceiptPtr segmentReceipt = segmentElt.save(dst);
    
    printCurrentTime("begin segment remux");

    // on-the-fly remux Clusters/BlockGroups/SimpleBlocks:
    TRemuxer remuxer(trackInOut, trackOutIn, *i, segmentElt, src, dst, tmp);
    remuxer.remux();

    printCurrentTime("finished segment remux");
    
    // rewrite the 1st SeekHead:
    {
      TSeekHead & seekHead = segmentElt.payload_.seekHeads_.front();
      IStorage::IReceiptPtr receipt = seekHead.storageReceipt();
      
      File::Seek autoRestorePosition(dst.file_);
      dst.file_.seek(receipt->position());
      
      seekHead.save(dst);
    }
    
    // rewrite element position references (second pass):
    RewriteReferences().eval(segmentElt);

    // rewrite the segment payload size:
    {
      IStorage::IReceiptPtr receipt = segmentElt.storageReceipt();
      IStorage::IReceiptPtr payload = segmentElt.payloadReceipt();
      IStorage::IReceiptPtr payloadEnd = dst.receipt();

      uint64 payloadSize = payloadEnd->position() - payload->position();
      uint64 elementIdSize = uintNumBytes(TSegment::kId);
      
      IStorage::IReceiptPtr payloadSizeReceipt =
        receipt->receipt(elementIdSize, 8);
      
      TByteVec v = vsizeEncode(payloadSize, 8);
      payloadSizeReceipt->save(Bytes(v));
    }
  }

  // close open file handles:
  src.file_.close();
  dst.file_.close();
  tmp.file_.close();
  
  // remove temp file:
  File::remove(tmpPath.c_str());

  printCurrentTime("done");

  // avoid waiting for all the destructors to be called:
  ::exit(0);
  
  doc = MatroskaDoc();
  out = WebmDoc(kFileFormatMatroska);
  
  return 0;
}

//  +t 1 +t 2 +t 7
