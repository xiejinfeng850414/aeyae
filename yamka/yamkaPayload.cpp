// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 23:49:04 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaPayload.h>

// system includes:
#include <assert.h>
#include <string.h>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // VInt::VInt
  // 
  VInt::VInt():
    TSuper()
  {
    setDefault(0);
  }
  
  //----------------------------------------------------------------
  // VInt::eval
  // 
  bool
  VInt::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VInt::isDefault
  // 
  bool
  VInt::isDefault() const
  {
    bool allDefault =
      TSuper::data_ == TSuper::dataDefault_;
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // VInt::calcSize
  // 
  uint64
  VInt::calcSize() const
  {
    return std::max<uint64>(TSuper::size_, intNumBytes(TSuper::data_));
  }

  //----------------------------------------------------------------
  // VInt::save
  // 
  IStorage::IReceiptPtr
  VInt::save(IStorage & storage) const
  {
    Bytes bytes;
    bytes << intEncode(TSuper::data_);
    
    return storage.save(bytes);
  }
  
  //----------------------------------------------------------------
  // VInt::load
  // 
  uint64
  VInt::load(FileStorage & storage, uint64 bytesToRead)
  {
    Bytes bytes((std::size_t)bytesToRead);
    if (!storage.load(bytes))
    {
      return 0;
    }
    
    TSuper::data_ = intDecode(bytes, bytesToRead);
    return bytesToRead;
  }


  //----------------------------------------------------------------
  // VUInt::VUInt
  // 
  VUInt::VUInt():
    TSuper()
  {
    setDefault(0);
  }
  
  //----------------------------------------------------------------
  // VUInt::eval
  // 
  bool
  VUInt::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VUInt::isDefault
  // 
  bool
  VUInt::isDefault() const
  {
    bool allDefault =
      TSuper::data_ == TSuper::dataDefault_;
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // VUInt::calcSize
  // 
  uint64
  VUInt::calcSize() const
  {
    return std::max<uint64>(TSuper::size_, uintNumBytes(TSuper::data_));
  }

  //----------------------------------------------------------------
  // VUInt::save
  // 
  IStorage::IReceiptPtr
  VUInt::save(IStorage & storage) const
  {
    Bytes bytes;
    bytes << uintEncode(TSuper::data_);
    
    return storage.save(bytes);
  }
  
  //----------------------------------------------------------------
  // VUInt::load
  // 
  uint64
  VUInt::load(FileStorage & storage, uint64 bytesToRead)
  {
    Bytes bytes((std::size_t)bytesToRead);
    if (!storage.load(bytes))
    {
      return 0;
    }
    
    TSuper::data_ = uintDecode(bytes, bytesToRead);
    return bytesToRead;
  }
  
  
  //----------------------------------------------------------------
  // VFloat::VFloat
  // 
  VFloat::VFloat():
    TSuper(4)
  {
    setDefault(0.0);
  }
  
  //----------------------------------------------------------------
  // VFloat::eval
  // 
  bool
  VFloat::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VFloat::isDefault
  // 
  bool
  VFloat::isDefault() const
  {
    bool allDefault =
      TSuper::data_ == TSuper::dataDefault_;
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // VFloat::calcSize
  // 
  uint64
  VFloat::calcSize() const
  {
    // only 32-bit floats and 64-bit doubles are allowed:
    return (TSuper::size_ > 4) ? 8 : 4;
  }
  
  //----------------------------------------------------------------
  // VFloat::save
  // 
  IStorage::IReceiptPtr
  VFloat::save(IStorage & storage) const
  {
    uint64 size = calcSize();
    Bytes bytes;
    
    if (size == 4)
    {
      bytes << floatEncode(float(TSuper::data_));
    }
    else
    {
      bytes << doubleEncode(TSuper::data_);
    }
    
    return storage.save(bytes);
  }
  
  //----------------------------------------------------------------
  // VFloat::load
  // 
  uint64
  VFloat::load(FileStorage & storage, uint64 bytesToRead)
  {
    Bytes bytes((std::size_t)bytesToRead);
    if (!storage.load(bytes))
    {
      return 0;
    }
    
    if (bytesToRead > 4)
    {
      TSuper::data_ = doubleDecode(bytes);
      TSuper::size_ = 8;
    }
    else
    {
      TSuper::data_ = double(floatDecode(bytes));
      TSuper::size_ = 4;
    }
    
    return bytesToRead;
  }
  
  
  //----------------------------------------------------------------
  // kVDateMilleniumUTC
  // 
  // 2001/01/01 00:00:00 UTC
  static const std::time_t kVDateMilleniumUTC = 978307200;
  
  //----------------------------------------------------------------
  // VDate::VDate
  // 
  VDate::VDate():
    TSuper(8)
  {
    setDefault(0);
    
    std::time_t currentTime = std::time(NULL);
    setTime(currentTime);
  }
  
  //----------------------------------------------------------------
  // VDate::eval
  // 
  bool
  VDate::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VDate::isDefault
  // 
  bool
  VDate::isDefault() const
  {
    bool allDefault =
      TSuper::data_ == TSuper::dataDefault_;
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // VDate::setTime
  // 
  void
  VDate::setTime(std::time_t t)
  {
    TSuper::data_ = int64(t - kVDateMilleniumUTC) * 1000000000;
  }
  
  //----------------------------------------------------------------
  // getTime
  // 
  std::time_t
  VDate::getTime() const
  {
    std::time_t t = kVDateMilleniumUTC + TSuper::data_ / 1000000000;
    return t;
  }
  
  //----------------------------------------------------------------
  // TSuper::calcSize
  // 
  uint64
  VDate::calcSize() const
  {
    return 8;
  }
  
  //----------------------------------------------------------------
  // VDate::save
  // 
  IStorage::IReceiptPtr
  VDate::save(IStorage & storage) const
  {
    uint64 size = calcSize();
    
    Bytes bytes;
    bytes << intEncode(TSuper::data_, size);
    
    return storage.save(bytes);
  }
  
  //----------------------------------------------------------------
  // VDate::load
  // 
  uint64
  VDate::load(FileStorage & storage, uint64 bytesToRead)
  {
    Bytes bytes((std::size_t)bytesToRead);
    if (!storage.load(bytes))
    {
      return 0;
    }
    
    TSuper::data_ = intDecode(bytes, bytesToRead);
    return bytesToRead;
  }
  
  
  //----------------------------------------------------------------
  // VString::eval
  // 
  bool
  VString::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VString::isDefault
  // 
  bool
  VString::isDefault() const
  {
    bool allDefault =
      TSuper::data_ == TSuper::dataDefault_;
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // VString::calcSize
  // 
  uint64
  VString::calcSize() const
  {
    return std::max<uint64>(TSuper::size_, TSuper::data_.size());
  }

  //----------------------------------------------------------------
  // VString::save
  // 
  IStorage::IReceiptPtr
  VString::save(IStorage & storage) const
  {
    Bytes bytes;
    bytes << TSuper::data_;
    
    return storage.save(bytes);
  }
  
  //----------------------------------------------------------------
  // VString::load
  // 
  uint64
  VString::load(FileStorage & storage, uint64 bytesToRead)
  {
    Bytes bytes((std::size_t)bytesToRead);
    if (!storage.load(bytes))
    {
      return 0;
    }
    
    TByteVec chars = TByteVec(bytes);
    TSuper::data_.assign((const char *)&chars[0], chars.size());
    return bytesToRead;
  }
  
  //----------------------------------------------------------------
  // VString::set
  // 
  VString &
  VString::set(const std::string & str)
  {
    TSuper::set(str);
    return *this;
  }
  
  //----------------------------------------------------------------
  // VString::set
  // 
  VString &
  VString::set(const char * cstr)
  {
    std::string str;
    if (cstr)
    {
      str = std::string(cstr);
    }
    
    TSuper::set(str);
    return *this;
  }
  
  
  //----------------------------------------------------------------
  // VBinary::VBinary
  // 
  VBinary::VBinary()
  {}
  
  //----------------------------------------------------------------
  // VBinary::set
  // 
  VBinary &
  VBinary::set(const Bytes & bytes, IStorage & storage)
  {
    receipt_ = storage.save(bytes);
    return *this;
  }

  //----------------------------------------------------------------
  // VBinary::get
  // 
  bool
  VBinary::get(Bytes & bytes) const
  {
    if (!receipt_)
    {
      return false;
    }
    
    bytes = Bytes((std::size_t)receipt_->numBytes());
    return receipt_->load(bytes);
  }
  
  //----------------------------------------------------------------
  // VBinary::setDefault
  // 
  VBinary &
  VBinary::setDefault(const Bytes & bytes, IStorage & storage)
  {
    receiptDefault_ = storage.save(bytes);
    receipt_ = receiptDefault_;
    return *this;
  }
  
  //----------------------------------------------------------------
  // VBinary::eval
  // 
  bool
  VBinary::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VBinary::isDefault
  // 
  bool
  VBinary::isDefault() const
  {
    if (receiptDefault_ && receipt_)
    {
      std::size_t sizeDefault = (std::size_t)receiptDefault_->numBytes();
      std::size_t size = (std::size_t)receipt_->numBytes();
      
      if (sizeDefault != size)
      {
        return false;
      }
      
      if (size == 0)
      {
        return true;
      }
      
      if (receiptDefault_ == receipt_)
      {
        return true;
      }
      
      Bytes bytesDefault(sizeDefault);
      if (receiptDefault_->load(bytesDefault))
      {
        // default payload can't be read:
        return false;
      }
      
      Bytes bytes(size);
      if (!receipt_->load(bytes))
      {
        // payload can't be read:
        return true;
      }
      
      // compare byte vectors:
      bool same = (bytesDefault.bytes_->front() ==
                   bytes.bytes_->front());
      return same;
    }
    
    return !receipt_;
  }
  
  //----------------------------------------------------------------
  // VBinary::calcSize
  // 
  uint64
  VBinary::calcSize() const
  {
    if (!receipt_)
    {
      return 0;
    }
    
    return receipt_->numBytes();
  }
  
  //----------------------------------------------------------------
  // VBinary::save
  // 
  IStorage::IReceiptPtr
  VBinary::save(IStorage & storage) const
  {
    if (!receipt_)
    {
      assert(false);
      return IStorage::IReceiptPtr();
    }
    
    Bytes data((std::size_t)receipt_->numBytes());
    if (!receipt_->load(data))
    {
      assert(false);
      return IStorage::IReceiptPtr();
    }
    
    return storage.save(data);
  }
  
  //----------------------------------------------------------------
  // VBinary::load
  // 
  uint64
  VBinary::load(FileStorage & storage, uint64 bytesToRead)
  {
    Bytes bytes((std::size_t)bytesToRead);
    
    receipt_ = storage.load(bytes);
    if (!receipt_)
    {
      return 0;
    }
    
    return receipt_->numBytes();
  }
  
  
  //----------------------------------------------------------------
  // VEltPosition::VEltPosition
  // 
  VEltPosition::VEltPosition():
    origin_(NULL),
    elt_(NULL),
    pos_(uintMax[8]),
    unknownPositionSize_(8)
  {}
  
  //----------------------------------------------------------------
  // VEltPosition::setUnknownPositionSize
  // 
  void
  VEltPosition::setUnknownPositionSize(uint64 vsize)
  {
    if (vsize > 8)
    {
      assert(false);
      vsize = 8;
    }
    
    if (pos_ == uintMax[unknownPositionSize_])
    {
      // adjust the unknown position:
      pos_ = uintMax[vsize];
    }
    
    unknownPositionSize_ = vsize;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::eval
  // 
  bool
  VEltPosition::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::isDefault
  // 
  bool
  VEltPosition::isDefault() const
  {
    // no point is saving an unresolved invalid reference:
    return !elt_;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::calcSize
  // 
  uint64
  VEltPosition::calcSize() const
  {
    if (!elt_)
    {
      return unknownPositionSize_;
    }
    
    IStorage::IReceiptPtr eltReceipt = elt_->storageReceipt();
    if (!eltReceipt)
    {
      return unknownPositionSize_;
    }
    
    if (receipt_)
    {
      // must use the same number of bytes as before:
      return receipt_->numBytes();
    }
    
    // NOTE:
    // 1. The origin position may not be known when this is called,
    // 2. We can assume that the relative position will not require
    //    any more bytes than the absolute position would.
    // 3. We'll use the absolute position to calculate the required
    //    number of bytes
    // 
    uint64 absolutePosition = eltReceipt->position();
    uint64 bytesNeeded = uintNumBytes(absolutePosition);
    return bytesNeeded;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::save
  // 
  IStorage::IReceiptPtr
  VEltPosition::save(IStorage & storage) const
  {
    uint64 bytesNeeded = calcSize();
    uint64 eltPosition = uintMax[bytesNeeded];
    uint64 originPosition = getOriginPosition();
    
    if (elt_)
    {
      IStorage::IReceiptPtr eltReceipt = elt_->storageReceipt();
      if (eltReceipt)
      {
        eltPosition = eltReceipt->position();
      }
    }
    
    // let VUInt do the rest:
    uint64 relativePosition = eltPosition - originPosition;
    VUInt data;
    data.setSize(bytesNeeded);
    data.set(relativePosition);
    
    receipt_ = data.save(storage);
    return receipt_;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::load
  // 
  uint64
  VEltPosition::load(FileStorage & storage, uint64 bytesToRead)
  {
    VUInt data;
    uint64 bytesRead = data.load(storage, bytesToRead);
    if (bytesRead)
    {
      origin_ = NULL;
      elt_ = NULL;
      pos_ = data.get();
    }
    
    return bytesRead;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::setOrigin
  // 
  void
  VEltPosition::setOrigin(const IElement * origin)
  {
    origin_ = origin;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::getOrigin
  // 
  const IElement *
  VEltPosition::getOrigin() const
  {
    return origin_;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::setElt
  // 
  void
  VEltPosition::setElt(const IElement * elt)
  {
    elt_ = elt;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::getElt
  // 
  const IElement *
  VEltPosition::getElt() const
  {
    return elt_;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::position
  // 
  uint64
  VEltPosition::position() const
  {
    return pos_;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::rewrite
  // 
  bool
  VEltPosition::rewrite() const
  {
    if (!receipt_)
    {
      return false;
    }
    
    if (!elt_)
    {
      return false;
    }
    
    IStorage::IReceiptPtr eltReceipt = elt_->storageReceipt();
    if (!eltReceipt)
    {
      return false;
    }
    
    uint64 originPosition = getOriginPosition();
    uint64 eltPosition = eltReceipt->position();
    uint64 relativePosition = eltPosition - originPosition;
    
    uint64 bytesNeeded = uintNumBytes(relativePosition);
    uint64 bytesUsed = receipt_->numBytes();
    
    if (bytesNeeded > bytesUsed)
    {
      // must use the same size as before:
      return false;
    }
    
    Bytes bytes;
    bytes << uintEncode(relativePosition, bytesUsed);
    
    bool saved = receipt_->save(bytes);
    return saved;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::getOriginPosition
  // 
  uint64
  VEltPosition::getOriginPosition() const
  {
    if (!origin_)
    {
      return 0;
    }
    
    IStorage::IReceiptPtr originReceipt = origin_->payloadReceipt();
    if (!originReceipt)
    {
      return 0;
    }
    
    // get the payload position:
    uint64 originPosition = originReceipt->position();
    return originPosition;
  }
  
}
