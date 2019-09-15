// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Sep  1 11:43:52 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ASSERT_H_
#define YAE_ASSERT_H_

// aeyae:
#include "../api/yae_log.h"


//----------------------------------------------------------------
// YAE_BREAKPOINT
//
#if defined(__APPLE__)
#  if defined(__ppc__)
#    define YAE_BREAKPOINT() __asm { trap }
#  else
#    define YAE_BREAKPOINT() asm("int $3")
#  endif
#elif __GNUC__
#  define YAE_BREAKPOINT() asm("int $3")
#else
#  define YAE_BREAKPOINT()
#endif

//----------------------------------------------------------------
// YAE_BREAKPOINT_IF
//
#define YAE_BREAKPOINT_IF(expr) if (!(expr)) {} else YAE_BREAKPOINT()

//----------------------------------------------------------------
// YAE_BREAKPOINT_IF_DEBUG_BUILD
//
#ifndef NDEBUG
#define YAE_BREAKPOINT_IF_DEBUG_BUILD() YAE_BREAKPOINT()
#else
#define YAE_BREAKPOINT_IF_DEBUG_BUILD()
#endif

//----------------------------------------------------------------
// YAE_ASSERT
//
#define YAE_ASSERT(expr) if (!(expr)) {          \
    yae::log(yae::TLog::kError,                  \
             __FILE__ ":" YAE_STR(__LINE__),     \
             "assertion failed: %s",             \
             YAE_STR(expr));                     \
    YAE_BREAKPOINT_IF_DEBUG_BUILD();             \
  } else {}


#endif // YAE_ASSERT_H_