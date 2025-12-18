/*
 * Copyright (C) 2026 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
/**
 * \file
 * Boot and Power Management Processor basic bits.
 *
 * Used by clients as well as by the server.
 */

#pragma once

#include <cstdio>  // snprintf() for tbuf logging
#include <cstring> // memset()

#include <l4/cxx/bitfield>
#include <l4/cxx/minmax>
#include <l4/cxx/unique_ptr>
#include <l4/cxx/utils>
#include <l4/re/util/debug>
#include <l4/re/util/unique_cap>
#include <l4/drv/tegra/base/base.h>
#include <l4/drv/tegra/bpmp/bpmp_if.h>

#include <l4/sys/ktrace.h>

/**
 * Hardware Synchronization Primitives.
 *
 * HSP register layout:
 *  - 64 KiB common registers
 *  - (nSM/2) * 64 KiB for shared mailboxes
 *  - (nSS) * 64 KiB for shared semaphores
 *  - (nAS) * 64 KiB for arbitrated semaphores
 *  - 64 KiB for doorbells
 *
 * With nSM=8, nSS=2, nAS=2, the doorbell registers start at 0x90000.
 *
 * BPMP does \b not use shared mailboxes but IVC (Inter VM Communication)
 * mailboxes. For synchronization, the HSP doorbell mechanism is used.
 */
template <class IMPL>
class Tegra_hsp_base
{
public:
  int init(l4_addr_t va)
  {
    Hsp_int_dimensioning id{_regs[Hsp_int_dimensioning::Offset]};
    auto const &impl = static_cast<IMPL &>(*this);
    impl.log.printf("Initialize: sm=%u, ss=%u, as=%u, db=%u, si=%u (%08x).\n",
                    id.n_sm().get(), id.n_ss().get(), id.n_as().get(),
                    id.n_db().get(), id.n_si().get(), id.raw);
    // NVIDIA Tegra Orin: sm=8, ss=2, as=2, db=10, si=8
    unsigned va_db = (1 + (id.n_sm() / 2) + id.n_ss() + id.n_as()) * 0x10000;
    va_doorbell_bpmp = va_db + Doorbell_bpmp_index * 0x100;
    va_doorbell_ccplex = va_db + Doorbell_ccplex_tz_not_sec * 0x100;

    _doorbell_bpmp.init(va + va_doorbell_bpmp);

    init_done = true;
    return L4_EOK;
  }

  /**
   * Trigger the doorbell telling the other side that relevant data has changed.
   *
   * \param reson  The reason for ringing the doorbell for logging purposes.
   */
  void trigger_bpmp_doorbell(char const *reason)
  {
    if (!init_done)
      {
        static_cast<IMPL &>(*this).log.printf("doorbell not initialized!\n");
        return;
      }

    _doorbell_bpmp.trigger(reason);
  }

private:
  /** Information about resources provided by this HSP instance. */
  struct Hsp_int_dimensioning
  {
    enum { Offset = 0x380 };
    l4_uint32_t raw;
    CXX_BITFIELD_MEMBER( 0,  3, n_sm, raw); ///< # of shared mboxes
    CXX_BITFIELD_MEMBER( 4,  7, n_ss, raw); ///< # of shared semaphores
    CXX_BITFIELD_MEMBER( 8, 11, n_as, raw); ///< # of arbitrated semaphores
    CXX_BITFIELD_MEMBER(12, 15, n_db, raw); ///< # of doorbells
    CXX_BITFIELD_MEMBER(16, 19, n_si, raw); ///< # of shared interrupts
  };

  enum
  {
    // See also Orin TRM Table 8.61: Mapping between Doorbell Number and Usage
    Doorbell_ccpmu = 0,
    Doorbell_ccplex_tz_not_sec = 1,
    Doorbell_ccplex_ts_sec = 2,
    Doorbell_bpmp_index = 3,
    Dorbell_bpmp_master = 19,
  };

  template <class HSP>
  class Doorbell
  {
  public:
    enum
    {
      // Write-only register: The written value doesn't matter.
      Trigger = 0x0,
      // Writing 1 to a bit clears the corresponding bit in HSP_DBELL_{db}_RAW
      // register and implicitly clears the same bit in this register.
      Pending = 0xc,
    };

    Doorbell(L4Re::Util::Dbg const &log)
    : _log(log)
    {}

    void init(l4_addr_t va)
    { _regs = new L4drivers::Mmio_register_block<32>(va); }

