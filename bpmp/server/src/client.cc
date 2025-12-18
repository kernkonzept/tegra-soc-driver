/*
 * Copyright (C) 2026 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 * Client interface.
 */

#include <l4/vbus/vdevice-ops.h>

#include "client.h"
#include "tegra_bpmp.h"

Client::Client(L4Re::Util::Object_registry *registry, Tegra_bpmp *pdev,
               std::string const &profile,
               Tegra_bpmp::Id_vec_bundle<l4_uint16_t> const &clocks,
               Tegra_bpmp::Id_vec_bundle<l4_uint8_t> const &powergates,
               Tegra_bpmp::Id_vec_bundle<l4_uint8_t> const &resetlines)
: _profile(profile),
  _pdev(pdev),
  _cl_state{clocks, powergates, resetlines},
  warn{Dbg::Warn, "client", profile},
  info{Dbg::Info, "client", profile},
  trace1{Dbg::Trace1, "client", profile},
  trace2{Dbg::Trace2, "client", profile}
{
  info.printf("New client profile='%s' TX/RX @ %p created.\n",
              profile.c_str(), _cl_state.msgs.get());

  L4Re::chkcap(registry->register_obj(this), "Register client");
}

long
Client::handle_vbus_msg(l4vbus_device_handle_t devid, l4_int32_t func,
                        L4::Ipc::Iostream &ios)
{
  trace2.printf("handle_vbus_msg func = %08x\n", func);
  switch (func)
    {
    case L4vbus_vicu_get_cap:
      if (devid == Id_icu)
        ios << obj_cap();
      else
        return -L4_ENODEV;
      break;

    case L4vbus_vdevice_get_by_hid:
      {
        l4vbus_device_handle_t child;
        if (L4_UNLIKELY(!ios.get(child)))
          return -L4_EMSGTOOSHORT;

        int depth;
        if (L4_UNLIKELY(!ios.get(depth)))
          return -L4_EMSGTOOSHORT;

        unsigned long sz;
        char const *hid = 0;
        ios >> L4::Ipc::buf_in(hid, sz);
        if (!hid)
          return -L4_EINVAL;

        trace2.printf(
          "handle_vbus_msg: L4vbus_cl_stateice_get_by_hid: hid='%.*s'\n",
          static_cast<int>(sz), hid);
        if (hid_eq_hidname(hid, sz, Hidname_icu))
          return get_icu_device(ios);
        else if (hid_eq_hidname(hid, sz, Hidname_hsp))
          return get_hsp_device(ios);
        else if (hid_eq_hidname(hid, sz, Hidname_sram))
          return get_sram_device(ios);
        else
          return -L4_EINVAL;
      }

    case L4vbus_vdevice_get_resource:
      {
        int res_idx;
        if (!ios.get(res_idx))
          return -L4_EMSGTOOSHORT;
        trace2.printf(
          "handle_vbus_msg: L4vbus_cl_stateice_get_resource: devid=%s res=%u\n",
          dev_id_to_str(devid), res_idx);
        if (devid == Id_hsp)
          return get_hsp_device_resource(res_idx, ios);
        if (devid == Id_sram)
          return get_sram_device_resource(res_idx, ios);
        else
          return -L4_ENOENT;
      }

    default:
      warn.printf("Unknown vbus function: devid=0x%lx func=0x%x\n", devid, func);
      return -L4_ENOSYS;
    }

  return L4_EOK;
}

long
Client::handle_mmio_read(L4Re::Mmio_space::Addr addr, char log2_size,
                         l4_uint64_t &value)
{
  using L4Re::Mmio_space;
  if (log2_size < Mmio_space::Wd_8bit || log2_size > Mmio_space::Wd_64bit)
    L4Re::throw_error_fmt(-L4_EINVAL,
                          "mmio: Invalid %d-bit read access", 8 << log2_size);
  if (addr >= Res_hsp_start && addr <= Res_hsp_end - (1U << log2_size))
    _pdev->mmio_read_hsp(_cl_state, addr - Res_hsp_start, log2_size, value);
  else if (addr >= Res_sram_start && addr <= Res_sram_end - (1U << log2_size))
    _pdev->mmio_read_sram(_cl_state, addr - Res_sram_start, log2_size, value);
  else
    L4Re::throw_error_fmt(-L4_ERANGE,
                          "mmio: Read offset %llx out of range", addr);
  return L4_EOK;
}

long
Client::handle_mmio_write(L4Re::Mmio_space::Addr addr, char log2_size,
                          l4_uint64_t value)
{
  using L4Re::Mmio_space;
  if (log2_size < Mmio_space::Wd_8bit || log2_size > Mmio_space::Wd_64bit)
    L4Re::throw_error_fmt(-L4_EINVAL,
                          "mmio: Invalid %d-bit write access", 8 << log2_size);
  if (addr >= Res_hsp_start && addr <= Res_hsp_end - (1U << log2_size))
    _pdev->mmio_write_hsp(_cl_state, addr - Res_hsp_start, log2_size, value);
  else if (addr >= Res_sram_start && addr <= Res_sram_end - (1U << log2_size))
    _pdev->mmio_write_sram(_cl_state, addr - Res_sram_start, log2_size, value);
  else
    L4Re::throw_error_fmt(-L4_ERANGE,
                          "mmio: Write offset %llx out of range", addr);
  return L4_EOK;
}

