/*
 * Copyright (C) 2026 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 */

#include <l4/re/util/object_registry>

#include "client_collection.h"

class Tegra_bpmp;
struct lua_State;

class Client_factory
: public L4::Epiface_t<Client_factory, L4::Factory>,
  public Client_collection
{
public:
  Client_factory(L4Re::Util::Object_registry *registry, lua_State *ls,
                 Tegra_bpmp *phys_device);

  long op_create(L4::Factory::Rights, L4::Ipc::Cap<void> &res,
                 l4_umword_t type, L4::Ipc::Varg_list_ref valist);

private:
  /**
   * Load a bundle (black list + white list) from a client profile.
   *
   * This information is used by Tegra_bpmp::Id_vec_bundle::disallowed().
   *
   * \param profile     The client profile name.
   * \param bdl         The bundle containing the black list and white list.
   * \param bdl_name    The name of the bundle. The tables 'bdl_name'_"white"
   *                    and 'bdl_name'_"black" are read from the client profile.
   * \param entry_name  The name of a single entry used for used logging.
   * \param idtostr     A function to determine the name of a single ID used for
   *                    logging.
   */
  template <typename T>
  bool load_profile_bundle(std::string const profile,
                           Tegra_bpmp::Id_vec_bundle<T> &bdl,
                           std::string const bundle_name,
                           char const *entry_name,
                           char const * (*idtostr)(unsigned id));

  /**
   * Load a black list or white list from a client profile.
   *
   * \param profile  The client profile name.
   * \param name     The name of the table.
   * \param tbl      The table.
   * \param tbl_all  Pointer to bool value storing the information if the
   *                 entries table was selected (in that case, dedicated entries
   *                 are ignored) or not. This setting is 'nullptr' for black
   *                 lists and point to a bool for white lists.
   */
  template <typename T>
  bool load_profile_table(std::string const profile, std::string const name,
                          std::vector<T> &tbl, bool *tbl_all);

  L4Re::Util::Object_registry *_registry;
  lua_State *_ls;
  Tegra_bpmp *_phys_device;

  Dbg warn{Dbg::Warn, "factory"};
  Dbg info{Dbg::Info, "factory"};
  Dbg trace5{Dbg::Trace5, "factory"};
};
