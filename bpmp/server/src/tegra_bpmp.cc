/*
 * Copyright (C) 2026 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 * Driver for the physical Tegra BPMP device.
 */

#include <cassert>

#include <l4/re/error_helper>

#include "tegra_bpmp.h"

int
Tegra_hsp::init(L4Re::Util::Object_registry *registry,
                L4::Cap<L4vbus::Vbus> vbus,
                L4vbus::Device dev, l4vbus_device_t devinfo)
{
  L4vbus::Icu icudev;
  L4Re::chksys(vbus->root().device_by_hid(&icudev, "L40009"),
               "hsp: Lookup ICU device");
  L4Re::chksys(icudev.vicu(_icu), "hsp: Request ICU capability");

  for (unsigned i = 0; i < devinfo.num_resources; ++i)
    {
      l4vbus_resource_t res;
      L4Re::chksys(dev.get_resource(i, &res), "hsp: Get device resource");
      char const *resname = reinterpret_cast<char const *>(&res.id);
      if (res.type == L4VBUS_RESOURCE_MEM)
        {
          if (!strncmp(resname, "reg0", 4))
            init_mmio(vbus, res.start, res.end - res.start + 1);
          else
            L4Re::throw_error_fmt(
              -L4_EINVAL, "hsp: Wrong memory resource '%.4s'", resname);
        }
      else if (res.type == L4VBUS_RESOURCE_IRQ)
        {
          if (!strncmp(resname, "irq", 3)
              && resname[3] >= '0' && resname[3] < '0' + 1 + int{Count_sm})
            init_irq(registry, resname[3], res.start, L4_irq_mode(res.flags));
          else
            L4Re::throw_error_fmt(
              -L4_EINVAL, "hsp: Wrong IRQ resource '%.4s'", resname);
        }
    }

  // Attach MMIO.
  if (!_regs_region.get())
    L4Re::throw_error(-L4_EINVAL, "hsp: No MMIO resource");

  return L4_EOK;
}

void
Tegra_hsp::init_mmio(L4::Cap<L4vbus::Vbus> vbus,
                     l4_addr_t mmio_addr, l4_size_t mmio_size)
{
  auto offs = mmio_addr & ~L4_PAGEMASK;
  auto rm_flags = L4Re::Rm::F::Search_addr | L4Re::Rm::F::Cache_uncached
                | L4Re::Rm::F::RW | L4Re::Rm::F::Eager_map;
  auto const *e = L4Re::Env::env();
  L4Re::chksys(e->rm()->attach(&_regs_region, mmio_size, rm_flags,
                               L4::Ipc::make_cap_rw(vbus), mmio_addr),
               "hsp: Attach vbus device memory");
  _regs = new L4drivers::Mmio_register_block<32>(_regs_region.get() + offs);
  L4Re::chksys(Tegra_hsp_base<Tegra_hsp>::init(_regs_region.get() + offs),
               "hsp: Initialize device");

  info.printf("MMIO @ %08lx-%08lx.\n", mmio_addr, mmio_addr + mmio_size - 1);
}

void
Tegra_hsp::init_irq(L4Re::Util::Object_registry *registry,
                    char irq_id, unsigned irq_num, L4_irq_mode irq_mode)
{
  assert(irq_id >= '0' && irq_id < '0' + 1 + int{Count_sm});

  Tegra_irqep<Tegra_hsp> *ep = nullptr;
  if (irq_id == '0')
    ep = &_irqep_db;
  else
    ep = &_irqep_sm[irq_id - '1'];
  ep->register_ep(registry, irq_num, irq_mode);
}

/**
 * Client driver unmasks the IRQ.
 */
