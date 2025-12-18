/*
 * Copyright (C) 2024 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 */

#pragma once

#include <l4/cxx/ipc_stream>
#include <l4/cxx/ref_ptr>
#include <l4/re/error_helper>
#include <l4/re/mmio_space>
#include <l4/re/util/object_registry>
#include <l4/vbus/vbus>

#include <vector>

#include "debug.h"
#include "tegra_bpmp.h"

/**
 * Client handling.
 * - Receive doorbell IRQ and forward IRQ to client.
 * - Read/write HSP (determine resources, trigger doorbell i=3, doorbell pending
 *   i=1).
 * - Read/write SRAM.
 */
class Client
: public L4::Epiface_t<Client,
                       L4::Kobject_2t<void, L4vbus::Vbus, L4Re::Mmio_space>>,
  public cxx::Ref_obj
{
  // vbus ICU
  static constexpr l4vbus_device_handle_t Id_icu = 0x23455432;
  static constexpr char const *Hidname_icu = "L40009";

  // hsp@3c00000
  static constexpr l4vbus_device_handle_t Id_hsp = 0x23455433;
  static constexpr char const *Hidname_hsp = "tegra234-hsp";
  static constexpr l4_uint64_t Res_hsp_start = 0x3c0'0000;
  static constexpr l4_uint64_t Res_hsp_end
    = Res_hsp_start + Tegra_hsp::Res_size_hsp - 1;
  /// IRQ number for doorbell IRQ
  static constexpr unsigned Res_hsp_num_irq_db = Tegra_hsp::Num_irq_db;
  /// IRQ numbers for shared semaphores
  static constexpr unsigned Res_hsp_num_irq_sm = Tegra_hsp::Num_irq_sm;

  // sram@40000000
  static constexpr l4vbus_device_handle_t Id_sram = 0x23455434;
  static constexpr char const *Hidname_sram = "tegra234-sysram";
  static constexpr l4_uint64_t Res_sram_start = 0x4000'0000;
  static constexpr l4_uint64_t Res_sram_end
    = Res_sram_start + Tegra_hsp::Res_size_sram - 1;

  static_assert(Tegra_bpmp::Offs_sram + Tegra_bpmp::Size_sram
                <= Tegra_hsp::Res_size_hsp);

public:
  Client(L4Re::Util::Object_registry *registry, Tegra_bpmp *pdev,
         std::string const &profile,
         Tegra_bpmp::Id_vec_bundle<l4_uint16_t> const &clocks,
         Tegra_bpmp::Id_vec_bundle<l4_uint8_t> const &powergates,
         Tegra_bpmp::Id_vec_bundle<l4_uint8_t> const &resetlines);
  ~Client() {}

   // Mmio_space interface
  long op_mmio_read(L4::Typeid::Rights<L4Re::Mmio_space>,
                    L4Re::Mmio_space::Addr addr, char log2_size,
                    l4_uint64_t &value)
  {
    try
      {
        return handle_mmio_read(addr, log2_size, value);
      }
    catch (L4::Runtime_error const &e)
      {
        warn.printf("Mmio_space read: %s: %s\n", e.extra_str(), e.str());
        return e.err_no();
      }
  }
  long op_mmio_write(L4::Typeid::Rights<L4Re::Mmio_space>,
                     L4Re::Mmio_space::Addr addr, char log2_size,
                     l4_uint64_t value)
  {
    try
      {
        return handle_mmio_write(addr, log2_size, value);
      }
    catch (L4::Runtime_error const &e)
      {
        warn.printf("Mmio_space write: %s: %s\n", e.extra_str(), e.str());
        return e.err_no();
      }
  }

  // IO server dispatch interface
  l4_msgtag_t op_dispatch(l4_utcb_t *utcb, l4_msgtag_t tag, L4vbus::Vbus::Rights)
  {
    L4::Ipc::Iostream ios(utcb);
    ios.Istream::tag() = tag;

    l4vbus_device_handle_t devid;
    if (L4_UNLIKELY(!ios.get(devid)))
      return l4_msgtag(-L4_EMSGTOOSHORT, 0, 0, 0);

    l4_uint32_t func;
    if (L4_UNLIKELY(!ios.get(func)))
      return l4_msgtag(-L4_EMSGTOOSHORT, 0, 0, 0);

    long ret = handle_vbus_msg(devid, func, ios);

    return ios.prepare_ipc(ret);
  }

  // Dataspace interface for IO -- all dummy because of Mmio_space access.
  long op_allocate(L4Re::Dataspace::Rights, l4_addr_t, l4_size_t)
  { return -L4_ENOSYS; }
  long op_copy_in(L4Re::Dataspace::Rights, L4Re::Dataspace::Offset,
                  L4::Ipc::Snd_fpage const &, L4Re::Dataspace::Offset,
                  L4Re::Dataspace::Size)
  { return -L4_ENOSYS; }
  long op_info(L4Re::Dataspace::Rights, L4Re::Dataspace::Stats &)
  { return -L4_ENOSYS; }
  long op_clear(L4Re::Dataspace::Rights, L4Re::Dataspace::Offset,
                L4Re::Dataspace::Size)
  { return -L4_ENOSYS; }
  long op_map(L4Re::Dataspace::Rights, L4Re::Dataspace::Offset,
              L4Re::Dataspace::Map_addr, L4Re::Dataspace::Flags,
              L4::Ipc::Snd_fpage &)
  {
    warn.printf("\033[31mop_map => -ENOSYS\033[m\n");
    return -L4_ENOSYS;
  }
  long op_map_info(L4Re::Dataspace::Rights, l4_addr_t &, l4_addr_t &)
  {
    warn.printf("\033[31mop_map_info => -ENOSYS\033[m\n");
    return -L4_ENOSYS;
  }

  // ICU -- bind/unbind to/from virtual IRQ
  long op_bind(L4::Icu::Rights, l4_umword_t irq_num, L4::Ipc::Snd_fpage irq_cap_fp)
  { return bind_irq(irq_num, irq_cap_fp); }
  int op_unbind(L4::Icu::Rights, l4_umword_t irq_num, L4::Ipc::Snd_fpage)
  { return unbind_irq(irq_num); }
  int op_info(L4::Icu::Rights, L4::Icu::_Info &)
  { return -L4_ENOSYS; }
  int op_msi_info(L4::Icu::Rights, l4_umword_t, l4_uint64_t, l4_icu_msi_info_t &)
  { return -L4_ENOSYS; }
  int op_mask(L4::Icu::Rights, l4_umword_t)
  { return -L4_ENOSYS; }
  int op_unmask(L4::Icu::Rights, l4_umword_t irq_num)
  {
    _pdev->unmask_irq(_cl_state, irq_num);
    return L4_EOK;
  }
  int op_set_mode(L4::Icu::Rights, l4_umword_t irq_num, l4_umword_t mode)
  {
    info.printf("\033[35mop_set_mode num=%lx mode=%lx\033[m\n", irq_num, mode);
    if (irq_num == Res_hsp_num_irq_db
        || (   irq_num >= Res_hsp_num_irq_sm
            && irq_num <  Res_hsp_num_irq_sm + Tegra_hsp::Count_sm))
      {
        if (mode == (L4_IRQ_F_SET_MODE | L4_IRQ_F_LEVEL | L4_IRQ_F_POS))
          return L4_EOK;

        return -L4_ENOSYS;
      }

    return -L4_EINVAL;
  }

  // Event server for Vbus events -- dummy
  long op_get_buffer(L4Re::Event::Rights, L4::Ipc::Cap<L4Re::Dataspace> &)
  { return -L4_ENOSYS; }
  long op_get_num_streams(L4Re::Event::Rights)
  { return -L4_ENOSYS; }
  long op_get_stream_info(L4Re::Event::Rights, int, L4Re::Event_stream_info)
  { return -L4_ENOSYS; }
  long op_get_stream_info_for_id(L4Re::Event::Rights, l4_umword_t,
                                 L4Re::Event_stream_info)
  { return -L4_ENOSYS; }
  long op_get_axis_info(L4Re::Event::Rights, l4_umword_t,
                        L4::Ipc::Array_in_buf<unsigned, unsigned long> const &,
                        L4::Ipc::Array_ref<L4Re::Event_absinfo, unsigned long> &)
  { return -L4_ENOSYS; }
  long op_get_stream_state_for_id(L4Re::Event::Rights, l4_umword_t,
                                  L4Re::Event_stream_state &)
  { return -L4_ENOSYS; }

  // Inhibitor -- dummy
  long op_acquire(L4Re::Inhibitor::Rights, l4_umword_t, L4::Ipc::String<>)
  { return L4_EOK; }
  long op_release(L4Re::Inhibitor::Rights, l4_umword_t)
  { return L4_EOK; }
  long op_next_lock_info(L4Re::Inhibitor::Rights, l4_mword_t &,
                         L4::Ipc::String<char> &)
  { return -L4_ENOSYS; }

  /** IRQ notification from device. */
  void handle_irq(unsigned irq_num);

private:
  long handle_vbus_msg(l4vbus_device_handle_t devid, l4_int32_t func,
                       L4::Ipc::Iostream &ios);
  long handle_mmio_read(L4Re::Mmio_space::Addr addr, char log2_size,
                        l4_uint64_t &value);
  long handle_mmio_write(L4Re::Mmio_space::Addr addr, char log2_size,
                         l4_uint64_t value);
  long bind_irq(l4_umword_t irq_num, L4::Ipc::Snd_fpage irq_cap_fp);
  long unbind_irq(l4_umword_t irq_num);
  long get_hsp_device(L4::Ipc::Iostream &ios) const;
  long get_hsp_device_resource(int res_idx, L4::Ipc::Iostream &ios) const;
  long get_sram_device(L4::Ipc::Iostream &ios) const;
  long get_sram_device_resource(int res_idx, L4::Ipc::Iostream &ios) const;
  long get_icu_device(L4::Ipc::Iostream &ios) const;

  static constexpr bool hid_eq_hidname(char const *hid, unsigned long sz,
                                       char const *hidname)
  { return sz == __builtin_strlen(hidname) + 1 && !strcmp(hid, hidname); }

  static constexpr l4_uint32_t str_to_id(char const *s)
  {
    l4_uint32_t id = 0;
    for (unsigned i = 0; i < 4 && s[i]; ++i)
      id |= s[i] << 8*i;

    return id;
  }

  static constexpr char const *dev_id_to_str(l4vbus_device_handle_t devid)
  {
    if (devid == Id_icu)
      return "icu";
    else if (devid == Id_hsp)
      return "hsp";
    else if (devid == Id_sram)
      return "sram";
    else
      return "???";
  }

  using Irq_client_ptr = L4::Cap<L4::Irq> *;
  constexpr Irq_client_ptr irq_num_to_irq_client(unsigned irq_num)
  {
    if (irq_num == Res_hsp_num_irq_db)
      return &_cl_state.irq_client_db;
    else if (   irq_num >= Res_hsp_num_irq_sm
             && irq_num <  Res_hsp_num_irq_sm + Tegra_hsp::Count_sm)
      return &_irq_client_sm[irq_num - Res_hsp_num_irq_sm];
    else
      return nullptr;
  }

  // client profile name
  std::string _profile;
  // Pointer to physical device to forward MMIO access. The emulation if
  // implemented there.
  Tegra_bpmp *_pdev = nullptr;

  // client IRQ sink for shared semaphore IRQs
  L4::Cap<L4::Irq> _irq_client_sm[Tegra_hsp::Count_sm];

  // Per-client device state.
  Tegra_bpmp::Client_state _cl_state;

  Dbg warn;
  Dbg info;
  Dbg trace1;
  Dbg trace2;
};
