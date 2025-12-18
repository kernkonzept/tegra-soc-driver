/*
 * Copyright (C) 2026 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
/**
 * \file
 * Pulse Width Modulator basic bits.
 *
 * Used by clients as well as by the server.
 */

#pragma once

#include <l4/cxx/bitfield>
#include <l4/drv/tegra/bpmp/bpmp_base.h>

template <class IMPL, class BPMP>
class Tegra_pwm_base
{
public:
  Tegra_pwm_base(BPMP *bpmp)
  : bpmp(bpmp)
  {}

  void fan_enable()
  {
    fan_clk_enable();
    fan_reset_deassert();
    Tegra_pwm_csr_0_0 csr_0_0;
    csr_0_0.enb() = 1;
    csr_0_0.pfm_0() = 0; // fixed value
    csr_0_0.pwm_0() = 88; // fixed value
    _regs[0] = csr_0_0.raw;
  }

private:
  struct Tegra_pwm_csr_0_0
  {
    l4_uint32_t raw = 0;
    CXX_BITFIELD_MEMBER( 0, 12, pfm_0, raw);      ///< frequency divider
    CXX_BITFIELD_MEMBER(16, 30, pwm_0, raw);      ///< pulse N width N/256
    CXX_BITFIELD_MEMBER(31, 31, enb, raw);        ///< 0: disable, 1: enable
  };

  int fan_clk_enable()
  {
    Tegra_bpmp_if::Mrq_clk enable_clk{Tegra_bpmp_if::Mrq_clk::Req::Enable::Cmd,
                                      Tegra_bpmp_if::Mrq_clk::Clk_id::Tegra234_clk_pwm3};
    if (int ret = bpmp->chan_cpu_tx_transfer(enable_clk) != 0)
      {
        auto const &impl = static_cast<IMPL &>(*this);
        impl.log.printf("could not enable clock for PWM3 fan (error %d)\n", ret);
        return -L4_EINVAL;
      }

    return L4_EOK;
  }

  int fan_reset_deassert()
  {
    Tegra_bpmp_if::Mrq_reset deassert{
      Tegra_bpmp_if::Mrq_reset::Reset_cmd::Deassert,
      Tegra_bpmp_if::Mrq_reset::Reset_id::Tegra234_reset_pwm3};
    if (int ret = bpmp->chan_cpu_tx_transfer(deassert) != 0)
      {
        auto const &impl = static_cast<IMPL &>(*this);
        impl.log.printf( "could not reset PWM3 fan (error %d)\n", ret);
        return -L4_EINVAL;
      }

    return L4_EOK;
  }

protected:
  // false=local (direct MMIO access), true=remote (Mmio_space access)
  bool _remote_service = false;

  // local/remote: either direct device access (local) or MMIO space (remote)
  L4drivers::Register_block<32> _regs;

private:
  BPMP *bpmp;
};
