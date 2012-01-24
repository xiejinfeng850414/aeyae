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
// LoaderSkipClusterPayload
// 
// Skip cluster payload, let the generic mechanism load everything else
// 
struct LoaderSkipClusterPayload : public LoadWithProgress
{
  LoaderSkipClusterPayload(uint64 srcSize):
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
    duration_(0),
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
  uint64 duration_;
  uint64 trackNo_;
  
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
  TLace():
    cuesTrackNo_(0),
    size_(0)
  {}
  
  void push(TBlockInfo * binfo)
  {
    TBlockFifo & track = track_[binfo->trackNo_];
    track.push(binfo);
    size_++;
  }

  TBlockInfo * pop()
  {
    std::size_t numTracks = track_.size();
    
    bool cuesTrackHasData =
      cuesTrackNo_ < numTracks &&
      track_[cuesTrackNo_].size_;
    
    uint64 bestTrackNo = cuesTrackHasData ? cuesTrackNo_ : numTracks;
    uint64 bestTrackTime = nextTime(cuesTrackNo_);

    uint64 trackTime = (uint64)(~0);
    for (std::size_t i = 0; i < numTracks; i++)
    {
      if (i != cuesTrackNo_ && (trackTime = nextTime(i)) < bestTrackTime)
      {
        bestTrackNo = i;
        bestTrackTime = trackTime;
      }
    }

    if (bestTrackNo < numTracks)
    {
      TBlockInfo * binfo = track_[bestTrackNo].pop();
      size_--;
      
      if (!binfo->duration_)
      {
        Track::MatroskaTrackType trackType = trackType_[bestTrackNo];
        uint64 defaultDuration = defaultDuration_[bestTrackNo];
        
        if (defaultDuration)
        {
          double frameDuration =
            double(defaultDuration) /
            double(timecodeScale_);
          
          std::size_t numFrames = binfo->block_.getNumberOfFrames();
          binfo->duration_ = (uint64)(0.5 + frameDuration * double(numFrames));
        }
        else if (binfo->next_ && trackType == Track::kTrackTypeAudio)
        {
          // calculate audio frame duration based on next frame duration:
          binfo->duration_ = binfo->next_->pts_ - binfo->pts_;
        }
      }
      
      return binfo;
    }
    
    return NULL;
  }

  uint64 nextTime(std::size_t trackNo)
  {
    if (trackNo >= track_.size() || !track_[trackNo].size_)
    {
      return (uint64)(~0);
    }
    
    TBlockFifo & track = track_[trackNo];
    uint64 startTime = track.head_->pts_;
    return startTime;
  }

  std::vector<Track::MatroskaTrackType> trackType_;
  std::vector<uint64> defaultDuration_;
  std::vector<TBlockFifo> track_;
  uint64 timecodeScale_;
  uint64 cuesTrackNo_;
  std::size_t size_;
};


//----------------------------------------------------------------
// TDataTable
// 
template <typename TData, unsigned int PageSize = 65536>
struct TDataTable
{
  //----------------------------------------------------------------
  // TPage
  // 
  struct TPage
  {
    enum { kSize = PageSize };
    TData data_[PageSize];
  };

  TDataTable(unsigned int maxSize):
    pages_(0),
    numPages_(0),
    size_(0)
  {
    if (maxSize % TPage::kSize)
    {
      maxSize -= maxSize % TPage::kSize;
      maxSize += TPage::kSize;
    }
    
    numPages_ = maxSize / TPage::kSize;
    
    unsigned int memSize = sizeof(TPage *) * numPages_;
    pages_ = (TPage **)malloc(memSize);
    memset(pages_, 0, memSize);
  }
  
  ~TDataTable()
  {
    for (unsigned int i = 0; i < numPages_; i++)
    {
      TPage * page = pages_[i];
      if (page)
      {
        free(page);
      }
    }
    
    free(pages_);
  }