    void trigger(char const *reason)
    {
      _log.printf("doorbell: %s\n", reason);
      if constexpr (HSP::Tbuf_log)
        {
          char buf[48];
          snprintf(buf, sizeof(buf), "hsp DB %.40s", reason);
          fiasco_tbuf_log(buf);
        }
      _regs[Trigger] = 1;
    }

  private:
    L4drivers::Register_block<32> _regs;
    L4Re::Util::Dbg const &_log;
  };

public:
  /**
   * HSP MMIO offset of CPU Complex (TZ not-secure) doorbell pending register.
   * Value is needed for HSP device emulation.
   */
  constexpr unsigned Reg_doorbell_ccplex_pending() const
  { return va_doorbell_ccplex + Doorbell<IMPL>::Pending; }

  /**
   * HSP MMIO offset of BPMP doorbell trigger register.
   * Value is needed for HSP device emulation.
   */
  constexpr unsigned Reg_doorbell_bpmp_trigger() const
  { return va_doorbell_bpmp + Doorbell<IMPL>::Trigger; }

protected:
  L4drivers::Register_block<32> _regs;
  Doorbell<IMPL> _doorbell_bpmp{static_cast<IMPL &>(*this).log};
  l4_addr_t va_doorbell_bpmp;
  l4_addr_t va_doorbell_ccplex;
  bool init_done = false;
};

/** Boot and Power Management Processor -- Implementation. */
template <class IMPL, class HSP>
class Tegra_bpmp_base : public Tegra_bpmp_if
{
public:
  Tegra_bpmp_base(HSP *hsp)
  : _hsp(hsp)
  {}

  int init(l4_addr_t base_tx, l4_size_t size_tx,
           l4_addr_t base_rx, l4_size_t size_rx)
  {
    auto const &impl = static_cast<IMPL &>(*this);

    impl.chan_log2.printf("Initialize: TX %08lx-%08lx, RX %08lx-%08lx.\n",
                          base_tx, base_tx + size_tx, base_rx, base_rx + size_rx);
    _tx_sram = cxx::make_unique<Ivc_sram>(base_tx, size_tx);
    _rx_sram = cxx::make_unique<Ivc_sram>(base_rx, size_rx);

    // Used for transferring data to/from mailbox.
    _cpu_tx = cxx::make_unique<Ivc_channel>(
                _hsp, _tx_sram->msg + Chan_cpu_tx, _rx_sram->msg + Chan_cpu_tx,
                impl.chan_log1, impl.chan_log2, impl.chan_log3, impl.chan_log4);
    // Channel currently unused. Linux uses this in tegra_bpmp_handle_rx(). This
    // is used by the controller driver to push data received from remote to the
    // upper layer.
    _cpu_rx = cxx::make_unique<Ivc_channel>(
                _hsp, _tx_sram->msg + Chan_cpu_rx, _rx_sram->msg + Chan_cpu_rx,
                impl.chan_log1, impl.chan_log2, impl.chan_log3, impl.chan_log4);

    _cpu_tx->reset();
    _cpu_rx->reset();

    return L4_EOK;
  }

  // XXX always using Chan_cpu_tx
  template <typename MRQ>
  l4_uint32_t chan_cpu_tx_transfer(MRQ &mrq)
  {
    static_assert(sizeof(mrq.req) == 1 || sizeof(mrq.req) % 4 == 0);
    static_assert(sizeof(mrq.rsp) == 1 || sizeof(mrq.rsp) % 4 == 0);
    return _cpu_tx->transfer(mrq.Mrq, &mrq.req, sizeof(mrq.req),
                             &mrq.rsp, sizeof(mrq.rsp));
  }

  void chan_cpu_tx_write_and_ring(l4_uint32_t mrq, void const *req,
                                  size_t req_size)
  { _cpu_tx->write_and_ring(mrq, req, req_size); }

  void chan_cpu_tx_read(l4_uint32_t mrq, l4_uint32_t cmd,
                        l4_uint32_t *code, void *data, size_t size)
  { _cpu_tx->read(mrq, cmd, code, data, size, _hsp); }

  void chan_cpu_rx_read(l4_uint32_t mrq, l4_uint32_t cmd,
                        l4_uint32_t *code, void *data, size_t size)
  { _cpu_rx->read(mrq, cmd, code, data, size, _hsp); }

  l4_uint32_t tx_chan_rcv_ep_count() const
  { return cxx::access_once(&_cpu_tx->tx_msg().rcv_ep.count); }

