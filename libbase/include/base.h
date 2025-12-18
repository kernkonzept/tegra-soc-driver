/*
 * Copyright (C) 2026 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
/**
 * \file
 * Base bits.
 *
 * Used by clients as well as by the server.
 */

#pragma once

#include <l4/cxx/bitfield>
#include <l4/drivers/hw_mmio_register_block>
#include <l4/re/util/debug>

/** Verbose MMIO read/write. */
class Tegra_mmio_verb_register_block_base
{
protected:
  l4_addr_t _base;
  L4Re::Util::Dbg const *_log;

public:
  explicit Tegra_mmio_verb_register_block_base(l4_addr_t base,
                                               L4Re::Util::Dbg const *log)
  : _base(base), _log(log)
  {}

  template< typename T >
  T read(l4_addr_t reg) const
  {
    T val = Asm_access::read(reinterpret_cast<T const *>(_base + reg));
    if (_log)
      {
        _log->printf("read reg=%04lx => %*.*x\n",
                     reg, int{sizeof(T) *2}, int{sizeof(T) * 2}, val);
      }

    return val;
  }

  template< typename T >
  void write(T val, l4_addr_t reg) const
  {
    if (_log)
      {
        T old = Asm_access::read(reinterpret_cast<T const *>(_base + reg));
        _log->printf("write reg=%04lx val=%*.*x (old=%*.*x)\n",
                     reg,
                     int{sizeof(T) * 2}, int{sizeof(T) * 2}, val,
                     int{sizeof(T) * 2}, int{sizeof(T) * 2}, old);
      }
    Asm_access::write(val, reinterpret_cast<T *>(_base + reg));
  }
};

struct Tegra_mmio_verb_register_block
: L4drivers::Register_block_impl<Tegra_mmio_verb_register_block, 32>,
  Tegra_mmio_verb_register_block_base
{
  explicit Tegra_mmio_verb_register_block(l4_addr_t base,
                                          L4Re::Util::Dbg const *log)
  : Tegra_mmio_verb_register_block_base(base, log) {}
};
