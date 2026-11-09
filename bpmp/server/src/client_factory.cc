/*
 * Copyright (C) 2026 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

/**
 * \file
 */

#include <lua.h>
#include <string>
#include <utility>

#include "client_factory.h"

static bool
parse_string_param(L4::Ipc::Varg const &param, std::string const prefix,
                   std::string *out)
{
  l4_size_t prefixlen = prefix.length();
  if (param.length() < prefixlen)
    return false;

  std::string const pstr(param.value<char const *>());
  if (pstr.compare(0, prefixlen, prefix) != 0)
    return false;

  *out = pstr.substr(prefixlen);
  return true;
}

Client_factory::Client_factory(L4Re::Util::Object_registry *registry,
                               lua_State *_ls, Tegra_bpmp *phys_device)
: _registry(registry), _ls(_ls), _phys_device(phys_device)
{
  L4Re::chkcap(registry->register_obj(this, "svr"),
               "Register factory server");
}

long
Client_factory::op_create(L4::Factory::Rights, L4::Ipc::Cap<void> &res,
                          l4_umword_t type, L4::Ipc::Varg_list_ref valist)
{
  if (type != 0)
    {
      warn.printf("op_create: invalid object type.\n");
      return -L4_EINVAL;
    }

  std::string profile;
  for (L4::Ipc::Varg const p: valist)
    {
      if (!p.is_of<char const *>())
        {
          warn.printf("op_create: string parameter expected.\n");
          return -L4_EINVAL;
        }

      if (parse_string_param(p, "profile=", &profile))
        continue;
    }

  if (profile.empty())
    {
      warn.printf("op_create: required parameter 'profile=' not found.\n");
      return -L4_EINVAL;
    }

  // Restore the Lua stack on every return path.
  struct Stack_guard
  {
    lua_State *ls;
    int top;
    ~Stack_guard() { lua_settop(ls, top); }
  } stack_guard{_ls, lua_gettop(_ls)};

  lua_getglobal(_ls, profile.c_str());
  if (lua_isnil(_ls, -1) || !lua_istable(_ls, -1))
    {
      warn.printf("op_create: profile '%s' not found.\n", profile.c_str());
      return -L4_EINVAL;
    }

  lua_pushstring(_ls, "bpmp");
  lua_gettable(_ls, -2);
  if (lua_isnil(_ls, -1) || !lua_istable(_ls, -1))
    {
      warn.printf("op_create(%s): 'bpmp' not found.\n", profile.c_str());
      return -L4_EINVAL;
    }

  Tegra_bpmp::Id_vec_bundle<l4_uint16_t> clocks;
  if (!load_profile_bundle(profile, clocks, "clocks", "clock",
                           Tegra_bpmp_if::Mrq_clk::Req::id_to_str))
    return -L4_EINVAL;

  Tegra_bpmp::Id_vec_bundle<l4_uint8_t> powergates;
  if (!load_profile_bundle(profile, powergates, "powergates", "pg",
                           Tegra_bpmp_if::Mrq_pg::Req::id_to_str))
    return -L4_EINVAL;


  Tegra_bpmp::Id_vec_bundle<l4_uint8_t> resetlines;
  if (!load_profile_bundle(profile, resetlines, "resetlines", "reset",
                           Tegra_bpmp_if::Mrq_reset::Req::id_to_str))
    return -L4_EINVAL;

  try
    {
      auto client = cxx::make_ref_obj<Client>(
                           _registry, _phys_device, profile,
                           clocks, powergates, resetlines);
      _clients.push_back(client);
      res = L4::Ipc::make_cap(client->obj_cap(), L4_CAP_FPAGE_RWSD);
    }
  catch (L4::Runtime_error const &e)
    {
      warn.printf("%s: %s\n", e.extra_str(), e.str());
      return e.err_no();
    }

  return L4_EOK;
}