  // XXX temporary
  l4_uint32_t tx_snd_count() const
  { return cxx::access_once(&_cpu_tx->tx_msg().snd_ep.count); }
  l4_uint32_t tx_snd_state() const
  { return cxx::access_once(&_cpu_tx->tx_msg().snd_ep.state); }
  l4_uint32_t tx_rcv_count() const
  { return cxx::access_once(&_cpu_tx->tx_msg().rcv_ep.count); }
  l4_uint32_t rx_snd_count() const
  { return cxx::access_once(&_cpu_tx->rx_msg().snd_ep.count); }
  l4_uint32_t rx_snd_state() const
  { return cxx::access_once(&_cpu_tx->rx_msg().snd_ep.state); }
  l4_uint32_t rx_rcv_count() const
  { return cxx::access_once(&_cpu_tx->rx_msg().rcv_ep.count); }

  l4_uint32_t chan_cpu_tx_rx_snd_ep_count() const
  { return cxx::access_once(&_cpu_tx->rx_msg().snd_ep.count); }
  l4_uint32_t chan_cpu_rx_rx_snd_ep_count() const
  { return cxx::access_once(&_cpu_rx->rx_msg().snd_ep.count); }

  l4_size_t chan_size_data() const
  { return _cpu_tx->size_data(); }

  // logging
  void log_tx_rx_state(char const *when) const
  { _cpu_tx->log_tx_rx_state(when); }

  //////////////////////////////////////////////////////////////////////////////
  int req_ping(l4_uint32_t ping_value)
  {
    Mrq_ping ping{ping_value};
    int ret = chan_cpu_tx_transfer(ping);
    if (ping.rsp.reply != l4_uint32_t{ping.req.challenge << 1})
      {
        static_cast<IMPL &>(*this).chan_log1
          .printf("ping failed (error=%d, reply=%08x).\n", ret, ping.rsp.reply);
        return -L4_EIO;
      }
    return L4_EOK;
  }

  //////////////////////////////////////////////////////////////////////////////
  int req_threaded_ping(l4_uint32_t ping_value)
  {
    Mrq_threaded_ping ping{ping_value};
    int ret = chan_cpu_tx_transfer(ping);
    if (ping.rsp.reply != l4_uint32_t{ping.req.challenge << 1})
      {
        static_cast<IMPL &>(*this).chan_log1
          .printf("threaded ping failed (error=%d, reply=%08x).\n",
                  ret, ping.rsp.reply);
        return -L4_EIO;
      }
    return L4_EOK;
  }

  //////////////////////////////////////////////////////////////////////////////
  bool req_supported(l4_uint32_t req_id)
  {
    Mrq_query_abi query_abi{req_id};
    int ret = chan_cpu_tx_transfer(query_abi);
    return ret >= 0 && query_abi.rsp.status == 0;
  }

  //////////////////////////////////////////////////////////////////////////////
  int req_query_fw_tag()
  {
    Mrq_query_fw_tag query_fw_tag;
    int ret = chan_cpu_tx_transfer(query_fw_tag);
    static_cast<IMPL &>(*this).chan_log1
      .printf("Mrq_query_fw_tag ret = %d, tag = %.*s\n",
              ret, int{sizeof(query_fw_tag.rsp.tag)}, query_fw_tag.rsp.tag);
    return ret >= 0 ? L4_EOK : -L4_EIO;
  }

  //////////////////////////////////////////////////////////////////////////////
  int req_clk_get_max()
  {
    Mrq_clk get_max{Mrq_clk::Req::Get_max_id::Cmd};
    int ret = chan_cpu_tx_transfer(get_max);
    return ret >= 0 ? get_max.rsp.get_max_id.max_id : -L4_EIO;
  }

  //////////////////////////////////////////////////////////////////////////////
  int req_clk_get_all_info(l4_uint32_t id,
                           Tegra_bpmp_if::Mrq_clk::Rsp::Get_all_info &info)
  {
     Mrq_clk get_all_info{Mrq_clk::Req::Get_all_info::Cmd,
                          static_cast<Mrq_clk::Clk_id>(id)};
     memset(&info, 0, sizeof(info));
     if (int ret = chan_cpu_tx_transfer(get_all_info); ret < 0)
       return -L4_EIO;

     info = get_all_info.rsp.get_all_info;
     return L4_EOK;
  }

  //////////////////////////////////////////////////////////////////////////////
  int req_clk_enable(Mrq_clk::Clk_id id)
  {
    Mrq_clk enable{Mrq_clk::Req::Enable::Cmd, id};
    int ret = chan_cpu_tx_transfer(enable);
    return ret >= 0 ? L4_EOK : -L4_EIO;
  }