void
Tegra_hsp::unmask_irq(unsigned irq_num)
{
  Tegra_irqep<Tegra_hsp> *ep;
  if (irq_num == Num_irq_db)
    L4Re::throw_error(-L4_EINVAL, "hsp: unmask_irq(doorbell) directly called");
  else if (irq_num >= Num_irq_sm && irq_num < Num_irq_sm + Count_sm)
    ep = &_irqep_sm[irq_num - Num_irq_sm]; // shared semaphores
  else
    L4Re::throw_error_fmt(-L4_EINVAL,
                          "hsp: Unknown IRQ number %x during unmask", irq_num);

  ep->unmask();
}

Tegra_bpmp::Tegra_bpmp(L4Re::Util::Object_registry *registry, Tegra_hsp *hsp)
: Tegra_bpmp_base(hsp)
{
  auto vbus = L4Re::Env::env()->get_cap<L4vbus::Vbus>("vbus");
  if (!vbus.is_valid())
    {
      warn.printf("No 'vbus' capability found -- no HW device access!\n");
      return;
    }

  l4vbus_device_t devinfo;

  // hsp@3c00000
  L4vbus::Device dev_hsp;
  L4Re::chksys(vbus->root().device_by_hid(&dev_hsp, "tegra234-hsp",
                                          L4VBUS_MAX_DEPTH, &devinfo),
               "Lookup device with hid='tegra234-hsp'");
  if (int ret = _hsp->init(registry, vbus, dev_hsp, devinfo); ret < 0)
    L4Re::throw_error(-L4_EINVAL, "Cannot initialize 'hsp'");

  _hsp_reg_doorbell_ccplex_pending = _hsp->Reg_doorbell_ccplex_pending();
  _hsp_reg_doorbell_bpmp_trigger = _hsp->Reg_doorbell_bpmp_trigger();

  // sram@40000000
  L4vbus::Device dev_sram;
  L4Re::chksys(vbus->root().device_by_hid(&dev_sram, "tegra234-sysram",
                                          L4VBUS_MAX_DEPTH, &devinfo),
               "Lookup device with hid='tegra234-sysram'");
  init_sram(vbus, dev_sram, devinfo);

  for (unsigned i = 0; i < Count_msgs - 1; ++i)
    _msgs[i].next = _msgs + i + 1;
  _msgs[Count_msgs - 1].next = nullptr;
  _free_msgs_head = _msgs;

  l4_uint64_t pdev_pending;
  _hsp->mmio_read(_hsp_reg_doorbell_ccplex_pending, pdev_pending, false);
  _hsp->mmio_write(_hsp_reg_doorbell_ccplex_pending, pdev_pending);
  _hsp->unmask_doorbell_ccplex_irq();
}

void
Tegra_bpmp::init_sram(L4::Cap<L4vbus::Vbus> vbus,
                        L4vbus::Device dev, l4vbus_device_t devinfo)
{
  l4_addr_t mmio_addr = 0;
  l4_size_t mmio_size = 0;
  for (unsigned i = 0; i < devinfo.num_resources; ++i)
    {
      l4vbus_resource_t res;
      L4Re::chksys(dev.get_resource(i, &res), "sram: Get device resource");
      if (res.type == L4VBUS_RESOURCE_MEM)
        {
          if (mmio_size)
            L4Re::throw_error(-L4_EINVAL,
                              "sram: More than one MMIO region for 'sram'");
          mmio_addr = res.start;
          mmio_size = res.end - res.start + 1;

          auto offset = mmio_addr & ~L4_PAGEMASK;
          auto rm_flags = L4Re::Rm::F::Search_addr | L4Re::Rm::F::Cache_uncached
                        | L4Re::Rm::F::RW | L4Re::Rm::F::Eager_map;
          auto const *e = L4Re::Env::env();
          L4Re::chksys(e->rm()->attach(&_regs_region, mmio_size, rm_flags,
                                       L4::Ipc::make_cap_rw(vbus), mmio_addr),
                       "sram: Attach vbus device memory");
          // XXX
          unsigned offs_tx = _regs_region.get() + offset + Offs_sram;
          unsigned offs_rx = _regs_region.get() + offset + Offs_sram + 0x1000;
          if (Tegra_bpmp_base::init(offs_tx, 0x1000, offs_rx, 0x1000) < 0)
            L4Re::throw_error(-L4_EINVAL, "sram: Cannot initialized 'sram'");

          _regs = new L4drivers::Mmio_register_block<32>(_regs_region.get());
        }
    }

  if (!mmio_addr)
    L4Re::throw_error(-L4_EINVAL, "sram: No MMIO resource");
}