  bool add(const TData & data)
  {
    unsigned int i = size_ / TPage::kSize;
    if (i >= numPages_)
    {
      assert(i < numPages_);
      return false;
    }
    
    TPage * page = pages_[i];
    if (!page)
    {
      unsigned int memSize = sizeof(TPage);
      page = (TPage *)malloc(memSize);
      memset(page, 0, memSize);
      pages_[i] = page;
    }

    unsigned int j = size_ % TPage::kSize;
    page->data_[j] = data;
    size_++;
    
    return true;
  }
  
  TPage ** pages_;
  unsigned int numPages_;
  unsigned int size_;
};

//----------------------------------------------------------------
// TCue
// 
struct TCue
{
  uint64 time_;
  uint64 track_;
  uint64 cluster_;
  uint64 block_;
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

  void remux(uint64 t0, uint64 t1);
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
  void addCuePoint(TBlockInfo * binfo);
  
  const TTrackMap & trackSrcDst_;
  const TTrackMap & trackDstSrc_;
  const TSegment & srcSeg_;
  TSegment & dstSeg_;
  FileStorage & src_;
  FileStorage & dst_;
  FileStorage & tmp_;
  uint64 clusterBlocks_;
  uint64 cuesTrackKeyframes_;
  std::vector<bool> needCuePointForTrack_;
  
  const uint64 segmentPayloadPosition_;
  uint64 clusterRelativePosition_;
  