  //////////////////////////////////////////////////////////////////////////////
  int req_clk_set_rate(Mrq_clk::Clk_id id, l4_uint32_t rate)
  {
    Mrq_clk set_rate{Mrq_clk::Req::Set_rate::Cmd, id};
    set_rate.req.set_rate.rate = rate;
    int ret = chan_cpu_tx_transfer(set_rate);
    return ret >= 0 ? L4_EOK : -L4_EIO;
  }

  //////////////////////////////////////////////////////////////////////////////
  int req_clk_round_rate(Mrq_clk::Clk_id id, l4_uint32_t &rate)
  {
    Mrq_clk round_rate{Mrq_clk::Req::Round_rate::Cmd, id};
    round_rate.req.round_rate.rate = rate;
    if (int ret = chan_cpu_tx_transfer(round_rate); ret < 0)
      return -L4_EIO;

    rate = round_rate.rsp.round_rate.rate;
    return L4_EOK;
  }

  //////////////////////////////////////////////////////////////////////////////
  int req_pg_get_max()
  {
    Mrq_pg get_max{Mrq_pg::Req::Get_max_id::Cmd,
                   Mrq_pg::Tegra234_power_domain_dummy};
    if (int ret = chan_cpu_tx_transfer(get_max); ret < 0)
      return -L4_EIO;

    return get_max.rsp.get_max_id.max_id;
  }

  //////////////////////////////////////////////////////////////////////////////
  int req_pg_get_name(l4_uint32_t id,
                      Tegra_bpmp_if::Mrq_pg::Rsp::Get_name &info)
  {
     Mrq_pg get_name{Mrq_pg::Req::Get_name::Cmd, id};
     if (int ret = chan_cpu_tx_transfer(get_name); ret < 0)
       return -L4_EIO;

     info = get_name.rsp.get_name;
     return L4_EOK;
  }

  //////////////////////////////////////////////////////////////////////////////
  int req_pg_set_state(l4_uint32_t id, l4_uint32_t state)
  {
    Mrq_pg set_state{Mrq_pg::Req::Set_state::Cmd, id};
    set_state.req.set_state.state = state;
    if (int ret = chan_cpu_tx_transfer(set_state); ret < 0)
      return -L4_EIO;

    return L4_EOK;
  }

  //////////////////////////////////////////////////////////////////////////////
  int req_reset_deassert(Mrq_reset::Reset_id id)
  {
    Mrq_reset deassert{Mrq_reset::Reset_cmd::Deassert, id};
    if (int ret = chan_cpu_tx_transfer(deassert); ret < 0)
      return -L4_EIO;

    return L4_EOK;
  }

  //////////////////////////////////////////////////////////////////////////////
  int req_uphy_set_pcie_controller_state(l4_uint16_t lane,
                                         l4_uint8_t pcie_id, l4_uint8_t enable)
  {
    Mrq_uphy set_state{{lane, Mrq_uphy::Req::Cmd_uphy_pcie_controller_state,
                       {{ pcie_id, enable }}},{}};
    if (int ret = chan_cpu_tx_transfer(set_state); ret < 0)
      return -L4_EIO;

    return L4_EOK;
  }

protected:
  /**
   * Channels: 3 for threaded for parallel handling of requests.
   *
   * Chan_cpu_rx is meant as an universal "back channel" from firmware to the
   * CPU an can be used to push data "out-of-order". This channel is not used
   * during normal Linux operation (always count=0).
   */
  enum
  {
    Chan_thread_0 = 0,  // reset works, ping works
    Chan_thread_1 = 1,  // reset works, ping works
    Chan_thread_2 = 2,  // reset works, ping works
    Chan_cpu_tx = 3,    // reset works, ping works
    Chan_cpu_rx = 13,   // reset works, ping no response
                        // all other channels: reset times out
  };

  /**
   * Single IVC message.
   * 64-byte alignment to avoid cache line bouncing.
   */
  struct alignas(256) Ivc_msg
  {
    /**
     * Channel state.
     * Usually the state remains Established.
     */
    enum State : l4_uint32_t
    {
      /**
       * Normal, ready-to-communicate state.
       */
      State_established = 0,
      /**
       * An endpoint signals that it is ready for the other side to clear
       * counters, indicating a pause in direct communication.
       */
      State_sync = 1,
      /**
       * After observing State_sync and clearing its own counters, the
       * transmitting endpoint enters State_ack, confirming readiness to receive
       * from the remote endpoint.
       */
      State_ack = 2,
    };

