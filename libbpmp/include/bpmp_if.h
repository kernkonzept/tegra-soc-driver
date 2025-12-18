// vim:set ft=cpp: -*- Mode: C++ -*-
/*
 * Copyright (C) 2026 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
/**
 * \file
 * Boot and Power Management Processor requests.
 *
 * Used by clients as well as by the server.
 */

#pragma once

#include <l4/sys/l4int.h>
#include <l4/re/util/debug>

/** Boot and Power Management Processor -- Interface. */
struct Tegra_bpmp_if
{
  static char const *req_to_str(unsigned i);

  //////////////////////////////////////////////////////////////////////////////
  /** A simple ping (Mrq = 0). */
  struct Mrq_ping
  {
    enum { Mrq = 0 };
    Mrq_ping(l4_uint32_t challenge)
    : req{challenge}
    {}

    struct Req
    {
      l4_uint32_t challenge;      ///< word passed to firmware
    } req;

    struct Rsp
    {
      l4_uint32_t reply;          ///< passed word left-shifted by 1
    } rsp;
  };

  //////////////////////////////////////////////////////////////////////////////
  /**
   * A simple ping with BPMP responding from a thread context (Mrq = 9).
   * Seems to behave exactly like Mrq_ping.
   */
  struct Mrq_threaded_ping
  {
    enum { Mrq = 9 };
    Mrq_threaded_ping(l4_uint32_t challenge)
    : req{challenge}
    {}

    struct Req
    {
      l4_uint32_t challenge;      ///< word passed to firmware
    } req;

    struct Rsp
    {
      l4_uint32_t reply;          ///< passed word left-shifted by 1
    } rsp;
  };

  //////////////////////////////////////////////////////////////////////////////
  /** Reset (Mrq = 20). */
  struct Mrq_reset
  {
    enum { Mrq = 20 };
    enum class Reset_id
    {
      Tegra234_reset_pex2_core_8 = 25, // pcie@resets/core
      Tegra234_reset_pex2_core_8_apb = 26, // pcie@resets/apb
      Tegra234_reset_pwm3 = 70,
    };
    enum class Reset_cmd
    {
      Assert = 1,
      Deassert = 2,
      Reset_module = 3,
      Reset_get_max_id = 4,
    };
    Mrq_reset(Reset_cmd cmd, Reset_id reset_id)
    : req(cmd, reset_id)
    {}

    struct Req
    {
      Req(Reset_cmd cmd, Reset_id reset_id)
      : cmd(static_cast<l4_uint32_t>(cmd)),
        reset_id(static_cast<l4_uint32_t>(reset_id))
      {}
      l4_uint32_t cmd;
      l4_uint32_t reset_id;

      void log(L4Re::Util::Dbg const &log, char const *prefix = "") const
      {
        log.printf("%sreset, cmd=%s, id=0x%x (%s)\n",
                   prefix, fn_to_str(), reset_id, id_to_str());
      }

      /** Do we need to filter this request against black list / white list? */
      constexpr bool filter_required() const
      {
        switch (cmd)
          {
          case static_cast<l4_uint32_t>(Reset_cmd::Assert):
          case static_cast<l4_uint32_t>(Reset_cmd::Deassert):
            return true;
          default:
            return false;
          }
      }

      char const *fn_to_str() const { return fn_to_str(cmd); }
      char const *id_to_str() const { return id_to_str(reset_id); }
      static char const *fn_to_str(unsigned fn);
      static char const *id_to_str(unsigned reset_id);
    } req;

    struct Rsp
    {
    } rsp;
  };

  //////////////////////////////////////////////////////////////////////////////
  struct Mrq_i2c
  {
    enum { Mrq = 21 };
  };

  //////////////////////////////////////////////////////////////////////////////
  /** Clock (Mrq = 22). */
  struct Mrq_clk
  {
    enum { Mrq = 22 };
    enum class Clk_id
    {
      Tegra234_clk_pwm3 = 107,
      Tegra234_clk_sdhci = 120, // mmc@3400000
      Tegra234_clk_pex2_c8_core = 172,
      Tegra234_clk_tmclk = 219, // mmc@3400000
    };
    Mrq_clk(l4_uint32_t cmd, Clk_id clk_id = Clk_id{0})
    : req(cmd, clk_id)
    {}

    struct Req
    {
      Req(l4_uint32_t cmd, Clk_id clk_id = Clk_id{0})
      : cmd{(cmd << 24) | static_cast<l4_uint32_t>(clk_id), {}}
      {}

