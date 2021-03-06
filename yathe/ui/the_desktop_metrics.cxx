// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_desktop_metrics.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Feb 5 16:02:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : The base class for desktop metrics (DPI)


// system includes:
#include <stdlib.h>

// local includes:
#include "ui/the_desktop_metrics.hxx"
#include "utils/the_dynamic_array.hxx"

//----------------------------------------------------------------
// metrics
//
static the_dynamic_array_t<the_desktop_metrics_t *> metrics(0, 1, NULL);

//----------------------------------------------------------------
// the_desktop_metrics
//
const the_desktop_metrics_t *
the_desktop_metrics(unsigned int desktop)
{
  if (desktop >= metrics.size())
  {
    return NULL;
  }

  return metrics[desktop];
}

//----------------------------------------------------------------
// the_desktop_metrics
//
void
the_desktop_metrics(the_desktop_metrics_t * m, unsigned int desktop)
{
  if (desktop >= metrics.size())
  {
    metrics.resize(desktop + 1);
    metrics[desktop] = m;
  }
  else
  {
    delete metrics[desktop];
    metrics[desktop] = m;
  }
}