    /**
     * Request/response flags.
     */
    enum Flags : l4_uint32_t
    {
      /// Request an answer from the peer.
      Flags_ack = 1 << 0,
      /**
       * Ring the sender's doorbell when responding.
       * If not set, the sender needs to poll for a response.
       */
      Flags_ring = 1 << 1,
      /**
       * CRC + payload present. If set, Flags has the following fields:
       * - bits 16..31: crc16
       * - bits  8..15: payload length
       * - bits  4.. 7: xix (transaction ID)
       * - bits  0.. 3: options, see Flags.
       */
      Flags_crc_present = 1 << 2,
    };

    /**
     * Fields owned by the transmitting endpoint.
     * For messages on TX channel, this is us (the CPU).
     * For messages on RX channel, this is the other side.
     */
    struct alignas(64)
    {
      /** \b 0x00: Current request number. */
      l4_uint32_t count;
      /** \b 0x04: Request state. See State. */
      State       state;
    } snd_ep;

    /**
     * Fields owned by the receiving endpoint.
     * For messages on TX channel, this is the other side.
     * For messages on RX channel, this is us (the CPU).
     */

    struct alignas(64)
    {
      /** \b 0x40: Current request number. */
      l4_uint32_t count;
    } rcv_ep;

    struct alignas(64)
    {
      /**
       * \b 0x80: For TX, MRQ (request code),
       *          for RX, request response code (0: success, < 0: error).
       */
      l4_uint32_t code;
      /** \b 0x84: See Flags. */
      Flags       flags;
      /** \b 0x88: Request / response data. */
      l4_uint32_t data[120 / sizeof(l4_uint32_t)];
    };

    bool established()
    { return cxx::access_once(&snd_ep.state) == State_established; }

    /// Read from message box and return the return code and the response data.
    void mb_read(l4_uint32_t *ret, void *data, size_t size)
    {
      rmb();
      if (data && size > 1)
        {
          auto const *src = this->data;
          auto *dst = static_cast<l4_uint32_t *>(data);
          for (unsigned i = 0; i < (size + 3) / 4; ++i, ++src, ++dst)
            *dst = cxx::access_once(src);
        }
      *ret = cxx::access_once(&this->code);
    }

    /// Advance the request number of receiving endpoint.
    void mb_read_advance()
    {
      cxx::write_now(&rcv_ep.count, cxx::access_once(&rcv_ep.count) + 1);
      mb();
    }

    void mb_write(l4_uint32_t mrq, Flags flags, const void *data, size_t size)
    {
      cxx::write_now(&this->code, mrq);
      cxx::write_now(&this->flags, flags);
      if (data)
        {
          auto *src = static_cast<l4_uint32_t const *>(data);
          auto *dst = this->data;
          for (unsigned i = 0; i < (size + 3) / 4; ++i, ++src, ++dst)
            cxx::write_now(dst, *src);
        }
      wmb();
    }

    /// Advance the request number of the transmitting endpoint.
    void mb_write_advance()
    {
      cxx::write_now(&snd_ep.count, cxx::access_once(&snd_ep.count) + 1);
      mb();
    }

    /**
     * Raw access to this message.
     *
     * \attention Do only use for messages on virtual memory! For accessing the
     *            hardware device, use mb_read()/mb_write()!
     */
    constexpr void *raw_ptr(unsigned offset)
    {
      return static_cast<void *>(reinterpret_cast<l4_uint8_t *>(this) + offset);
    }

    /**
     * Raw access to this message.
     *
     * \attention Do only use for messages on virtual memory! For accessing the
     *            hardware device, use mb_read()/mb_write()!
     */
    constexpr void const *raw_ptr(unsigned offset) const
    {
      return static_cast<void const *>(
               reinterpret_cast<l4_uint8_t const *>(this) + offset);
    }
  };
  static_assert(sizeof(Ivc_msg) == 256);

public:
  static constexpr unsigned Sizeof_ivc_msg = sizeof(Ivc_msg);

private:
  /**
   * IVC (Inter VM communication) SRAM for RX and TX (one memory area for each
   * direction).
   */
  struct Ivc_sram
  {
    Ivc_sram(l4_addr_t virt, l4_size_t size)
    {
      msg = reinterpret_cast<Ivc_msg *>(virt);
      num_msg = size / sizeof(Ivc_msg);
    }

    Ivc_msg *msg;
    unsigned num_msg;
  };