void
Tegra_bpmp::write_access_to_str(unsigned chan_offs, l4_uint32_t value,
                                char *str, size_t strsize)
{
  switch (chan_offs)
    {
    case offsetof(Ivc_msg, code):
      snprintf(str, strsize, " (%s)", Tegra_bpmp_if::req_to_str(value & 0xff));
      break;
    }
}

void
Tegra_bpmp::tbuf_log_counters(char const *str, Client_state const &cl_state,
                              unsigned chan)
{
  if constexpr (Tbuf_log)
    {
      Ivc_msg const &cl_msg_tx = cl_state.cl_msg_tx(chan);
      Ivc_msg const &cl_msg_rx = cl_state.cl_msg_rx(chan);
      char buf[68];
      snprintf(buf, sizeof(buf),
               "%s[%x] v:%u.%u.%u_%u.%u.%u p:%u.%u.%u_%u.%u.%u",
               str, chan,
               cl_msg_tx.snd_ep.count, cl_msg_tx.snd_ep.state, cl_msg_tx.rcv_ep.count,
               cl_msg_rx.snd_ep.count, cl_msg_rx.snd_ep.state, cl_msg_rx.rcv_ep.count,
               tx_snd_count(), tx_snd_state(), tx_rcv_count(),
               rx_snd_count(), rx_snd_state(), rx_rcv_count());
      fiasco_tbuf_log(buf);
    }
}

/**
 * Prevent certain messages from being submitted to the physical device.
 */
bool
Tegra_bpmp::filter_skip_cmd(Client_state &cl_state, unsigned chan)
{
  Ivc_msg const &cl_msg_tx = cl_state.cl_msg_tx(chan);

  bool skip = false;
  if (cl_msg_tx.code == Mrq_clk::Mrq)
    {
      auto &req = *reinterpret_cast<Mrq_clk::Req const *>(cl_msg_tx.data);
      if (req.filter_required())
        skip = cl_state.clocks.disallowed(req.cmd.id());
    }
  else if (cl_msg_tx.code == Mrq_pg::Mrq)
    {
      auto &req = *reinterpret_cast<Mrq_pg::Req const *>(cl_msg_tx.data);
      if (req.filter_required())
        skip = cl_state.powergates.disallowed(req.cmd.pg_id);
    }
  else if (cl_msg_tx.code == Mrq_reset::Mrq)
    {
      auto &req = *reinterpret_cast<Mrq_reset::Req const *>(cl_msg_tx.data);
      if (req.filter_required())
        skip = cl_state.resetlines.disallowed(req.reset_id);
    }
  if (!skip)
    return false;

  Ivc_msg &cl_msg_rx = cl_state.cl_msg_rx(chan);
  // pretend success to client
  cl_msg_rx.code = 0;
  // TELL CLIENT: virtual device sent response
  cl_msg_rx.mb_write_advance();

  log_skipped_msg(cl_msg_tx);

  // Notify client if requested.
  if (cl_msg_tx.flags & Ivc_msg::Flags_ring)
    {
      cl_state.doorbell_ccplex_pending |= (1U << Tegra_hsp::Doorbell_master_bpmp);
      if (!cl_state.doorbell_ccplex_masked)
        doorbell_ccplex_ring_at_client(cl_state);
    }

  return true;
}

/**
 * Create a message on the physical device.
 */