      struct Cmd
      {
        l4_uint32_t cmd_and_id; ///< bits 0..23: clock id, bits 24..31: sub-cmd
        l4_uint32_t __clear[3];
        constexpr l4_uint32_t cmd() const { return cmd_and_id >> 24; }
        constexpr l4_uint32_t id() const { return cmd_and_id & 0xffffff; }
      };
      struct Get_rate
      {
        enum { Cmd = 1 };
        l4_uint32_t __cmd_and_id; // Cmd
      };
      struct Set_rate
      {
        enum { Cmd = 2 };
        l4_uint32_t __cmd_and_id; // Cmd
        l4_uint32_t __unused;
        l4_int64_t rate;
      };
      struct Round_rate
      {
        enum { Cmd = 3 };
        l4_uint32_t __cmd_and_id; // Cmd
        l4_uint32_t __unused;
        l4_int64_t rate;
      };
      struct Get_parent
      {
        enum { Cmd = 4 };
        l4_uint32_t __cmd_and_id; // Cmd
      };
      struct Set_parent
      {
        enum { Cmd = 5 };
        l4_uint32_t __cmd_and_id; // Cmd
        l4_uint32_t parent_id;
      };
      struct Is_enabled
      {
        enum { Cmd = 6 };
        l4_uint32_t __cmd_and_id; // Cmd
      };
      struct Enable
      {
        enum { Cmd = 7 };
        l4_uint32_t __cmd_and_id; // Cmd
      };
      struct Disable
      {
        enum { Cmd = 8 };
        l4_uint32_t __cmd_and_id; // Cmd
      };
      struct Get_all_info
      {
        enum { Cmd = 14 };
      };
      struct Get_max_id
      {
        enum { Cmd = 15 };
        l4_uint32_t __cmd_and_id; // Cmd
      };
      struct Get_fpmax_at_vmin
      {
        enum { Cmd = 16 };
        l4_uint32_t __cmd_and_id; // Cmd
      };
      union
      {
        Cmd cmd;
        Get_rate get_rate;
        Set_rate set_rate;
        Round_rate round_rate;
        Get_parent get_parent;
        Set_parent set_parent;
        Is_enabled is_enabled;
        Enable enable;
        Disable disable;
        Get_all_info get_all_info;
        Get_max_id get_max_id;
        Get_fpmax_at_vmin get_fpmax_at_vmin;
      };

      /** Shall be logged with a higher priority. */
      constexpr bool is_important() const
      {
        switch (cmd.cmd())
          {
          case Set_rate::Cmd:
          case Set_parent::Cmd:
          case Enable::Cmd:
          case Disable::Cmd:
            return true;
          default:
            return false;
          }
      }

      /** Shall be logged with a lower priority. */
      constexpr bool is_frequent() const
      {
        switch (cmd.cmd())
          {
          case Get_rate::Cmd:
          case Get_parent::Cmd:
          case Is_enabled::Cmd:
          case Get_all_info::Cmd:
            return true;
          default:
            return false;
          }
      }

      /** Do we need to filter this request against black list / white list? */
      constexpr bool filter_required() const
      {
        switch (cmd.cmd())
          {
          case Enable::Cmd:
          case Disable::Cmd:
            return true;
          default:
            return false;
          }
      }

      void log(L4Re::Util::Dbg const &log, char const *prefix = "") const
      {
        if (cmd.cmd() == Set_rate::Cmd || cmd.cmd() == Round_rate::Cmd)
          {
            if (set_rate.rate == 0x7fffffff'ffffffff || set_rate.rate == 0xffffffff)
              log.printf("%sclk, cmd=%s, id=0x%x (%s, rate=MAX)\n",
                         prefix, fn_to_str(), cmd.id(), id_to_str());
            else
              log.printf("%sclk, cmd=%s, id=0x%x (%s, rate=%llu Hz)\n",
                         prefix, fn_to_str(), cmd.id(), id_to_str(), set_rate.rate);
          }
        else if (cmd.cmd() == Set_parent::Cmd)
          log.printf("%sclk, cmd=%s, id=0x%x (%s), parent=0x%x (%s)\n",
                     prefix, fn_to_str(), cmd.id(), id_to_str(),
                     set_parent.parent_id, id_to_str(set_parent.parent_id));
        else
          log.printf("%sclk, cmd=%s, id=0x%x (%s)\n",
                     prefix, fn_to_str(), cmd.id(), id_to_str());
      }
      char const *fn_to_str() const { return fn_to_str(cmd.cmd()); }
      char const *id_to_str() const { return id_to_str(cmd.id()); }
      static char const *fn_to_str(unsigned fn);
      static char const *id_to_str(unsigned clk_id);
    } req;
    static_assert(sizeof(Req) == 16);