  /**
   * A channel has an Ivc_msg for TX into TX SRAM and a corresponding Ivc_msg
   * for RX into RX SRAM.
   */
  class Ivc_channel
  {
  public:
    Ivc_channel(HSP *hsp, Ivc_msg *tx_msg, Ivc_msg *rx_msg,
                L4Re::Util::Dbg log1, L4Re::Util::Dbg log2,
                L4Re::Util::Dbg log3, L4Re::Util::Dbg log4)
    : _hsp(hsp), _tx_msg(tx_msg), _rx_msg(rx_msg),
      _log1(log1), _log2(log2), _log3(log3), _log4(log4)
    {}

    /** Reset channel during initialization. */
    bool reset()
    {
      auto *kip = l4re_kip();
      auto wait_until = l4_kip_clock(kip) + 100'000;
      _tx_msg->snd_ep.state = Ivc_msg::State_sync;
      _hsp->trigger_bpmp_doorbell("reset: state=sync");

      do
        {
          auto state = cxx::access_once(&_rx_msg->snd_ep.state);
          if (state == Ivc_msg::State_sync)
            {
              // the other side signals us that we may reset our counters
              rmb();
              _tx_msg->snd_ep.count = 0;
              _rx_msg->rcv_ep.count = 0;
              wmb();
              _tx_msg->snd_ep.state = Ivc_msg::State_ack;
              _hsp->trigger_bpmp_doorbell("reset: state=sync~>ack");
            }
          else if (_tx_msg->snd_ep.state == Ivc_msg::State_sync
                   && state == Ivc_msg::State_ack)
            {
              rmb();
              _tx_msg->snd_ep.count = 0;
              _rx_msg->rcv_ep.count = 0;
              wmb();
              _tx_msg->snd_ep.state = Ivc_msg::State_established;
              _hsp->trigger_bpmp_doorbell("reset: state=sync~>established");
            }
          else if (_tx_msg->snd_ep.state == Ivc_msg::State_ack)
            {
              rmb();
              _tx_msg->snd_ep.state = Ivc_msg::State_established;
              _hsp->trigger_bpmp_doorbell("reset: state=ack~>established");
            }
          if (l4_kip_clock(kip) >= wait_until)
            {
              _log2.printf("RESET timeout\n");
              return false;
            }
        }
      while (!_tx_msg->established());
      return true;
    }

    /**
     * Perform a full mailbox transfer including writing, triggering the door
     * bell and reading.
     * Do NOT request ringing the doorbell. Use during setup phase.
     */
    l4_uint32_t transfer(l4_uint32_t mrq, void const *req, size_t req_size,
                         void *rsp, size_t rsp_size)
    {
      log_tx_rx_state("before write");
      l4_uint32_t cmd = static_cast<l4_uint32_t const *>(req)[0]; // logging
      write(mrq, Ivc_msg::Flags_ack, req, req_size, _hsp);
      log_tx_rx_state(" after write");

      log_tx_rx_state(" before read");
      l4_uint32_t code = ~0U;
      read(mrq, cmd, &code, rsp, rsp_size, _hsp);
      log_tx_rx_state("  after read");

      return code;
    }

    void write_and_ring(l4_uint32_t mrq, void const *req, size_t req_size)
    {
      log_tx_rx_state("before write");
      auto flags = typename Ivc_msg::Flags{Ivc_msg::Flags_ack | Ivc_msg::Flags_ring};
      write(mrq, flags, req, req_size, _hsp);
      log_tx_rx_state(" after write");
    }

    bool read(l4_uint32_t mrq, l4_uint32_t cmd,
              l4_uint32_t *code, void *data, size_t size, HSP *hsp)
    {
      if (!check_read_ok())
        return false;

      _rx_msg->mb_read(code, data, size);

      if (!check_read_ok())
        return false;

      _rx_msg->mb_read_advance();
      hsp->trigger_bpmp_doorbell("rcv_ep.count increased (read)");

      log_read(mrq, cmd, static_cast<l4_int32_t>(*code), data, size);

      return true;
    }

    void log_tx_rx_state(char const *when)
    {
      _log4.printf("state %s: tx=%d/%d/%d, rx=%d/%d/%d\n",
        when,
        _tx_msg->snd_ep.count, _tx_msg->snd_ep.state, _tx_msg->rcv_ep.count,
        _rx_msg->snd_ep.count, _rx_msg->snd_ep.state, _rx_msg->rcv_ep.count);
    }

    Ivc_msg const &tx_msg() const { return *_tx_msg; }
    Ivc_msg const &rx_msg() const { return *_rx_msg; }

