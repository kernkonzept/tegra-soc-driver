/*
 * Copyright (C) 2026 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
/**
 * \file
 * Pulse Width Modulator client access.
 *
 * The client either performs direct MMIO access on the device or it uses the
 * pwm service as proxy.
 */

#pragma once

#include <l4/re/util/debug>
#include <l4/drv/tegra/pwm/pwm_base.h>
#include <l4/drv/tegra/bpmp/bpmp_base.h>

class Tegra_pwm : public Tegra_pwm_base<Tegra_pwm>
{
public:
  Tegra_pwm(Tegra_bpmp *bpmp, l4_uint64_t mmio_base, l4_uint64_t mmio_size)
  : _bpmp(bpmp), _remote_service(false)
  {
    map_mmio(mmio_base, mmio_size);
  }

private:
  using Dbg = L4Re::Util::Dbg;

  void map_mmio(l4_uint64_t mmio_base, l4_uint64_t mmio_size)
  {
    if (_remote_service)
      _regs = new Mmio_space_register_block_32(mmio_space, addr, size);
    else
      _regs = new Mmio_map_register_block_32(_mbox.bus_cap(), addr, size);
  }

  Tegra_bpmp *_bpmp;

protected:
  Dbg log;
};
