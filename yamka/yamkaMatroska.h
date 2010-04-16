// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 15:56:33 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_MATROSKA_H_
#define YAMKA_MATROSKA_H_

// yamka includes:
#include <yamkaElt.h>
#include <yamkaPayload.h>
#include <yamkaEBML.h>

// system includes:
#include <deque>


namespace Yamka
{

  //----------------------------------------------------------------
  // ChapTranslate
  // 
  struct ChapTranslate : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0x69fc, "EditionUID") editionUID_;
    Elt(VUInt, 0x69bf, "ChapTransCodec") chapTransCodec_;
    Elt(VBinary, 0x69a5, "ChapTransID") chapTransID_;
  };
  
  //----------------------------------------------------------------
  // SegInfo
  // 
  struct SegInfo : public EbmlMaster
  {
    SegInfo();
    
    ImplementsPayloadAPI();
    
    Elt(VBytes<16>, 0x73a4, "SegmentUID") segUID_;
    Elt(VString, 0x7384, "SegmentFilename") segFilename_;
    Elt(VBytes<16>, 0x3cb923, "PrevUID") prevUID_;
    Elt(VString, 0x3c83ab, "PrevFilename") prevFilename_;
    Elt(VBytes<16>, 0x3eb923, "NextUID") nextUID_;
    Elt(VString, 0x3e83bb, "NextFilename") nextFilename_;
    Elt(VBytes<16>, 0x4444, "FamilyUID") familyUID_;
    Elt(ChapTranslate, 0x6924, "ChapTranslate") chapTranslate_;
    Elt(VUInt, 0x2ad7b1, "TimecodeScale") timecodeScale_;
    Elt(VFloat, 0x4489, "Duration") duration_;
    Elt(VDate, 0x4461, "DateUTC") date_;
    Elt(VString, 0x7ba9, "Title") title_;
    Elt(VString, 0x4d80, "MuxingApp") muxingApp_;
    Elt(VString, 0x5741, "WritingApp") writingApp_;
  };
  
  //----------------------------------------------------------------
  // TrackTranslate
  // 
  struct TrackTranslate : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0x66fc, "EditionUID") editionUID_;
    Elt(VUInt, 0x66bf, "TrackTransCodec") trackTransCodec_;
    Elt(VBinary, 0x66a5, "TrackTransID") trackTransID_;
  };
  
  //----------------------------------------------------------------
  // Video
  // 
  struct Video : public EbmlMaster
  {
    Video();
    
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0x9A, "FlagInterlaced") flagInterlaced_;
    Elt(VUInt, 0x53B8, "StereoMode") stereoMode_;
    Elt(VUInt, 0xB0, "PixelWidth") pixelWidth_;
    Elt(VUInt, 0xBA, "PixelHeight") pixelHeight_;
    Elt(VUInt, 0x54AA, "PixelCropBottom") pixelCropBottom_;
    Elt(VUInt, 0x54BB, "PixelCropTop") pixelCropTop_;
    Elt(VUInt, 0x54CC, "PixelCropLeft") pixelCropLeft_;
    Elt(VUInt, 0x54DD, "PixelCropRight") pixelCropRight_;
    Elt(VUInt, 0x54B0, "DisplayWidth") displayWidth_;
    Elt(VUInt, 0x54BA, "DisplayHeight") displayHeight_;
    Elt(VUInt, 0x54B2, "DisplayUnits") displayUnits_;
    Elt(VUInt, 0x54B3, "AspectRatioType") aspectRatioType_;
    Elt(VBytes<4>, 0x2EB524, "ColorSpace") colorSpace_;
    Elt(VFloat, 0x2FB523, "GammaValue") gammaValue_;
    Elt(VFloat, 0x2383E3, "FrameRate") frameRate_;
  };
  
  //----------------------------------------------------------------
  // Audio
  // 
  struct Audio : public EbmlMaster
  {
    Audio();
    
    ImplementsPayloadAPI();
    
    Elt(VFloat, 0xB5, "SamplingFrequency") sampFreq_;
    Elt(VFloat, 0x78B5, "OutputSamplingFrequency") sampFreqOut_;
    Elt(VUInt, 0x9F, "Channels") channels_;
    Elt(VBinary, 0x7D7B, "ChannelPositions") channelPositions_;
    Elt(VUInt, 0x6264, "BitDepth") bitDepth_;
  };
  
  //----------------------------------------------------------------
  // ContentCompr
  // 
  struct ContentCompr : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0x4254, "ContentCompAlgo") algo_;
    Elt(VBinary, 0x4255, "ContentCompSettings") settings_;
  };
  
  //----------------------------------------------------------------
  // ContentEncrypt
  // 
  struct ContentEncrypt : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0x47E1, "ContentEncAlgo") encAlgo_;
    Elt(VBinary, 0x47E2, "ContentEncKeyID") encKeyID_;
    Elt(VBinary, 0x47E3, "ContentSignature") signature_;
    Elt(VBinary, 0x47E4, "ContentSigKeyID") sigKeyID_;
    Elt(VUInt, 0x47E5, "ContentSigAlgo") sigAlgo_;
    Elt(VUInt, 0x47e6, "ContentSigHashAlgo") sigHashAlgo_;
  };
  
  //----------------------------------------------------------------
  // ContentEnc
  // 
  struct ContentEnc : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0x5031, "ContentEncodingOrder") order_;
    Elt(VUInt, 0x5032, "ContentEncodingScope") scope_;
    Elt(VUInt, 0x5033, "ContentEncodingType") type_;
    Elt(ContentCompr, 0x5034, "ContentCompression") compression_;
    Elt(ContentEncrypt, 0x5035, "ContentEncryption") encryption_;
  };
  
  //----------------------------------------------------------------
  // ContentEncodings
  // 
  struct ContentEncodings : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elts(ContentEnc, 0x6240, "ContentEnc") encodings_;
  };
  
  //----------------------------------------------------------------
  // Track
  // 
  struct Track : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0xD7, "TrackNumber") trackNumber_;
    Elt(VUInt, 0x73C5, "TrackUID") trackUID_;
    Elt(VUInt, 0x83, "TrackType") trackType_;
    Elt(VUInt, 0xB9, "FlagEnabled") flagEnabled_;
    Elt(VUInt, 0x88, "FlagDefault") flagDefault_;
    Elt(VUInt, 0x55AA, "FlagForces") flagForced_;
    Elt(VUInt, 0x9C, "FlagLacing") flagLacing_;
    Elt(VUInt, 0x6DE7, "MinCache") minCache_;
    Elt(VUInt, 0x6DF8, "MaxCache") maxCache_;
    Elt(VUInt, 0x23E383, "DefaultDuration") frameDuration_;
    Elt(VFloat, 0x23314F, "TrackTimecodeScale") timecodeScale_;
    Elt(VInt, 0x537F, "TrackOffset") trackOffset_;
    Elt(VUInt, 0x55EE, "MaxBlockAddID") maxBlockAddID_;
    Elt(VString, 0x536E, "Name") name_;
    Elt(VString, 0x22B59C, "Language") language_;
    Elt(VString, 0x86, "CodecID") codecID_;
    Elt(VBinary, 0x63A2, "CodecPrivate") codecPrivate_;
    Elt(VString, 0x258688, "CodecName") codecName_;
    Elt(VUInt, 0x7446, "AttachmentLink") attachmentLink_;
    Elt(VString, 0x3A9697, "CodecSettings") codecSettings_;
    Elt(VString, 0x3B4040, "CodecInfoURL") codecInfoURL_;
    Elt(VString, 0x26B240, "CodecDownloadURL") codecDownloadURL_;
    Elt(VUInt, 0xAA, "CodecDecodeAll") codecDecodeAll_;
    Elt(VUInt, 0x6FAB, "TrackOverlay") trackOverlay_;
    Elt(TrackTranslate, 0x6624, "TrackTranslate") trackTranslate_;
    Elt(Video, 0xE0, "Video") video_;
    Elt(Audio, 0xE1, "Audio") audio_;
    Elt(ContentEncodings, 0x6D80, "ContentEncodings") contentEncs_;
  };
  
  //----------------------------------------------------------------
  // Tracks
  // 
  struct Tracks : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elts(Track, 0xae, "TrackEntry") tracks_;
  };
  
  //----------------------------------------------------------------
  // CueRef
  // 
  struct CueRef : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0x96, "CueRefTime") time_;
    Elt(VEltPosition, 0x97, "CueRefCluster") cluster_;
    Elt(VUInt, 0x535F, "CueRefNumber") block_;
    Elt(VEltPosition, 0xEB, "CueRefCodecState") codecState_;
  };
  
  //----------------------------------------------------------------
  // CueTrkPos
  // 
  struct CueTrkPos : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0xF7, "Track") track_;
    Elt(VEltPosition, 0xF1, "ClusterPosition") cluster_;
    Elt(VUInt, 0x5378, "CueBlockNumber") block_;
    Elt(VEltPosition, 0xEA, "CueCodecState") codecState_;
    Elt(CueRef, 0xDB, "CueReference") ref_;
  };
  
  //----------------------------------------------------------------
  // CuePoint
  // 
  struct CuePoint : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0xB3, "CueTime") time_;
    Elts(CueTrkPos, 0xB7, "CueTrackPosition") trkPosns_;
  };
  
  //----------------------------------------------------------------
  // Cues
  // 
  struct Cues : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elts(CuePoint, 0xBB, "CuePoint") points_;
  };
  
  //----------------------------------------------------------------
  // SeekEntry
  // 
  struct SeekEntry : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VBinary, 0x53AB, "SeekID") id_;
    Elt(VEltPosition, 0x53AC, "SeekPosition") position_;
  };
  
  //----------------------------------------------------------------
  // SeekHead
  // 
  struct SeekHead : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elts(SeekEntry, 0x4DBB, "Seek") seek_;
  };
  
  //----------------------------------------------------------------
  // AttdFile
  // 
  struct AttdFile : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VString, 0x467E, "FileDescription") description_;
    Elt(VString, 0x466E, "FileName") filename_;
    Elt(VString, 0x4660, "FileMimeType") mimeType_;
    Elt(VBinary, 0x465C, "FileData") data_;
    Elt(VUInt, 0x46AE, "FileUID") fileUID_;
    Elt(VBinary, 0x4675, "FileReferral") referral_;
  };
  
  //----------------------------------------------------------------
  // Attachments
  // 
  struct Attachments : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elts(AttdFile, 0x61A7, "AttachedFile") files_;
  };
  
  //----------------------------------------------------------------
  // ChapTrk
  // 
  struct ChapTrk : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elts(VUInt, 0x89, "ChapterTrackNumber") tracks_;
  };
  
  //----------------------------------------------------------------
  // ChapDisp
  // 
  struct ChapDisp : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VString, 0x85, "ChapString") string_;
    Elt(VString, 0x437C, "ChapLanguage") language_;
    Elt(VString, 0x437E, "ChapCountry") country_;
  };
  
  //----------------------------------------------------------------
  // ChapProcCmd
  // 
  struct ChapProcCmd : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0x6922, "ChapProcessTime") time_;
    Elt(VBinary, 0x6933, "ChapProcessData") data_;
  };
  
  //----------------------------------------------------------------
  // ChapProc
  // 
  struct ChapProc : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0x6955, "ChapProcessCodecID") codecID_;
    Elt(VBinary, 0x450D, "ChapProcessPrivate") procPrivate_;
    Elts(ChapProcCmd, 0x6911, "ChapProcCommands") cmds_;
  };
  
  //----------------------------------------------------------------
  // ChapAtom
  // 
  struct ChapAtom : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0x73C4, "ChapterUID") UID_;
    Elt(VUInt, 0x91, "ChapterTimeStart") timeStart_;
    Elt(VUInt, 0x92, "ChapterTimeEnd") timeEnd_;
    Elt(VUInt, 0x98, "ChapterFlagHidden") hidden_;
    Elt(VUInt, 0x4598, "ChapterFlagEnabled") enabled_;
    Elt(VBinary, 0x6E67, "ChapterSegmentUID") segUID_;
    Elt(VBinary, 0x6EBC, "ChapterSegmentEditionUID") segEditionUID_;
    Elt(VUInt, 0x63C3, "ChapterPhysicalEquiv") physEquiv_;
    Elt(ChapTrk, 0x8F, "ChapterTracks") tracks_;
    Elts(ChapDisp, 0x80, "ChapterDisplay") display_;
    Elts(ChapProc, 0x6944, "ChapProcess") process_;
  };
  
  //----------------------------------------------------------------
  // Edition
  // 
  struct Edition : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0x45BC, "EditionUID") UID_;
    Elt(VUInt, 0x45BD, "EditionFlagHidden") flagHidden_;
    Elt(VUInt, 0x45DB, "EditionFlagDefault") flagDefault_;
    Elt(VUInt, 0x45DD, "EditionFlagOrdered") flagOrdered_;
    Elts(ChapAtom, 0xB6, "ChapterAtom") chapAtoms_;
  };
  
  //----------------------------------------------------------------
  // Chapters
  // 
  struct Chapters : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elts(Edition, 0x45B9, "EditionEntry") editions_;
  };
  
  //----------------------------------------------------------------
  // TagTargets
  // 
  struct TagTargets : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0x68CA, "TargetTypeValue") typeValue_;
    Elt(VString, 0x63CA, "TargetType") type_;
    Elts(VUInt, 0x63C5, "TrackUID") trackUIDs_;
    Elts(VUInt, 0x63C9, "EditionUID") editionUIDs_;
    Elts(VUInt, 0x63C4, "ChapterUID") chapterUIDs_;
    Elts(VUInt, 0x63C6, "AttachmentUID") attachmentUIDs_;
  };
  
  //----------------------------------------------------------------
  // SimpleTag
  // 
  struct SimpleTag : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VString, 0x45A3, "TagName") name_;
    Elt(VString, 0x447A, "TagLanguage") lang_;
    Elt(VUInt, 0x4484, "TagDefault") default_;
    Elt(VString, 0x4487, "TagString") string_;
    Elt(VBinary, 0x4485, "TagBinary") binary_;
  };
  
  //----------------------------------------------------------------
  // Tag
  // 
  struct Tag : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(TagTargets, 0x63C0, "Targets") targets_;
    Elts(SimpleTag, 0x67C8, "SimpleTags") simpleTags_;
  };
  
  //----------------------------------------------------------------
  // Tags
  // 
  struct Tags : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elts(Tag, 0x7373, "Tag") tags_;
  };
  
  //----------------------------------------------------------------
  // SilentTracks
  // 
  struct SilentTracks : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elts(VUInt, 0x58D7, "SilentTrackNumber") tracks_;
  };
  
  //----------------------------------------------------------------
  // Block
  // 
  struct Block : public VBinary
  {
  };
  
  //----------------------------------------------------------------
  // BlockVirtual
  // 
  struct BlockVirtual : public VBinary
  {
  };
  
  //----------------------------------------------------------------
  // BlockMore
  // 
  struct BlockMore : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0xEE, "BlockAddID") blockAddID_;
    Elt(VBinary, 0xA5, "BlockAdditional") blockAdditional_;
  };
  
  //----------------------------------------------------------------
  // BlockAdditions
  // 
  struct BlockAdditions : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elts(BlockMore, 0xA6, "BlockMore") more_;
  };
  
  //----------------------------------------------------------------
  // BlockGroup
  // 
  struct BlockGroup : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0x9B, "BlockDuration") duration_;
    Elt(Block, 0xA1, "Block") block_;
    Elts(BlockVirtual, 0xA2, "BlockVirtual") blockVirtual_;
    Elt(BlockAdditions, 0x75A1, "BlockAdditions") additions_;
    Elt(VUInt, 0xFA, "ReferencePriority") refPriority_;
    Elts(VInt, 0xFB, "ReferenceBlock") refBlock_;
    Elt(VInt, 0xFD, "ReferenceVirtual") refVirtual_;
    Elt(VBinary, 0xA4, "CodecState") codecState_;
    Elts(VBinary, 0x8E, "Slice") slices_;
  };
  
  //----------------------------------------------------------------
  // SimpleBlock
  // 
  struct SimpleBlock : public VBinary
  {
  };
  
  //----------------------------------------------------------------
  // Cluster
  // 
  struct Cluster : public EbmlMaster
  {
    Cluster();
    
    ImplementsPayloadAPI();
    
    Elt(VUInt, 0xE7, "Timecode") timecode_;
    Elt(SilentTracks, 0x5854, "SilentTracks") silent_;
    Elt(VEltPosition, 0xA7, "Position") position_;
    Elt(VUInt, 0xAB, "PrevSize") prevSize_;
    Elts(BlockGroup, 0xA0, "BlockGroup") blockGroups_;
    Elts(SimpleBlock, 0xA3, "SimpleBlock") simpleBlocks_;
    Elts(VBinary, 0xAF, "EncryptedBlock") encryptedBlocks_;
  };
  
  //----------------------------------------------------------------
  // Segment
  // 
  struct Segment : public EbmlMaster
  {
    ImplementsPayloadAPI();
    
    // resolve positional references (seeks, cues, etc...)
    void resolveReferences(const IElement * origin);
    
    Elt(SegInfo, 0x1549A966, "SegInfo") info_;
    Elt(Tracks, 0x1654AE6b, "Tracks") tracks_;
    Elts(SeekHead, 0x114D9B74, "SeekHead") seekHeads_;
    Elts(Cues, 0x1C53BB6B, "Cues") cues_;
    Elts(Attachments, 0x1941A469, "Attachments") attachments_;
    Elt(Chapters, 0x1043a770, "Chapters") chapters_;
    Elts(Tags, 0x1254C367, "Tags") tags_;
    Elts(Cluster, 0x1f43b675, "Cluster") clusters_;
  };
  
  //----------------------------------------------------------------
  // MatroskaDoc
  // 
  struct MatroskaDoc : public EbmlDoc
  {
    MatroskaDoc();
    
    ImplementsPayloadAPI();
    
    // resolve positional references (seeks, cues, etc...)
    // for each segment:
    void resolveReferences();
    
    Elts(Segment, 0x18538067, "Segment") segments_;
  };
  
}


#endif // YAMKA_MATROSKA_H_
