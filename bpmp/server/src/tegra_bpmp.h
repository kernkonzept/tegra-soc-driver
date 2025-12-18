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

#include <algorithm>
#include <cassert>
#include <functional>

#include <l4/drv/tegra/bpmp/bpmp_base.h>
#include <l4/re/util/object_registry>
#include <l4/sys/ktrace.h>
#include <l4/vbus/vbus>

#include "debug.h"

template <class HSP>
class Tegra_irqep : public L4::Irqep_t<Tegra_irqep<HSP>>
{
public:
  Tegra_irqep(HSP *hsp)
  : _hsp(hsp)
  {}

  void register_ep(L4Re::Util::Object_registry *registry,
                   unsigned irq_num, L4_irq_mode irq_mode)
  {
    _irq_num = irq_num;
    _irq = L4Re::chkcap(registry->register_irq_obj(this),
                        "hsp: Register IRQ server object");
    L4Re::chksys(_hsp->icu()->set_mode(irq_num, irq_mode), "hsp: Set IRQ mode");
    int ret = L4Re::chksys(_hsp->icu()->bind(irq_num, _irq),
                           "hsp: Bind interrupt to ICU");
    _irq_unmask_at_icu = ret == 1;

    info.printf("IRQ %x (%s-triggered, unmask at %s).\n",
                irq_num, irq_mode == L4_IRQ_F_LEVEL_HIGH ? "level-high" : "edge",
                ret == 1 ? "ICU" : "IRQ object");
  }

  void handle_irq()
  {
    if (_irq_num != _hsp->Num_irq_db)
      if (_log_irq_sm)
        {
          warn.printf("\033[31mhandle_irq IRQ %x\033[m\n", _irq_num);
          _log_irq_sm = false;
        }

    _hsp->handle_irq(_irq_num);
  }

  void unmask() const
  {
    if (_irq_unmask_at_icu)
      _hsp->icu()->unmask(_irq_num);
    else
      this->obj_cap()->unmask();
  }

private:
  // The IRQ object for the hardware interrupt. Would be required for unbinding.
  L4::Cap<L4::Irq> _irq;
  // The IRQ number of the IRQ at the ICU.
  int _irq_num;
  // True, if the IRQ has to be unmasked at the ICU.
  bool _irq_unmask_at_icu;

  HSP *_hsp;

  bool _log_irq_sm = true;
  Dbg info{Dbg::Info, "hsp-irqep"};
  Dbg warn{Dbg::Warn, "hsp-irqep"};
};

class Tegra_hsp : public Tegra_hsp_base<Tegra_hsp>
{
  using Handle_irq = std::function<void(unsigned)>;

public:
  // Also used by Tegra_hsp_base<>.
  static constexpr bool Tbuf_log = false;

  // number of CCPLEX doorbell IRQ
  static constexpr unsigned Num_irq_db = 32 + 0xb0;
  // number of first shared semaphore IRQ
  static constexpr unsigned Num_irq_sm = 32 + 0x78;
  // number of shared semaphores
  static constexpr unsigned Count_sm = 8;

  static constexpr unsigned Res_size_hsp = 0xa0000;
  static constexpr unsigned Res_size_sram = 0x80000;

  static constexpr unsigned Doorbell_master_ccplex = 17;
  static constexpr unsigned Doorbell_master_bpmp = 19;

  int init(L4Re::Util::Object_registry *registry, L4::Cap<L4vbus::Vbus> vbus,
           L4vbus::Device dev, l4vbus_device_t devinfo);

  /** IRQ endpoint handler. */
  void handle_irq(unsigned irq_num) const
  {
    if (_handle_irq)
      _handle_irq(irq_num);
  }

  void mmio_read(unsigned offs, l4_uint64_t &value, bool log) const
  {
    value = _regs.r<32>(offs);
    if (log)
      tbuf_log_read(offs, value);
  }

  void mmio_write(unsigned offs, l4_uint64_t value)
  {
    tbuf_log_write(offs, value);
    _regs.r<32>(offs) = value;
  }

  /**
   * Call handle_irq() at our hsp (Tegra_bpmp).
   * Called by main().
   * Adding a HSP template parameter to this class is more ugly.
   */
  void set_handle_irq(Handle_irq handle_irq)
  { _handle_irq = handle_irq; }

  /**
   * Unmask IRQ.
   * For unmasking CCPLEX doorbell IRQ, use unmask_doorbell_ccplex_irq()!
   */
  void unmask_irq(unsigned irq_num);