    struct Rsp
    {
      struct Get_rate     { l4_int64_t rate; };
      struct Set_rate     { l4_int64_t rate; };
      struct Round_rate   { l4_int64_t rate; };
      struct Get_parent   { l4_uint32_t parent_id; };
      struct Set_parent   { l4_uint32_t parent_id; };
      struct Is_enabled   { l4_uint32_t state; };
      struct Enable       {};
      struct Disable      {};
      struct Get_all_info
      {
        enum Flags
        {
          // Has multiple parents?
          Has_mux = 1U << 0,
          // Clock rate can be changed.
          Has_set_rate = 1 << 1,
          // Has no parent? The only parent is 0x1'0000.
          Is_root = 1U << 2,
          Is_var_root = 1U << 3,
          // Set_rate or Set_parent will return error.
          No_change_parent = 1U << 30,
          // Enable or Disable will return error.
          No_change_state = 1U << 31,
        };
        l4_uint32_t flags;
        l4_uint32_t parent;
        l4_uint32_t parents[16];
        l4_uint8_t num_parents;
        l4_uint8_t name[40];
      };
      struct Get_max_id
      {
        l4_uint32_t max_id;
      };
      struct Get_fpmax_at_vmin
      {
        l4_uint64_t rate;
      };
      union
      {
        Get_rate get_rate;
        Set_rate set_rate;
        Round_rate round_rate;
        Get_parent get_parent;
        Set_parent set_parent;
        Is_enabled is_enabled;
        Enable enable;
        Disable disable;
        Get_all_info get_all_info;
        Get_max_id get_max_id;
        Get_fpmax_at_vmin get_fpmax_at_vmin;
      };
      bool log(L4Re::Util::Dbg const &log, l4_uint32_t cmd, l4_uint32_t code,
               char const *prefix = "") const
      {
        if (   (cmd >> 24) == Req::Set_rate::Cmd
            || (cmd >> 24) == Req::Get_rate::Cmd
            || (cmd >> 24) == Req::Round_rate::Cmd)
          {
            log.printf("%sclk, code=%d, rate=%lld Hz\n",
                       prefix, code, get_rate.rate);
            return true;
          }
        else if ((cmd >> 24) == Req::Get_parent::Cmd)
          {
            log.printf("%sclk, code=%d, parent=0x%x (%s)\n",
                       prefix, code, get_parent.parent_id,
                       Req::id_to_str(get_parent.parent_id));
            return true;
          }

        return false;
      }
    } rsp;
  };

  //////////////////////////////////////////////////////////////////////////////
  /** Call to check if MRQ is supported (Mrq = 23). */
  struct Mrq_query_abi
  {
    enum { Mrq = 23 };
    Mrq_query_abi(l4_uint32_t mrq)
    : req{mrq}
    {}

    struct Req
    {
      l4_uint32_t mrq;            ///< MRQ function number to check
    } req;

    struct Rsp
    {
      l4_uint32_t status;         ///< 0: OK; != 0: not supported
    } rsp;
  };

  //////////////////////////////////////////////////////////////////////////////
  /**
   * Thermal (Mrq = 27).
   *
   * Some of the MRQ messages sent by the CPU can lead to BPMP eventually
   * sending an MRQ message to the CPU.
   */
  struct Mrq_thermal
  {
    enum { Mrq = 27 };
    struct Req
    {
      struct Type
      {
        l4_uint32_t type;
      };
      struct Query_abi
      {
        enum { Cmd = 0 };
        l4_uint32_t __type; // Type
        l4_uint32_t type;
      };
      struct Get_temp
      {
        enum { Cmd = 1 };
        l4_uint32_t __type; // Type
        l4_uint32_t zone;
      };
      struct Set_trip
      {
        enum { Cmd = 2 };
        l4_uint32_t __type; // Type
        l4_uint32_t zone;
        l4_int32_t low;
        l4_int32_t high;
        l4_uint32_t enabled;
      };
      struct Get_num_zones
      {
        enum { Cmd = 3 };
        l4_uint32_t __type; // Type
        l4_uint32_t num;
      };
      struct Get_thermtrip
      {
        enum { Cmd = 4 };
        l4_uint32_t __type; // Type
        l4_uint32_t zone;
      };
      union
      {
        Type type;
        Query_abi query_abi;
        Get_temp get_temp;
        Set_trip set_trip;
        Get_num_zones get_num_zones;
        Get_thermtrip get_thermtrip;
      };