    static constexpr size_t size_data()
    { return sizeof(Ivc_msg::data); }

  private:
    bool write(l4_uint32_t mrq, typename Ivc_msg::Flags flags,
               const void *data, size_t size, HSP *hsp)
    {
      log_write("write: ", mrq, flags, data, size);

      if (!check_write_ok())
        return false;

      _tx_msg->mb_write(mrq, flags, data, size);

      if (!check_write_ok())
        return false;

      _tx_msg->mb_write_advance();
      hsp->trigger_bpmp_doorbell("snd_ep.count increased (write)");
      return true;
    }

    /** Helper for logging. */
    static void to_hex(char *str_data, void const *data, size_t size)
    {
      if (data)
        for (unsigned i = 0; i < size; ++i)
          {
            l4_uint8_t b = static_cast<l4_uint8_t const *>(data)[i];
            static constexpr char hex[] = "0123456789abcdef";
            str_data[3 * i + 0] = ' ';
            str_data[3 * i + 1] = hex[b >> 4];
            str_data[3 * i + 2] = hex[b & 0xf];
            str_data[3 * i + 3] = '\0';
          }
    }

    void log_write(char const *prefix, l4_uint32_t mrq,
                   typename Ivc_msg::Flags flags,
                   const void *data, size_t size) const
    {
      if (_log1.is_active())
        {
          switch (mrq)
            {
            case Mrq_clk::Mrq:
              {
                auto &req = *static_cast<Mrq_clk::Req const *>(data);
                auto const &log = req.is_important() ? _log1 : _log3;
                req.log(log, prefix);
                return;
              }
            case Mrq_reset::Mrq:
              {
                auto &req = *static_cast<Mrq_reset::Req const *>(data);
                req.log(_log3, prefix);
                return;
              }
            case Mrq_pg::Mrq:
              {
                auto &req = *static_cast<Mrq_pg::Req const *>(data);
                auto const &log = req.is_important() ? _log1 : _log3;
                req.log(log, prefix);
                return;
              }
            case Mrq_thermal::Mrq:
              {
                auto &req = *static_cast<Mrq_thermal::Req const *>(data);
                req.log(_log3, prefix);
                return;
              }
            case Mrq_debug::Mrq:
              {
                auto &req = *static_cast<Mrq_debug::Req const *>(data);
                req.log(_log4, prefix);
                return;
              }
            }
        }

      if (_log4.is_active())
        {
          enum { Num_bytes = 12 };
          char str_data[Num_bytes * 3 + 1] = "";
          to_hex(str_data, data, cxx::min<size_t>(size, Num_bytes));
          _log4.printf("%smrq=%d (%s), flags=%02x, data%s (%zd)\n",
                       prefix, mrq, Tegra_bpmp_if::req_to_str(mrq),
                       flags, str_data, size);
        }
    }

    void log_read(l4_uint32_t mrq, l4_uint32_t cmd,
                  l4_int32_t code, void *data, size_t size) const
    {
      char const *code_str = code < 0 ? "\033[31;47mread: " : "read: ";
      if (_log3.is_active())
        {
          switch (mrq)
            {
            case Mrq_clk::Mrq:
              {
                auto &rsp = *static_cast<Mrq_clk::Rsp const *>(data);
                if (rsp.log(_log3, cmd, code, code_str))
                  return;
                break;
              }
            case Mrq_thermal::Mrq:
              {
                auto &rsp = *static_cast<Mrq_thermal::Rsp const *>(data);
                if (rsp.log(_log3, cmd, code, code_str))
                  return;
                break;
              }
            }
        }

      if (_log4.is_active())
        {
          enum { Num_bytes = 12 };
          char str_data[Num_bytes * 3 + 1] = "";
          to_hex(str_data, data, cxx::min<size_t>(size, Num_bytes));
          _log4.printf("%scode=%d, data%s (%zd)\n",
                       code_str, code, str_data, size);
        }
    }