  /** Unmask CCPLEX doorbell IRQ. */
  void unmask_doorbell_ccplex_irq()
  { _irqep_db.unmask(); }

  // used by Tegra_irqep
  L4::Cap<L4::Icu> icu() const { return _icu; }

private:
  void init_mmio(L4::Cap<L4vbus::Vbus> vbus,
                 l4_addr_t mmio_addr, l4_size_t mmio_size);
  void init_irq(L4Re::Util::Object_registry *registry,
                char irq_id, unsigned irq_num, L4_irq_mode irq_mode);

  static void tbuf_log_read(unsigned offs, l4_uint64_t value)
  {
    if constexpr (Tbuf_log)
      fiasco_tbuf_log_3val("hsp read", offs, value, 0);
  }

  static void tbuf_log_write(unsigned offs, l4_uint64_t value)
  {
    if constexpr (Tbuf_log)
      fiasco_tbuf_log_3val("hsp writ" /*typo intended*/, offs, value, 0);
  }

  // Forward device IRQ to the hsp.
  Handle_irq _handle_irq{nullptr};

  // The ICU the hardware interrupts are triggered on.
  L4::Cap<L4::Icu> _icu{L4Re::chkcap(L4Re::Util::cap_alloc.alloc<L4::Icu>(),
                                     "hsp: Allocate ICU capability")};

  // initialized in Tegra_hsp::init_mmio()
  L4Re::Rm::Unique_region<l4_addr_t> _regs_region;
  Tegra_irqep<Tegra_hsp> _irqep_db{this};
  Tegra_irqep<Tegra_hsp> _irqep_sm[Count_sm]{ this, this, this, this,
                                              this, this, this, this };

  Dbg info{Dbg::Info, "hsp"};

public:
  // used by Tegra_hsp_base (Doorbell logging)
  Dbg log{Dbg::Trace4, "hsp"};
};

class Tegra_bpmp : public Tegra_bpmp_base<Tegra_bpmp, Tegra_hsp>
{
  static constexpr bool Tbuf_log = false;

  static constexpr unsigned Count_msgs = 32;
  using Handle_irq = std::function<void(unsigned)>;

public:
  /** Offs of SRAM TX/RX memory (TX: 4 KiB, RX: 4 KiB)) in the SRAM device. */
  static constexpr unsigned Offs_sram = 0x70000;
  /** Size of entire TX/RX memory. */
  static constexpr unsigned Size_sram = 0x2000;
  /** Number of IVC message channel pairs (TX + RX). */
  static constexpr unsigned Count_ivc_chan_pairs
    = Tegra_bpmp::Size_sram / (2 * Tegra_bpmp::Sizeof_ivc_msg);
  /** First RX channel. */
  static constexpr unsigned First_rx_chan = Count_ivc_chan_pairs;

  /**
   * Bundle of black list vector and white list vector.
   */
  template <typename TYPE>
  struct Id_vec_bundle
  {
    /**
     * Sorted vector for black lists / white lists.
     */
    struct Id_vec
    {
      bool find(TYPE id) const
      { return std::binary_search(vec.begin(), vec.end(), id); }

      bool is_sorted() const
      { return std::is_sorted(vec.begin(), vec.end()); }

      std::vector<TYPE> vec;
    };

    /**
     * Return \b true if either
     * - the ID is blacklisted, OR
     * - there all_white == false and the ID is NOT whitelisted.
     */
    bool disallowed(TYPE id) const
    { return !(all_white || white.find(id)) || black.find(id); }

    /** Verify that both lists are sorted. Required for binary searching. */
    bool is_sorted() const
    { return black.is_sorted() && white.is_sorted(); }

    bool all_white{false};
    Id_vec black;
    Id_vec white;
  };