      void log(L4Re::Util::Dbg const &log, char const *prefix = "") const
      {
        if (type.type == Get_temp::Cmd)
          log.printf("%sthermal, cmd=%s, zone=0x%x\n",
                     prefix, fn_to_str(), get_temp.zone);
        else
          log.printf("%sthermal, cmd=%s\n", prefix, fn_to_str());
      }

      char const *fn_to_str() const { return fn_to_str(type.type); }
      static char const *fn_to_str(unsigned fn);
    };
    static_assert(sizeof(Req) == 20);

    struct Rsp
    {
      struct Get_temp
      {
        l4_int32_t temp;
      };
      struct Get_num_zones
      {
        l4_uint32_t num;
      };
      struct Get_thermtrip
      {
        l4_uint32_t thermtrip;
      };
      union
      {
        Get_temp get_temp;
        Get_num_zones get_num_zones;
        Get_thermtrip get_thermtrip;
      };

      bool log(L4Re::Util::Dbg const &log, l4_uint32_t cmd, l4_uint32_t code,
               char const *prefix = "") const
      {
        if (cmd == Req::Get_temp::Cmd)
          {
            l4_uint32_t t = get_temp.temp >= 0 ? get_temp.temp : -get_temp.temp;
            log.printf("%sthermal, code=%d, temperature=%s%u.%03u °C\n",
                       prefix, code,
                       get_temp.temp < 0 ? "-" : "", t / 1000, t % 1000);
            return true;
          }

        return false;
      }
    };

    struct Mrq_thermal_req_to_host
    {
      struct Trip_reached
      {
        l4_uint32_t zone;
      };
      l4_uint32_t type;
      union
      {
        Trip_reached trip_reached;
      };

    };
    static_assert((sizeof(Mrq_thermal_req_to_host) % 4) == 0);
  };

  //////////////////////////////////////////////////////////////////////////////
  /** Perform a power gate operation (Mrq = 66). */
  struct Mrq_pg
  {
    enum { Mrq = 66 };
    enum
    {
      Tegra234_power_domain_dummy = 0,
      Tegra234_power_domain_pciex4ca = 13,
    };
    Mrq_pg(l4_uint32_t cmd, l4_uint32_t pg_id)
    : req(cmd, pg_id)
    {
    }
    struct Req
    {
      Req(l4_uint32_t cmd, l4_uint32_t pg_id)
      : cmd{cmd, pg_id}
      {}

      struct Cmd
      {
        l4_uint32_t cmd;
        l4_uint32_t pg_id;
      };
      struct Query_abi
      {
        enum { Cmd = 0 };
        l4_uint32_t __cmd; // Cmd
        l4_uint32_t __id;  // Cmd
        l4_uint32_t type;
      };
      struct Set_state
      {
        enum { Cmd = 1 };
        l4_uint32_t __cmd; // Cmd
        l4_uint32_t __id;  // Cmd
        l4_uint32_t state;
      };
      struct Get_state
      {
        enum { Cmd = 2 };
        l4_uint32_t __cmd; // Cmd
        l4_uint32_t __id;  // Cmd
      };
      struct Get_name
      {
        enum { Cmd = 3 };
        l4_uint32_t __cmd; // Cmd
        l4_uint32_t __id;  // Cmd
      };
      struct Get_max_id
      {
        enum { Cmd = 4 };
        l4_uint32_t __cmd; // Cmd
        l4_uint32_t __id;  // Cmd
      };
      union
      {
        Cmd cmd;
        Query_abi query_abi;
        Set_state set_state;
        Get_state get_state;
        Get_name get_name;
        Get_max_id get_max_id;
      };

      /** Shall be logged with a higher priority. */
      constexpr bool is_important() const
      { return cmd.cmd == Set_state::Cmd; }

      /** Shall be logged with a lower priority. */
      constexpr bool is_frequent() const
      {
        switch (cmd.cmd)
          {
          case Get_name::Cmd:
            return true;
          default:
            return false;
          }
      }

      /** Do we need to filter this request against black list / white list? */
      constexpr bool filter_required() const
      {
        switch (cmd.cmd)
          {
          case Set_state::Cmd:
            return true;
          default:
            return false;
          }
      }

      void log(L4Re::Util::Dbg const &log, char const *prefix = "") const
      {
        char const *extra = "";
        if (cmd.cmd == Set_state::Cmd)
          extra = set_state.state == 0
                    ? ", OFF"
                    : set_state.state == 1
                        ? ", ON"
                        : ",  RUNNING";
        log.printf("%spg, cmd=%s, id=0x%x (%s%s)\n",
                   prefix, fn_to_str(), cmd.pg_id, id_to_str(), extra);
      }