void
Tegra_bpmp::submit_cl_msg_to_pdev(Client_msg *msg,
                                  [[maybe_unused]] unsigned pdev_chan)
{
  assert(pdev_chan == Chan_cpu_tx);

  Client_state &cl_state = *(msg->cl_state);
  unsigned chan = msg->chan;
  Ivc_msg &cl_msg_tx = cl_state.cl_msg_tx(chan);

  unsigned msg_size;
  if (cl_state.tx_written[chan] >= offsetof(Ivc_msg, data))
    msg_size = cl_state.tx_written[chan] - offsetof(Ivc_msg, data);
  else
    msg_size = 0;

  tbuf_log_counters("befor writ" /*typos intended*/, cl_state, chan);

  // write + ring BPMP doorbell and expect interrupt on completion
  chan_cpu_tx_write_and_ring(cl_msg_tx.code, &cl_msg_tx.data, msg_size);

  tbuf_log_counters("after writ" /*typo intended*/, cl_state, chan);
}

/** Log the content of skipped messages. */
void
Tegra_bpmp::log_skipped_msg(Ivc_msg const &cl_msg_tx) const
{
  char const *prefix = "\033[31;1mSKIP ";
  switch (cl_msg_tx.code)
    {
    case Mrq_debug::Mrq:
      {
        auto &req = reinterpret_cast<Mrq_debug::Req const &>(cl_msg_tx.data);
        req.log(warn, prefix);
        break;
      }
    case Mrq_clk::Mrq:
      {
        auto &req = reinterpret_cast<Mrq_clk::Req const &>(cl_msg_tx.data);
        req.log(warn, prefix);
        break;
      }
    case Mrq_thermal::Mrq:
      {
        auto &req = reinterpret_cast<Mrq_thermal::Req const &>(cl_msg_tx.data);
        req.log(warn, prefix);
        break;
      }
    case Mrq_pg::Mrq:
      {
        auto &req = reinterpret_cast<Mrq_pg::Req const &>(cl_msg_tx.data);
        req.log(warn, prefix);
        break;
      }
    case Mrq_reset::Mrq:
      {
        auto &req = reinterpret_cast<Mrq_reset::Req const &>(cl_msg_tx.data);
        req.log(warn, prefix);
        break;
      }
    default:
      warn.printf("%sreq=%s, cmd=%08x\033[m\n",
                  prefix, Tegra_bpmp_if::req_to_str(cl_msg_tx.code),
                  cl_msg_tx.data[0]);
      break;
    }
}

/**
 * Client driver writes HSP to trigger the BPMP doorbell.
 *
 * We need to check all channels:
 * - If TX_msg.snd_ep.count was increased, client submitted new message which we
 *   need to submit to the physical device.
 *   - Allocate new message.
 *   - If possible, submit to physical device, otherwise add to "active" list.
 * - If RX_msg.rcv_ep.count was increased, client acknowledged to have read the
 *   response.
 *   - That case is not relevant here. It is only relevant for reading the HSP
 *     CCPLEX doorbell status.
 */
void
Tegra_bpmp::doorbell_bpmp_trigger(Client_state &cl_state)
{
  if constexpr (Tbuf_log)
    fiasco_tbuf_log("drv triggered doorbell");

  // check TX channels if 'snd_ep.count' was increased
  for (unsigned chan = 0; chan < Count_ivc_chan_pairs; ++chan)
    {
      Ivc_msg &cl_msg_tx = cl_state.cl_msg_tx(chan);
      if (cl_msg_tx.snd_ep.state == Ivc_msg::State_established)
        if (cl_msg_tx.snd_ep.count != cl_msg_tx.rcv_ep.count)
          {
            // TELL CLIENT: virtual device received message
            // This has to be done first to ensure that we don't process this
            // message twice.
            cl_msg_tx.mb_read_advance();

            if (!filter_skip_cmd(cl_state, chan))
              {
                Client_msg *msg = Client_msg::new_msg(*this, &cl_state, chan);
                unsigned pdev_chan;
                if (acquire_pdev_chan(msg, pdev_chan))
                  submit_cl_msg_to_pdev(msg, pdev_chan);
                else
                  {
                    if constexpr (Tbuf_log)
                      {
                        ++_nr_pending;
                        fiasco_tbuf_log_3val("ADD MSG TO PENDING",
                                             _nr_pending, 0, 0);
                      }
                    msg->add_to_active(*this);
                  }
              }
          }
    }
}