  /**
   * Per-client "virtual" device state.
   */
  struct Client_state
  {
    Client_state(Id_vec_bundle<l4_uint16_t> clocks,
                 Id_vec_bundle<l4_uint8_t> powergates,
                 Id_vec_bundle<l4_uint8_t> resetlines)
    : clocks(clocks), powergates(powergates), resetlines(resetlines)
    {
      // vectors are created sorted in Client_factory::load_profile_table()
      assert(this->clocks.is_sorted());
      assert(this->powergates.is_sorted());
      assert(this->resetlines.is_sorted());

      sram_ds = L4Re::chkcap(L4Re::Util::make_unique_cap<L4Re::Dataspace>(),
                             "Allocate dataspace capability for sram");

      auto const *e = L4Re::Env::env();
      auto alloc_flags = L4Re::Mem_alloc::Continuous | L4Re::Mem_alloc::Pinned;
      L4Re::chksys(e->mem_alloc()->alloc(Size_sram, sram_ds.get(), alloc_flags),
                   "Allocate sram dataspace");
      auto rm_flags = L4Re::Rm::F::Search_addr | L4Re::Rm::F::Cache_normal
                    | L4Re::Rm::F::RW | L4Re::Rm::F::Eager_map;
      L4Re::chksys(e->rm()->attach(&msgs, Size_sram, rm_flags,
                                   L4::Ipc::make_cap_rw(sram_ds.get())),
                   "Attach sram dataspace");
      for (unsigned i = 0; i < 2 * Count_ivc_chan_pairs; ++i)
        switch (i % Count_ivc_chan_pairs)
          {
          case Chan_thread_0:
          case Chan_thread_1:
          case Chan_thread_2:
          case Chan_cpu_tx:
          case Chan_cpu_rx:
            msgs.get()[i].snd_ep.state = Ivc_msg::State_established;
            break;
          default:
            // prevent doorbell_bpmp_trigger() from checking unused channels
            msgs.get()[i].snd_ep.state = Ivc_msg::State_sync;
            break;
          }
    }

    Ivc_msg const &cl_msg_tx(unsigned chan) const
    { return *(msgs.get() + chan); }

    Ivc_msg &cl_msg_tx(unsigned chan)
    { return *(msgs.get() + chan); }

    Ivc_msg const &cl_msg_rx(unsigned chan) const
    { return *(msgs.get() + chan + First_rx_chan); }

    Ivc_msg &cl_msg_rx(unsigned chan)
    { return *(msgs.get() + chan + First_rx_chan); }

    /** Dataspace for TX+RX channels. */
    L4Re::Util::Unique_cap<L4Re::Dataspace> sram_ds;

    /** Attached region for TX+RX channels. */
    L4Re::Rm::Unique_region<Ivc_msg *> msgs;

    /** client IRQ sink for CCPLEX doorbell IRQ */
    L4::Cap<L4::Irq> irq_client_db;

    /** bitfield of per-channel pending CCPLEX doorbell IRQs */
    l4_uint32_t doorbell_ccplex_pending = 0;
    static_assert(Count_ivc_chan_pairs <= sizeof(doorbell_ccplex_pending) * 8);

    /** true if CCPLEX doorbell IRQ is masked */
    bool doorbell_ccplex_masked = true;

    /**
     * Per-TX channel: size of current TX message.
     * This value is used to determine the size of the current message. Without
     * this optimization we would always need to write sizeof(Ivc_msg::data).
    */
    unsigned tx_written[Count_ivc_chan_pairs]{};

    /**
     * List of black lists / white lists for clocks / power gates / reset lines.
     *
     * Possible optimization: If there are many entries, it might be better to
     * use bitmaps instead. For clocks, this would require a 512/8 = 64 bytes
     * per list (it seems that the maximum clock ID is < 512), for power gates
     * and reset lines 32 bytes per list.
     */
    const Id_vec_bundle<l4_uint16_t> clocks;
    const Id_vec_bundle<l4_uint8_t> powergates;
    const Id_vec_bundle<l4_uint8_t> resetlines;
  };

  /**
   * An active message submitted by a client and eventually handled by the
   * physical device.
   */
  struct Client_msg
  {
    template <typename BPMP>
    static Client_msg *
    new_msg(BPMP &bpmp, Client_state *cl_state, unsigned chan)
    {
      if (!bpmp._free_msgs_head)
        L4Re::throw_error(-L4_EINVAL, "No free client message context available");

      auto *msg = bpmp._free_msgs_head;
      msg->cl_state = cl_state;
      msg->chan = chan;

      // remove from free list
      bpmp._free_msgs_head = msg->next;

      return msg;
    }

    template <typename BPMP>
    void add_to_active(BPMP &bpmp)
    {
      // append to active list
      next = nullptr;
      if (!bpmp._active_msgs_head)
        bpmp._active_msgs_head = this;
      prev = bpmp._active_msgs_tail;
      if (bpmp._active_msgs_tail)
        bpmp._active_msgs_tail->next = this;
      bpmp._active_msgs_tail = this;
    }