  TLace lace_;
  TDataTable<uint64> seekTable_;
  TDataTable<TCue> cueTable_;
  TCluster clusterElt_;
  uint64 t0_;
  uint64 t1_;
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
  clusterBlocks_(0),
  cuesTrackKeyframes_(0),
  segmentPayloadPosition_(dstSeg.payloadReceipt()->position()),
  clusterRelativePosition_(0),
  seekTable_(1 << 24),
  cueTable_(1 << 27),
  t0_(0),
  t1_(0)
{
  TTracks & tracksElt = dstSeg_.payload_.tracks_;
  std::deque<TTrack> & tracks = tracksElt.payload_.tracks_;
  std::size_t numTracks = tracks.size();
  
  // create a barrier to avoid making too many CuePoints:
  needCuePointForTrack_.assign(numTracks + 1, true);
  
  lace_.timecodeScale_ =
    dstSeg_.payload_.info_.payload_.timecodeScale_.payload_.get();
  
  // store track types using base-1 array index for quicker lookup,
  // because track numbers are also base-1:
  lace_.trackType_.resize(numTracks + 1);
  lace_.defaultDuration_.resize(numTracks + 1);
  lace_.track_.resize(numTracks + 1);
  
  lace_.trackType_[0] = Track::kTrackTypeUndefined;
  lace_.defaultDuration_[0] = 0;
  
  for (std::size_t i = 0; i < numTracks; i++)
  {
    Track & track = tracks[i].payload_;
    uint64 trackType = track.trackType_.payload_.get();
    lace_.trackType_[i + 1] = (Track::MatroskaTrackType)trackType;

    uint64 defaultDuration = track.frameDuration_.payload_.get();
    lace_.defaultDuration_[i + 1] = defaultDuration;
  }
  
  // determine which track is the "main" track for starting Clusters:
  for (std::size_t i = 0; i < numTracks; i++)
  {
    Track & track = tracks[i].payload_;
    uint64 trackNo = track.trackNumber_.payload_.get();
    
    Track::MatroskaTrackType trackType = lace_.trackType_[i + 1];
    if (trackType == Track::kTrackTypeVideo)
    {
      lace_.cuesTrackNo_ = trackNo;
      break;
    }
    else if (trackType == Track::kTrackTypeAudio && !lace_.cuesTrackNo_)
    {
      lace_.cuesTrackNo_ = trackNo;
    }
  }

  if (!lace_.cuesTrackNo_ && numTracks)
  {
    lace_.cuesTrackNo_ = tracks[0].payload_.trackNumber_.payload_.get();
  }

  // add 2nd SeekHead:
  dstSeg_.payload_.seekHeads_.push_back(TSeekHead());
  TSeekHead & seekHeadElt = dstSeg_.payload_.seekHeads_.back();
  
  // index the 2nd SeekHead and Cues:
  SeekHead & seekHead1 = dstSeg_.payload_.seekHeads_.front().payload_;
  seekHead1.indexThis(&dstSeg_, &seekHeadElt, tmp_);
  seekHead1.indexThis(&dstSeg_, &(dstSeg_.payload_.cues_), tmp_);
}

//----------------------------------------------------------------
// overwriteUnknownPayloadSize
// 
static bool
overwriteUnknownPayloadSize(IElement & elt, IStorage & storage)
{
  IStorage::IReceiptPtr receipt = elt.storageReceipt();
  if (!receipt)
  {
    return false;
  }
  
  IStorage::IReceiptPtr payload = elt.payloadReceipt();
  IStorage::IReceiptPtr payloadEnd = storage.receipt();
  
  uint64 payloadSize = payloadEnd->position() - payload->position();
  uint64 elementIdSize = uintNumBytes(TSegment::kId);
  
  IStorage::IReceiptPtr payloadSizeReceipt =
    receipt->receipt(elementIdSize, 8);
  
  TByteVec v = vsizeEncode(payloadSize, 8);
  return payloadSizeReceipt->save(Bytes(v));
}

//----------------------------------------------------------------
// TRemuxer::remux
// 
void
TRemuxer::remux(uint64 inPointInSeconds, uint64 outPointInSeconds)
{
  uint64 oneSecond = NANOSEC_PER_SEC / lace_.timecodeScale_;
  t0_ = oneSecond * inPointInSeconds;
  t1_ = oneSecond * outPointInSeconds;
  
  bool extractTimeSegment = (t0_ && t1_);
  
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
    
    if (extractTimeSegment)
    {
      if (clusterTime > t1_ + oneSecond)
      {
        break;
      }
      
      if (clusterTime + oneSecond * 10 < t0_)
      {
        continue;
      }
    }
    
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

  // save the seek point table, rewrite the SeekHead size
  {
    TSeekHead & seekHead2 = dstSeg_.payload_.seekHeads_.back();
    seekHead2.setFixedSize(uintMax[8]);
    seekHead2.save(dst_);
    
    unsigned int pageSize = TDataTable<uint64>::TPage::kSize;
    unsigned int numPages = seekTable_.size_ / pageSize;
    if (seekTable_.size_ % pageSize)
    {
      numPages++;
    }

    TByteVec bvCluster = uintEncode(TCluster::kId);
    TByteVec bvSeekEntry = uintEncode(TSeekEntry::kId);
    TByteVec bvSeekId = uintEncode(SeekEntry::TId::kId);
    TByteVec bvSeekPos = uintEncode(SeekEntry::TPosition::kId);
    
    for (unsigned int i = 0; i < numPages; i++)
    {
      const TDataTable<uint64>::TPage * page = seekTable_.pages_[i];
      
      unsigned int size = pageSize;
      if ((i + 1) == numPages)
      {
        size = seekTable_.size_ % pageSize;
      }

      for (unsigned int j = 0; j < size; j++)
      {
        TByteVec bvPosition = uintEncode(page->data_[j]);

        Bytes seekPointPayload;
        seekPointPayload
          << bvSeekId
          << vsizeEncode(bvCluster.size())
          << bvCluster
          << bvSeekPos
          << vsizeEncode(bvPosition.size())
          << bvPosition;
        
        Bytes seekPoint;
        seekPoint
          << bvSeekEntry
          << vsizeEncode(seekPointPayload.size())
          << seekPointPayload;
        
        dst_.save(seekPoint);
      }
    }
    
    // rewrite SeekHead size:
    overwriteUnknownPayloadSize(seekHead2, dst_);
  }
  
  // save the cue point table, rewrite the Cues size
  {
    TCues & cuesElt = dstSeg_.payload_.cues_;
    cuesElt.setFixedSize(uintMax[8]);
    cuesElt.save(dst_);
    
    unsigned int pageSize = TDataTable<uint64>::TPage::kSize;
    unsigned int numPages = cueTable_.size_ / pageSize;
    if (cueTable_.size_ % pageSize)
    {
      numPages++;
    }

    TByteVec bvCuePoint = uintEncode(TCuePoint::kId);
    TByteVec bvCueTime = uintEncode(CuePoint::TTime::kId);
    TByteVec bvCueTrkPos = uintEncode(CuePoint::TCueTrkPos::kId);
    TByteVec bvCueTrack = uintEncode(CueTrkPos::TTrack::kId);
    TByteVec bvCueCluster = uintEncode(CueTrkPos::TCluster::kId);
    TByteVec bvCueBlock = uintEncode(CueTrkPos::TBlock::kId);
    
    for (unsigned int i = 0; i < numPages; i++)
    {
      const TDataTable<TCue>::TPage * page = cueTable_.pages_[i];
      
      unsigned int size = pageSize;
      if ((i + 1) == numPages)
      {
        size = cueTable_.size_ % pageSize;
      }

      for (unsigned int j = 0; j < size; j++)
      {
        // shortcut to Cue data:
        const TCue & cue = page->data_[j];
        
        TByteVec bvPosition = uintEncode(cue.cluster_);
        TByteVec bvBlock = uintEncode(cue.block_);
        TByteVec bvTrack = uintEncode(cue.track_);
        TByteVec bvTime = uintEncode(cue.time_);
        
        Bytes cueTrkPosPayload;
        cueTrkPosPayload
          << bvCueTrack
          << vsizeEncode(bvTrack.size())
          << bvTrack
          << bvCueCluster
          << vsizeEncode(bvPosition.size())
          << bvPosition
          << bvCueBlock
          << vsizeEncode(bvBlock.size())
          << bvBlock;
        
        Bytes cuePointPayload;
        cuePointPayload
          << bvCueTime
          << vsizeEncode(bvTime.size())
          << bvTime
          << bvCueTrkPos
          << vsizeEncode(cueTrkPosPayload.size())
          << cueTrkPosPayload;
        
        Bytes cuePoint;
        cuePoint
          << bvCuePoint
          << vsizeEncode(cuePointPayload.size())
          << cuePointPayload;
        
        dst_.save(cuePoint);
      }
    }
    
    // rewrite Cues size:
    overwriteUnknownPayloadSize(cuesElt, dst_);
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
      srcBlockTime == dstBlockTime)
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

  info->duration_ = bgroupElt.payload_.duration_.payload_.get();
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

  TBlockInfo * binfo = lace_.pop();
  uint64 clusterTime = clusterElt_.payload_.timecode_.payload_.get();
  int64 blockTime = binfo->pts_ - clusterTime;
  
  bool extractTimeSegment = (t0_ && t1_);
  if (extractTimeSegment)
  {
    if ((binfo->pts_ > t1_) ||
        (binfo->pts_ + binfo->duration_ < t0_))
    {
      delete binfo;
      return;
    }
  }
  
  if ((binfo->trackNo_ == 0) ||
      (blockTime > kMaxShort) ||
      (binfo->trackNo_ == lace_.cuesTrackNo_ &&
       binfo->block_.isKeyframe() &&
       cuesTrackKeyframes_ == 1) ||
      (clusterElt_.storageReceipt() == NULL))
  {
    startNextCluster(binfo);

    clusterTime = clusterElt_.payload_.timecode_.payload_.get();
    blockTime = binfo->pts_ - clusterTime;
  }

  if (binfo->trackNo_ && updateHeader(clusterTime, *binfo))
  {
    Cluster & cluster = clusterElt_.payload_;
    
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
    addCuePoint(binfo);
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
  
  // start a new cluster:
  clusterBlocks_ = 0;
  cuesTrackKeyframes_ = 0;
  clusterElt_ = TCluster();
  
  // set the start time:
  clusterElt_.payload_.timecode_.payload_.set(binfo->pts_);

  if (!binfo->trackNo_ && binfo->silentElt_.mustSave())
  {
    // copy the SilentTracks, update track numbers to match:
    typedef SilentTracks::TTrack TTrkNumElt;
    typedef std::list<TTrkNumElt> TTrkNumElts;

    clusterElt_.payload_.silent_.alwaysSave();
    
    const TTrkNumElts & srcSilentTracks =
      binfo->silentElt_.payload_.tracks_;

    TTrkNumElts & dstSilentTracks =
      clusterElt_.payload_.silent_.payload_.tracks_;

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
  
  clusterElt_.savePaddedUpToSize(dst_, uintMax[8]);
  
  // save relative cluster position for 2nd SeekHead:
  uint64 absPos = clusterElt_.storageReceipt()->position();
  clusterRelativePosition_ = absPos - segmentPayloadPosition_;
  seekTable_.add(clusterRelativePosition_);
}

//----------------------------------------------------------------
// TRemuxed::finishCurrentCluster
// 
void
TRemuxer::finishCurrentCluster()
{
  // fix the cluster size:
  overwriteUnknownPayloadSize(clusterElt_, dst_);
}

//----------------------------------------------------------------
// TRemuxer::addCuePoint
// 
void
TRemuxer::addCuePoint(TBlockInfo * binfo)
{
  if (!binfo->block_.isKeyframe())
  {
    return;
  }

  if (binfo->trackNo_ == lace_.cuesTrackNo_)
  {
    cuesTrackKeyframes_++;

    // reset the CuePoint barrier:
    needCuePointForTrack_.assign(needCuePointForTrack_.size(), true);
  }

  if (!needCuePointForTrack_[binfo->trackNo_])
  {
    // avoid making too many CuePoints:
    return;
  }
  
  needCuePointForTrack_[binfo->trackNo_] = false;
  
  TCue cue;
  cue.time_ = binfo->pts_;
  cue.track_ = binfo->trackNo_;
  cue.cluster_ = clusterRelativePosition_;
  cue.block_ = clusterBlocks_;
  cueTable_.add(cue);
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
  uint64 t1 = 0;
  
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
  LoaderSkipClusterPayload skipClusters(srcSize);

  // attempt to load via SeekHead(s):
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
    seekHeadElt.setFixedSize(256);
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
    }

    // index the first set of top-level elements:
    seekHead.indexThis(&segmentElt, &segInfoElt, tmp);
    seekHead.indexThis(&segmentElt, &tracksElt, tmp);

    TAttachment & attachmentsElt = segmentElt.payload_.attachments_;
    attachmentsElt = segmentIn.attachments_;
    if (attachmentsElt.mustSave())
    {
      seekHead.indexThis(&segmentElt, &attachmentsElt, tmp);
    }
    
    TChapters & chaptersElt = segmentElt.payload_.chapters_;
    chaptersElt = segmentIn.chapters_;
    if (chaptersElt.mustSave())
    {
      seekHead.indexThis(&segmentElt, &chaptersElt, tmp);
    }

    // minor cleanup prior to saving:
    RemoveVoids().eval(segmentElt);
    
    // discard previous storage receipts:
    DiscardReceipts().eval(segmentElt);
    
    // save a placeholder:
    IStorage::IReceiptPtr segmentReceipt = segmentElt.save(dst);
    
    printCurrentTime("begin segment remux");

    // on-the-fly remux Clusters/BlockGroups/SimpleBlocks:
    TRemuxer remuxer(trackInOut, trackOutIn, *i, segmentElt, src, dst, tmp);
    remuxer.remux(t0, t1);

    printCurrentTime("finished segment remux");
    
    // rewrite the SeekHead:
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
    overwriteUnknownPayloadSize(segmentElt, dst);
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