/**
 * Client driver unmasks CCPLEX doorbell IRQ.
 */
void
Tegra_bpmp::doorbell_ccplex_unmask(Client_state &cl_state)
{
  if constexpr (Tbuf_log)
    fiasco_tbuf_log_3val("drv unmask doorbell",
                         cl_state.doorbell_ccplex_pending, 0, 0);
  cl_state.doorbell_ccplex_masked = false;
  if (cl_state.doorbell_ccplex_pending)
    doorbell_ccplex_ring_at_client(cl_state);
}

/**
 * Device raised CCPLEX doorbell IRQ to tell driver that relevant status
 * has changed.
 *
 * Fetch the relevant state from the physical device and put it onto the client
 * state before the caller (Client::handle_irq()) notifies the client.
 */
void
Tegra_bpmp::doorbell_ccplex_handle()
{
  l4_uint64_t pdev_pending;
  _hsp->mmio_read(_hsp_reg_doorbell_ccplex_pending, pdev_pending, false);
  _hsp->mmio_write(_hsp_reg_doorbell_ccplex_pending, pdev_pending);

  if constexpr (Tbuf_log)
    fiasco_tbuf_log_3val("doorbell_handle", pdev_pending, 0, 0);

  if (pdev_pending & (1U << Tegra_hsp::Doorbell_master_bpmp))
    {
      // XXX check for all pdev channels
      if (Client_msg *msg = get_pdev_active_client_msg(Chan_cpu_tx))
        {
          Client_state &cl_state = *(msg->cl_state);
          unsigned chan = msg->chan;
          Ivc_msg &cl_msg_rx = cl_state.cl_msg_rx(chan);

          // The CCPLEX doorbell is usually triggered twice: Once for signaling
          // that the message was received, once again for signaling that the
          // response is available.
          if (rx_snd_count() != rx_rcv_count())
            {
              tbuf_log_counters("befor read" /*typo intended*/, cl_state, chan);

              Ivc_msg const &cl_msg_tx = cl_state.cl_msg_tx(chan);
              chan_cpu_tx_read(cl_msg_tx.code /*only for logging*/,
                               cl_msg_tx.data[0] /*only for logging*/,
                               &cl_msg_rx.code, cl_msg_rx.data,
                               sizeof(Ivc_msg::data));

              // TELL CLIENT: A response is available.
              cl_msg_rx.mb_write_advance();

              tbuf_log_counters("after read", cl_state, chan);

              release_pdev_chan(msg);

              msg->done(*this);
              msg = nullptr;

              // Notify client if requested.
              if (cl_msg_tx.flags & Ivc_msg::Flags_ring)
                {
                  cl_state.doorbell_ccplex_pending
                    |= (1U << Tegra_hsp::Doorbell_master_bpmp);
                  if (!cl_state.doorbell_ccplex_masked)
                    doorbell_ccplex_ring_at_client(cl_state);
                }

              // Look for next client request.
              if (Client_msg *msg = _active_msgs_head)
                {
                  unsigned pdev_chan;
                  if (acquire_pdev_chan(msg, pdev_chan))
                    {
                      if constexpr (Tbuf_log)
                        {
                          --_nr_pending;
                          fiasco_tbuf_log_3val("NEXT PENDING",
                                               _nr_pending, 0, 0);
                        }
                      msg->remove_from_active(*this);
                      submit_cl_msg_to_pdev(msg, pdev_chan);
                    }
                  else
                    printf("\033[31mCANNOT WORK ON ACTIVE MESSAGE\033[m\n");
                }
            }
        }
    }

  _hsp->unmask_doorbell_ccplex_irq();
}

/**
 * Trigger a CCPLEX doorbell IRQ at the client.
 */