    template <typename BPMP>
    void remove_from_active(BPMP &bpmp)
    {
      // remove from active list
      (next ? next->prev : bpmp._active_msgs_tail) = prev;
      (prev ? prev->next : bpmp._active_msgs_head) = next;
    }

    template <typename BPMP>
    void done(BPMP &bpmp)
    {
      // insert at the beginning of free list
      next = bpmp._free_msgs_head;
      bpmp._free_msgs_head = this;
      chan = ~0U;
      pdev_chan = ~0U;
    }

    /** pointer to previous message in active list; nullptr if first element */
    Client_msg *prev = nullptr;
    /** pointer to next message in free list or active list; nullptr if last */
    Client_msg *next = nullptr;
    /** pointer to client state */
    Client_state *cl_state = nullptr;
    /** channel used by client for this message */
    unsigned chan = ~0U;
    /** channel at physical device used for this message */
    unsigned pdev_chan = ~0U;
  };

  explicit Tegra_bpmp(L4Re::Util::Object_registry *registry, Tegra_hsp *hsp);
  ~Tegra_bpmp() {}

  /** Client notification: How to notify clients when an IRQ was triggered. */
  void set_handle_irq_at_clients(Handle_irq handle_irq_at_clients)
  { _handle_irq_at_clients = handle_irq_at_clients; }

  void mmio_read_hsp(Client_state &cl_state,
                     unsigned offs, char log2_size, l4_uint64_t &value)
  {
    if (log2_size != 2)
      L4Re::throw_error_fmt(-L4_EINVAL,
                            "hsp: Invalid %d-bit read access", 8 << log2_size);
    if (offs == _hsp_reg_doorbell_ccplex_pending)
      value = cl_state.doorbell_ccplex_pending;
    else
      _hsp->mmio_read(offs, value, true);
  }

  void mmio_write_hsp(Client_state &cl_state,
                      unsigned offs, char log2_size, l4_uint64_t value)
  {
    if (log2_size != 2)
      L4Re::throw_error_fmt(-L4_EINVAL,
                            "hsp: Invalid %d-bit write access", 8 << log2_size);
    if (offs == _hsp_reg_doorbell_ccplex_pending)
      cl_state.doorbell_ccplex_pending &= ~value;
    else if (offs == _hsp_reg_doorbell_bpmp_trigger)
      doorbell_bpmp_trigger(cl_state);
    else
      _hsp->mmio_write(offs, value);
  }

  void mmio_read_sram(Client_state &cl_state,
                      unsigned offs, char log2_size, l4_uint64_t &value)
  {
    if (offs >= Offs_sram && offs <= Offs_sram + Size_sram - (1U << log2_size))
      {
        unsigned offs_ds = offs - Offs_sram;
        unsigned chan = offs_ds / Tegra_bpmp::Sizeof_ivc_msg;
        unsigned chan_offs = offs_ds - chan * Tegra_bpmp::Sizeof_ivc_msg;
        value = read_chan(cl_state, chan, chan_offs, log2_size);
      }
    else
      {
        // guest does not read/write outside of TX/RX area
        L4Re::throw_error_fmt(
          -L4_EINVAL, "sram: read outside TX/RX area (%x)", offs);
      }

    mb();
  }

  void mmio_write_sram(Client_state &cl_state,
                       unsigned offs, char log2_size, l4_uint64_t value)
  {
    if (offs >= Offs_sram && offs <= Offs_sram + Size_sram - (1U << log2_size))
      {
        unsigned offs_ds = offs - Offs_sram;
        unsigned chan = offs_ds / Tegra_bpmp::Sizeof_ivc_msg;
        unsigned chan_offs = offs_ds - chan * Tegra_bpmp::Sizeof_ivc_msg;
        write_chan(cl_state, chan, chan_offs, log2_size, value);
      }
    else
      {
        // guest does not read/write outside of TX/RX area
        L4Re::throw_error_fmt(
          -L4_EINVAL, "sram: write outside TX/RX area (%x)", offs);
      }

    wmb();
  }

  void unmask_irq(Client_state &cl_state, unsigned irq_num)
  {
    if (irq_num == Tegra_hsp::Num_irq_db)
      doorbell_ccplex_unmask(cl_state);
    else
      _hsp->unmask_irq(irq_num);
  }