    /**
     * Wait for some time until we are allowed to write.
     */
    bool wait_for_write_empty()
    {
      auto const *kip = l4re_kip();
      auto now = l4_kip_clock(kip);
      auto wait_until = now + 100'000; // XXX
      while (_tx_msg->snd_ep.count != _tx_msg->rcv_ep.count)
        {
          if (l4_kip_clock(kip) >= wait_until)
            {
              _log2.printf("WRITE TIMEOUT\n");
              return false;
            }
          l4_ipc_sleep_ms(1);
        }

      if constexpr (HSP::Tbuf_log)
        if (l4_kip_clock(kip) >= now + 1'000) // XXX
          fiasco_tbuf_log_3val("LONG WRITE WAIT", l4_kip_clock(kip) - now, 0, 0);

      return true;
    }

    /**
     * Wait for some time until we are allowed to read.
     *
     * Certain firmware calls take a longer time for completion. This should not
     * matter because the firmware usually sends 2 doorbell IRQs: The first one
     * after the request was received, the second one when the response was
     * send. So if we really experience a longer wait time, this is either
     * because we poll (during bootstrap setup) or because of some mis-behavior
     * of the firmware.
     */
    bool wait_for_read_not_empty()
    {
      auto const *kip = l4re_kip();
      auto now = l4_kip_clock(kip);
      auto wait_until = now + 100'000; // XXX
      while (_rx_msg->snd_ep.count == _rx_msg->rcv_ep.count)
        {
          if (l4_kip_clock(kip) >= wait_until)
            {
              _log2.printf("READ TIMEOUT\n");
              return false;
            }
          l4_ipc_sleep_ms(1);
        }

      if constexpr (HSP::Tbuf_log)
        if (l4_kip_clock(kip) >= now + 1'000) // XXX
          fiasco_tbuf_log_3val("LONG READ WAIT", l4_kip_clock(kip) - now, 0, 0);
      return true;
    }

    bool check_write_ok()
    {
      if (!_tx_msg->established())
        return false;

      return wait_for_write_empty();
    }

    /** Wait until reading from mailbox is possible. */
    bool check_read_ok()
    {
      if (!_rx_msg->established())
        return false;

      return wait_for_read_not_empty();
    }

    HSP *_hsp;
    Ivc_msg *_tx_msg;
    Ivc_msg *_rx_msg;
    L4Re::Util::Dbg _log1;
    L4Re::Util::Dbg _log2;
    L4Re::Util::Dbg _log3;
    L4Re::Util::Dbg _log4;
  };

public:
  static void mb()
  {
    // XXX Jan: use dsb?
#if defined(__ARM_ARCH) && __ARM_ARCH == 7
    asm volatile ("dmb ish" : : : "memory");
#elif defined(__ARM_ARCH) && __ARM_ARCH >= 8
    asm volatile ("dmb ish" : : : "memory");
#elif defined(__mips__)
    asm volatile ("sync" : : : "memory");
#elif defined(__amd64__) || defined(__i386__) || defined(__i686__)
    asm volatile ("mfence" : : : "memory");
#elif defined(__riscv)
    asm volatile ("fence ow, ow" : : : "memory");
#else
#warning Missing proper memory barrier
    asm volatile ("" : : : "memory");
#endif
  }

  static void wmb()
  {
    // XXX Jan: use dsb?
#if defined(__ARM_ARCH) && __ARM_ARCH == 7
    asm volatile ("dmb ishst" : : : "memory");
#elif defined(__ARM_ARCH) && __ARM_ARCH >= 8
    asm volatile ("dmb ishst" : : : "memory");
#elif defined(__mips__)
    asm volatile ("sync" : : : "memory");
#elif defined(__amd64__) || defined(__i386__) || defined(__i686__)
    asm volatile ("sfence" : : : "memory");
#elif defined(__riscv)
    asm volatile ("fence ow, ow" : : : "memory");
#else
#warning Missing proper memory write barrier
    asm volatile ("" : : : "memory");
#endif
  }

  static void rmb()
  {
    // XXX Jan: use dsb?
#if defined(__ARM_ARCH) && __ARM_ARCH == 7
    asm volatile ("dmb ish"   : : : "memory");
#elif defined(__ARM_ARCH) && __ARM_ARCH >= 8
    asm volatile ("dmb ishld" : : : "memory");
#elif defined(__mips__)
    asm volatile ("sync" : : : "memory");
#elif defined(__amd64__) || defined(__i386__) || defined(__i686__)
    asm volatile ("lfence" : : : "memory");
#elif defined(__riscv)
    asm volatile ("fence ir, ir" : : : "memory");
#else
#warning Missing proper memory read barrier
    asm volatile ("" : : : "memory");
#endif
  }

private:
  cxx::unique_ptr<Ivc_sram> _tx_sram;
  cxx::unique_ptr<Ivc_sram> _rx_sram;
  cxx::unique_ptr<Ivc_channel> _cpu_tx;
  cxx::unique_ptr<Ivc_channel> _cpu_rx;

protected:
  HSP *_hsp;
};
