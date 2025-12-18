/*
 * Copyright (C) 2026 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 */

#pragma once

#include <string>

#include <l4/re/util/debug>

struct Err : L4Re::Util::Err
{
  explicit Err(Level l = Normal) : L4Re::Util::Err(l, "bpmp") {}
};

class Dbg : public L4Re::Util::Dbg
{
public:
  enum Level
  {
    Warn   =  1,
    Info   =  2,
    // important BPMP requests (e.g. enable/disable clock)
    Trace1 =  4,
    // additional status messages
    Trace2 =  8,
    // less important BPMP requests (e.g. get temperature + response)
    Trace3 = 16,
    // all other read/write messages + IVC channel states
    Trace4 = 32,
    // raw MMIO register accesses (many messages)
    Trace5 = 64,
  };

  Dbg(unsigned long l = Info, char const *subsys = nullptr)
  : L4Re::Util::Dbg(l, "bpmp", subsys) {}

  explicit Dbg(unsigned long l, char const *component, std::string subsys)
  : L4Re::Util::Dbg(l, component, create_subsys_str(subsys)) {}

private:
  char const *create_subsys_str(std::string const &str)
  {
    snprintf(comp_str, sizeof(comp_str), "%s", str.c_str());
    return comp_str;
  }

  char comp_str[32];
};