  void handle_irq(unsigned irq_num)
  {
    if (irq_num == Tegra_hsp::Num_irq_db)
      doorbell_ccplex_handle();
    else
      {
        if (_handle_irq_at_clients)
          _handle_irq_at_clients(irq_num);
        else
          warn.printf("handle_irq: client not (yet) attached\n");
      }
  }

private:
  void init_sram(L4::Cap<L4vbus::Vbus> vbus,
                   L4vbus::Device dev, l4vbus_device_t devinfo);

  l4_uint64_t read_chan(Client_state const &cl_state, unsigned chan,
                        unsigned offs, char log2_size);
  void write_chan(Client_state &cl_state, unsigned chan,
                  unsigned offs, char log2_size, l4_uint64_t value);

  void doorbell_bpmp_trigger(Client_state &cl_state);
  void doorbell_ccplex_handle();
  void doorbell_ccplex_unmask(Client_state &cl_state);
  void doorbell_ccplex_ring_at_client(Client_state &cl_state);

  /**
   * Acquire a free pair of channels (TX+RX) for a new message.
   * Will be one of Chan_cpu_tx, Chan_thread_{0..2}.
   * At the moment only Chan_cpu_tx.
   */
  bool acquire_pdev_chan(Client_msg *msg, unsigned &pdev_chan)
  {
    if (_chan_cpu_tx_active_msg)
      return false;

    _chan_cpu_tx_active_msg = msg; // XXX
    pdev_chan = Chan_cpu_tx; // XXX
    msg->pdev_chan = pdev_chan;
    return true;
  }

  void release_pdev_chan(Client_msg *msg)
  {
    if (msg->pdev_chan != Chan_cpu_tx)
      L4Re::throw_error_fmt(-L4_EINVAL,
                            "release_pdev_chan invalid %x", msg->pdev_chan);
    _chan_cpu_tx_active_msg = nullptr;
  }

  Client_msg *get_pdev_active_client_msg(unsigned pdev_chan) const
  {
    if (pdev_chan == Chan_cpu_tx)
      return _chan_cpu_tx_active_msg;
    else
      return nullptr;
  }

  bool filter_skip_cmd(Client_state &cl_state, unsigned chan);
  void submit_cl_msg_to_pdev(Client_msg *msg, unsigned pdev_chan);
  static void write_access_to_str(unsigned chan_offs, l4_uint32_t value,
                                  char *str, size_t strsize);
  void tbuf_log_counters(char const *str, Client_state const &cl_state,
                         unsigned chan);
  void log_skipped_msg(Ivc_msg const &cl_msg_tx) const;

  // Function to call when an IRQ was triggered and at least one client wants to
  // receive IRQs.
  Handle_irq _handle_irq_at_clients{nullptr};

  Dbg warn{Dbg::Warn, "bpmp"};
  Dbg info{Dbg::Info, "bpmp"};
  // raw MMIO register accesses
  Dbg trace5{Dbg::Trace5, "bpmp"};

  // XXX Direct access to sram!
  L4Re::Rm::Unique_region<l4_addr_t> _regs_region;
  L4drivers::Register_block<32> _regs;

  // XXX so far only for Chan_cpu_tx
  Client_msg *_chan_cpu_tx_active_msg{nullptr};

  // pool of pending messages from all clients
  Client_msg _msgs[Count_msgs];
  Client_msg *_free_msgs_head{nullptr};
  Client_msg *_active_msgs_head{nullptr};
  Client_msg *_active_msgs_tail{nullptr};

  // initialized in Tegra_bpmp::Tegra_bpmp()
  unsigned _hsp_reg_doorbell_ccplex_pending{0};
  unsigned _hsp_reg_doorbell_bpmp_trigger{0};

  // ONLY for Tbuf_log=true: number of currently pending client messages
  unsigned _nr_pending{0};

public:
  // Tegra_bpmp_base IVC channel logging: important write messages worth to decode
  Dbg chan_log1{Dbg::Trace1, "dev"};
  // Tegra_bpmp_base IVC channel logging: warnings (timeout etc)
  Dbg chan_log2{Dbg::Trace2, "dev"};
  // Tegra_bpmp_base IVC channel logging: less important write messages
  Dbg chan_log3{Dbg::Trace3, "dev"};
  // Tegra_bpmp_base IVC channel logging: all other read/write messages + state
  // + doorbell
  Dbg chan_log4{Dbg::Trace4, "dev"};
};