void
Tegra_bpmp::doorbell_ccplex_ring_at_client(Client_state &cl_state)
{
  auto *irq_client = &cl_state.irq_client_db;
  if (irq_client->is_valid())
    {
      if (long ret = l4_ipc_error((*irq_client)->trigger(), l4_utcb()))
        {
          printf("NOTIFY IRQ FAILED (ret=%ld)\n", ret);
          *irq_client = L4::Cap<L4::Irq>::Invalid;
        }
    }
  else
    printf("\033[31mTegra_bpmp::doorbell_handle(): "
           "no client bound to DB IRQ\033[m\n");
}

/**
 * Client reads from its associated virtual SRAM.
 */
l4_uint64_t
Tegra_bpmp::read_chan(Client_state const &cl_state, unsigned chan,
                      unsigned chan_offs, char log2_size)
{
  auto const &cl_msg = cl_state.cl_msg_tx(chan);
  l4_uint64_t value;
  switch (chan_offs)
    {
    case offsetof(Ivc_msg, snd_ep.count):
      value = cl_msg.snd_ep.count;
      break;
    case offsetof(Ivc_msg, snd_ep.state):
      value = cl_msg.snd_ep.state;
      break;
    case offsetof(Ivc_msg, rcv_ep.count):
      value = cl_msg.rcv_ep.count;
      break;
    default:
      {
        auto const *chan_ptr = cl_msg.raw_ptr(chan_offs);
        switch (log2_size)
          {
          case 0: value = *static_cast<l4_uint8_t const *>(chan_ptr); break;
          case 1: value = *static_cast<l4_uint16_t const *>(chan_ptr); break;
          case 2: value = *static_cast<l4_uint32_t const *>(chan_ptr); break;
          case 3: value = *static_cast<l4_uint64_t const *>(chan_ptr); break;
          default: value = 0; // cannot happen
          }
      }
    }
  trace5.printf("sram read %s chan=%x offs=%02x => %.*llx\n",
                chan >= First_rx_chan ? "RX" : "TX",
                chan, chan_offs, 2 << log2_size, value);
  if constexpr (Tbuf_log)
    fiasco_tbuf_log_3val(chan >= First_rx_chan ? "RX read" : "tx read",
                         chan % First_rx_chan, chan_offs, value);
  return value;
}

/**
 * Client writes to its associated virtual SRAM.
 */
void
Tegra_bpmp::write_chan(Client_state &cl_state, unsigned chan,
                       unsigned chan_offs, char log2_size, l4_uint64_t value)
{
  char str[64] = "";
  write_access_to_str(chan_offs, value & 0xffffffff, str, sizeof(str));
  trace5.printf("\033[31;1msram writ %s chan=%x offs=%02x %.*llx%s\033[m\n",
                chan >= First_rx_chan ? "RX" : "TX",
                chan, chan_offs, 2 << log2_size, value, str);
  if constexpr (Tbuf_log)
    fiasco_tbuf_log_3val(chan >= First_rx_chan ? "RX writ" : "tx writ",
                         chan % First_rx_chan, chan_offs, value);
  switch (chan_offs)
    {
    case offsetof(Ivc_msg, snd_ep.state):
      // never leave State_established
      break;
    default:
      {
        auto &cl_msg = cl_state.cl_msg_tx(chan);
        auto *chan_ptr = cl_msg.raw_ptr(chan_offs);
        switch (log2_size)
          {
          case 0: *static_cast<l4_uint8_t *>(chan_ptr) = value; break;
          case 1: *static_cast<l4_uint16_t *>(chan_ptr) = value; break;
          case 2: *static_cast<l4_uint32_t *>(chan_ptr) = value; break;
          case 3: *static_cast<l4_uint64_t *>(chan_ptr) = value; break;
          }
      }
    }
  if (chan < First_rx_chan)
    if (chan_offs + (1U << log2_size) > cl_state.tx_written[chan])
      cl_state.tx_written[chan] = chan_offs + (1U << log2_size);
}