long
Client::get_hsp_device(L4::Ipc::Iostream &ios) const
{
  l4vbus_device_t hsp;
  snprintf(hsp.name, sizeof(hsp.name), "%s", Hidname_hsp);
  hsp.flags = 0;
  hsp.num_resources = 2 + Tegra_hsp::Count_sm;
  hsp.type = 0;
  ios << l4vbus_device_handle_t(Id_hsp);
  ios.put(hsp);
  return L4_EOK;
}

long
Client::get_hsp_device_resource(int res_idx, L4::Ipc::Iostream &ios) const
{
  l4vbus_resource_t res;
  if (res_idx == 0)
    {
      res.start = Res_hsp_start;
      res.end = Res_hsp_end;
      res.type = L4VBUS_RESOURCE_MEM;
      res.flags = L4VBUS_RESOURCE_F_MEM_MMIO_READ | L4VBUS_RESOURCE_F_MEM_MMIO_WRITE;
      res.provider = 0;
      res.id = str_to_id("reg0");
    }
  else if (res_idx >= 1 && res_idx < 2 + int{Tegra_hsp::Count_sm})
    {
      res.start = res_idx == 1 ? Res_hsp_num_irq_db
                               : Res_hsp_num_irq_sm + res_idx - 2;
      res.end = res.start;
      res.type = L4VBUS_RESOURCE_IRQ;
      res.flags = L4_IRQ_F_LEVEL | L4_IRQ_F_POS;
      res.provider = 0;
      res.id = str_to_id("irq0") | ((res_idx - 1) << 24);
    }
  else
    return -L4_ENOENT;
  ios.put(res);
  return L4_EOK;
}

long
Client::get_sram_device(L4::Ipc::Iostream &ios) const
{
  l4vbus_device_t sram;
  snprintf(sram.name, sizeof(sram.name), "%s", Hidname_sram);
  sram.flags = 0;
  sram.num_resources = 1;
  sram.type = 0;
  ios << l4vbus_device_handle_t(Id_sram);
  ios.put(sram);
  return L4_EOK;
}

long
Client::get_sram_device_resource(int res_idx, L4::Ipc::Iostream &ios) const
{
  l4vbus_resource_t res;
  switch (res_idx)
    {
    case 0:
      res.start = Res_sram_start;
      res.end = Res_sram_end;
      res.type = L4VBUS_RESOURCE_MEM;
      res.flags = L4VBUS_RESOURCE_F_MEM_MMIO_READ
                | L4VBUS_RESOURCE_F_MEM_MMIO_WRITE;
      res.provider = 0;
      res.id = str_to_id("reg0");
      break;
    default:
      return -L4_ENOENT;
    }
  ios.put(res);
  return L4_EOK;
}

long
Client::get_icu_device(L4::Ipc::Iostream &ios) const
{
  l4vbus_device_t icu;
  snprintf(icu.name, sizeof(icu.name), "%s", Hidname_icu);
  icu.flags = 0;
  icu.num_resources = 0;
  icu.type = 0;
  ios << l4vbus_device_handle_t(Id_icu);
  ios.put(icu);
  return L4_EOK;
}

long
Client::bind_irq(l4_umword_t irq_num, L4::Ipc::Snd_fpage irq_cap_fp)
{
  if (!irq_cap_fp.cap_received())
    return -L4_EINVAL;
  L4::Cap<L4::Irq> irqc = server_iface()->rcv_cap<L4::Irq>(0);
  server_iface()->realloc_rcv_cap(0);
  if (!irqc.is_valid())
    return -L4_EINVAL;
  l4_msgtag_t msg = L4Re::Env::env()->task()->cap_valid(irqc);
  if (msg.label() == 0)
    return -L4_EINVAL;
  if (L4::Cap<L4::Irq> *irq_client = irq_num_to_irq_client(irq_num))
    {
      if (irq_client->is_valid())
        return -L4_EBUSY;
      *irq_client = irqc;
      return 1L; // unmask via ICU object
    }
  else
    return -L4_EINVAL;
}

long
Client::unbind_irq(l4_umword_t irq_num)
{
  if (L4::Cap<L4::Irq> *irq_client = irq_num_to_irq_client(irq_num))
    {
      *irq_client = L4::Cap<L4::Irq>::Invalid;
      return L4_EOK;
    }
  else
    return -L4_EINVAL;
}

void
Client::handle_irq(unsigned irq_num)
{
  if (auto *irq_client = irq_num_to_irq_client(irq_num))
    {
      if (irq_client == &_cl_state.irq_client_db)
        L4Re::throw_error(-L4_EINVAL,
                          "Client::handle_irq called with irq_num=DB");
      if (irq_client->is_valid())
        {
          if (long ret = l4_ipc_error((*irq_client)->trigger(), l4_utcb()))
            {
              warn.printf("notify IRQ failed (ret=%ld)\n", ret);
              *irq_client = L4::Cap<L4::Irq>::Invalid;
            }
        }
      else
        warn.printf(
          "\033[31mClient::handle_irq: no client bound to IRQ %x\033[m\n",
          irq_num);
    }
  else
    L4Re::throw_error_fmt(-L4_EINVAL,
                          "Client::handle_irq wrong IRQ number %x", irq_num);
}
