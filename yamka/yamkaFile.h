// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 11:43:09 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_FILE_H_
#define YAMKA_FILE_H_

// yamka includes:
#include <yamkaCache.h>
#include <yamkaCrc32.h>
#include <yamkaIStorage.h>

// boost includes:
#include <boost/cstdint.hpp>

// system includes:
#include <string>
#include <stdexcept>
#include <cstdio>


namespace Yamka
{

#ifdef _WIN32
  //----------------------------------------------------------------
  // utf8_to_utf16
  //
  wchar_t *
  utf8_to_utf16(const char * utf8);

  //----------------------------------------------------------------
  // utf16_to_utf8
  //
  char *
  utf16_to_utf8(const wchar_t * str_utf16);

  //----------------------------------------------------------------
  // get_main_args_utf8
  //
  void
  get_main_args_utf8(int & argc, char **& argv);
#endif

  //----------------------------------------------------------------
  // fopen_utf8
  //
  std::FILE *
  fopen_utf8(const char * filename_utf8, const char * mode);

  //----------------------------------------------------------------
  // File
  //
  class File
  {
  public:
    ~File();

    //----------------------------------------------------------------
    // TSeek
    //
    typedef Seek<File> TSeek;

    //----------------------------------------------------------------
    // AccessMode
    //
    enum AccessMode
    {
      kReadOnly,
      kReadWrite
    };

    File(const std::string & pathUTF8 = std::string(),
         AccessMode fileMode = kReadOnly);

    // shallow copy (file handle is shared):
    File(const File & f);
    File & operator = (const File & f);

    // accessor to the file cache,
    // will return NULL if the file is not open:
    TCache * cache() const;

    // check whether the file handle is open:
    bool isOpen() const;

    // close current file handle.
    // NOTE: this applies to all copies of this File object
    //       due to implicit sharing
    void close();

    // close current file handle and open another file.
    // NOTE: this applies to all copies of this File object
    //       due to implicit sharing
    bool open(const std::string & pathUTF8,
              AccessMode fileMode = kReadOnly);

    // accessor to the filename (UTF-8):
    const std::string & filename() const;

    // seek to a specified file position:
    bool seek(TFileOffset offset,
              TPositionReference relativeTo = kAbsolutePosition);

    // helper:
    inline bool seekTo(TFileOffset absolutePosition)
    { return seek(absolutePosition, kAbsolutePosition); }

    // return current absolute file position:
    TFileOffset absolutePosition() const;

    // helper to get current file size:
    TFileOffset size();

    // truncate or extend file to a given size:
    bool setSize(TFileOffset size);

    // write out at current file position a specified number of bytes
    // from the source buffer:
    bool save(const void * src, std::size_t numBytes);

    // read at current file position a specified number of bytes
    // into the destination buffer:
    bool load(void * dst, std::size_t numBytes);

    // unlike load/save/read/write peek does not change the current
    // file position; otherwise peek behaves like load and
    // returns the number of bytes successfully loaded:
    std::size_t peek(void * dst, std::size_t numBytes) const;

    // calculate CRC-32 checksum over a region of this file:
    bool calcCrc32(uint64 seekToPosition,
                   uint64 numBytesToRead,
                   Crc32 & computeCrc32) const;

    // remove a given file:
    static bool remove(const char * filenameUTF8);

  private:
    // private implementation details:
    class Private;
    Private * const private_;
  };
}


#endif // YAMKA_FILE_H_