template <typename T>
bool
Client_factory::load_profile_bundle(std::string const profile,
                                    Tegra_bpmp::Id_vec_bundle<T> &bdl,
                                    std::string const bdl_name,
                                    char const *entry_name,
                                    char const *(*idtostr)(unsigned i))
{
  if (   !load_profile_table(profile, bdl_name + "_black",
                             bdl.black.vec, nullptr)
      || !load_profile_table(profile, bdl_name + "_white",
                             bdl.white.vec, &bdl.all_white))
    return false;

  if (bdl.all_white)
    info.printf("op_create(%s): \033[31;1mall %ss white listed.\033[m\n",
                profile.c_str(), entry_name);
  else
    {
      info.printf("op_create(%s): read %zu entries from table '%s_white':\n",
                  profile.c_str(), bdl.white.vec.size(), bdl_name.c_str());
      if (bdl.white.vec.size() == 0)
        info.printf("op_create(%s):   \033[31mno white list entries\033[m\n",
                    profile.c_str());
      for (auto i : bdl.white.vec)
        info.printf("op_create(%s):   \033[32;1m+%s id 0x%x (%s)\033[m\n",
                    profile.c_str(), entry_name, i, idtostr(i));
    }

  info.printf("op_create(%s): read %zu entries from table '%s_black':\n",
              profile.c_str(), bdl.black.vec.size(), bdl_name.c_str());
  if (bdl.black.vec.size() == 0)
    info.printf("op_create(%s):   \033[31mno black list entries,\033[m\n",
                profile.c_str());
  for (auto i : bdl.black.vec)
    info.printf("op_create(%s):   \033[31;1m-%s id 0x%x (%s)\033[m\n",
                profile.c_str(), entry_name, i, idtostr(i));

  return true;
}

template <typename T>
bool
Client_factory::load_profile_table(std::string const profile,
                                   std::string const name,
                                   std::vector<T> &tbl, bool *tbl_all)
{
  lua_pushstring(_ls, name.c_str());
  lua_gettable(_ls, -2);

  trace5.printf("op_create(%s): loading table '%s'\n", profile.c_str(), name.c_str());
  unsigned pop = 1;

  // little helper to reduce typing
  auto error = [this, &pop, profile, name](char const *str_err, int key = -1)
    {
      char key_str[32] = "";
      if (key != -1)
        snprintf(key_str, sizeof(key_str), " key=%d", key);
      warn.printf("op_create(%s): table '%s%s: %s.\n",
                  profile.c_str(), name.c_str(), key_str, str_err);
      lua_pop(_ls, pop);
      return false;
    };

  if (lua_isnil(_ls, -1) || !lua_istable(_ls, -1))
    return error("cannot load table");

  // For white lists, we also allow to specify "all" to allow every entry.
  // We don't allow "all" for black list because, by default, with empty white
  // and black lists, the client is not allowed to perform any access.
  if (tbl_all)
    *tbl_all = false;
  int count = 0; // count key/value pairs

  lua_pushnil(_ls);
  while (lua_next(_ls, -2) != 0) // key at index -2, value at index -1
    {
      pop = 3; // table, key, value
      ++count;

      trace5.printf("  %3u. entry: key=%d/%s, val=%d/%s\n",
                    count,
                    lua_isnumber(_ls, -2), lua_typename(_ls, lua_type(_ls, -2)),
                    lua_isnumber(_ls, -1), lua_typename(_ls, lua_type(_ls, -1)));

      if (!lua_isnumber(_ls, -2))
        return error("found non-Integer key", count);

      int k = lua_tointeger(_ls, -2);
      if (k != count)
        return error("skipped entries", k);

      if (lua_isnumber(_ls, -1))
        {

          int v = lua_tointeger(_ls, -1);

          // insert element into sorted vector
          tbl.insert(lower_bound(tbl.begin(), tbl.end(), v), v);
        }
      else
        {
          // call lua_tostring() only on a string!
          if (lua_type(_ls, -1) != LUA_TSTRING)
            return error("not an Integer", k);

          char const *s = lua_tostring(_ls, -1);
          if (!strcmp(s, "all"))
            {
              if (!tbl_all)
                return error("'all' in black list", k);
              if (*tbl_all)
                return error("contains 'all' more than once", k);

              *tbl_all = true;
              lua_pop(_ls, 1);
              continue;
            }
          else
            return error("contains unknown entry", k);
        }
      lua_pop(_ls, 1);
    }
  lua_pop(_ls, 1);

  trace5.printf("op_create(%s): done loading table '%s'.\n",
                profile.c_str(), name.c_str());
  return true;
}