      char const *fn_to_str() const { return fn_to_str(cmd.cmd); }
      char const *id_to_str() const { return id_to_str(cmd.pg_id); }
      static char const *fn_to_str(unsigned fn);
      static char const *id_to_str(unsigned pg_id);
    } req;
    static_assert(sizeof(Req) == 12);

    struct Rsp
    {
      struct Get_state
      {
        l4_uint32_t state;
      };
      struct Get_name
      {
        l4_uint8_t name[40];
      };
      struct Get_max_id
      {
        l4_uint32_t max_id;
      };
      union
      {
        Get_state get_state;
        Get_name get_name;
        Get_max_id get_max_id;
      };
    } rsp;
    static_assert(sizeof(Rsp) == 40);
  };

  //////////////////////////////////////////////////////////////////////////////
  /** Perform a STRAP operation (Mrq = 68). */
  struct Mrq_strap
  {
    enum { Mrq = 68 };
    Mrq_strap(l4_uint32_t cmd, l4_uint32_t strap_id, l4_uint32_t value)
    : req(cmd, strap_id, value)
    {
    }

    struct Req
    {
      Req(l4_uint32_t cmd, l4_uint32_t strap_id, l4_uint32_t value)
      : cmd{cmd, strap_id, value}
      {}

      struct Cmd
      {
        l4_uint32_t cmd;
        l4_uint32_t strap_id;
        l4_uint32_t value;
      } cmd;

    } req;
  };

  //////////////////////////////////////////////////////////////////////////////
  /** Perform a UPHY operation (Mrq = 69). */
  struct Mrq_uphy
  {
    enum { Mrq = 69 };
    struct Req
    {
      enum
      {
        Cmd_uphy_pcie_controller_state = 4,
      };
      l4_uint16_t lane;           ///< lane number
      l4_uint16_t cmd;            ///< sub-command id
      union
      {
        struct
        {
          l4_uint8_t pcie_controller;
          l4_uint8_t enable;
        } controller_state;
        char __pad[20];           ///< minimum payload 24 (20+4)
      };
    } req;

    struct Rsp
    {
      l4_uint32_t status;         ///< number of errors observed
    } rsp;
  };

  //////////////////////////////////////////////////////////////////////////////
  /** Query BPMP firmware tag (unique identifier, Mrq = 71). */
  struct Mrq_query_fw_tag
  {
    enum { Mrq = 71 };
    struct Req
    {
                                  ///< minimum payload 0
    } req;

    struct Rsp
    {
      l4_uint8_t tag[32];         ///< firmware tag
    } rsp;
  };

  //////////////////////////////////////////////////////////////////////////////
  /** Debug (Mrq = 75). */
  struct Mrq_debug
  {
    enum { Mrq = 75 };
    struct Req
    {
      Req(unsigned cmd)
      : cmd{cmd}
      {}
      struct Cmd
      {
        l4_uint32_t cmd;
      };
      struct Open
      {
        l4_uint32_t cmd;
        char name[116];
      };
      struct Open_ro : public Open
      {
        enum { Cmd = 0 };
      };
      struct Open_rw : public Open
      {
        enum { Cmd = 1 };
      };
      struct Read
      {
        enum { Cmd = 2 };
        l4_uint32_t cmd;
        l4_uint32_t fd;
      };
      struct Write
      {
        enum { Cmd = 3 };
        l4_uint32_t cmd;
        l4_uint32_t fd;
        l4_uint32_t datalen;
        char data[108];
      };
      struct Close
      {
        enum { Cmd = 4 };
        l4_uint32_t cmd;
        l4_uint32_t fd;
      };

      union
      {
        Cmd cmd;
        Open_ro open_ro;
        Open_rw open_rw;
        Read read;
        Write write;
        Close close;
      };

      void log(L4Re::Util::Dbg const &log, char const *prefix = "") const
      {
        if (cmd.cmd == Open_ro::Cmd|| cmd.cmd == Open_rw::Cmd)
          log.printf("%sdebug, cmd=%s, fn=%.32s\n",
                     prefix, fn_to_str(), open_ro.name);
        else
          log.printf("%sdebug, cmd=%s\n", prefix, fn_to_str());
      }

      char const *fn_to_str() const;
    } req;
    static_assert(sizeof(Req) == 120);

    struct Rsp
    {
      struct Open
      {
        l4_uint32_t fd;
        l4_uint32_t datalen;
      };
      struct Read
      {
        l4_uint32_t readlen;
        char data[116];
      };
      union
      {
        Open open;
        Read read;
      };
    } resp;
    static_assert(sizeof(Rsp) == 120);
  };
};
