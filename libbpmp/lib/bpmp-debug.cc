/*
 * Copyright (C) 2026 Kernkonzept GmbH.
 * Author(s): Frank Mehnert <frank.mehnert@kernkonzept.com>
 *
 * License: see LICENSE.spdx (in this directory or the directories above)
 */

#include <l4/drv/tegra/bpmp/bpmp_if.h>

////////////////////////////////////////////////////////////////////////////////
// General
////////////////////////////////////////////////////////////////////////////////
char const *Tegra_bpmp_if::req_to_str(unsigned i)
{
  switch (i)
    {
     case 0: return "ping";
     case 1: return "query tag";
     case 9: return "threaded ping";
    case 19: return "debugfs";
    case 20: return "reset";
    case 21: return "i2c";
    case 22: return "clk";
    case 23: return "query abi";
    case 27: return "thermal";
    case 28: return "cpu vhint";
    case 29: return "abi ratchet";
    case 31: return "emc dvfs latency";
    case 65: return "ringbuf console";
    case 66: return "pg";
    case 67: return "cpu ndiv limits";
    case 68: return "stray";
    case 69: return "uphy";
    case 70: return "cpu auto cc3";
    case 71: return "query fw tag";
    case 72: return "fpmon";
    case 73: return "ec";
    case 75: return "debug";
    case 76: return "emc dvfs emchub";
    case 77: return "bwmgr";
    case 78: return "iso client";
    case 79: return "emc disp rfl";
    case 80: return "telemtry";
    case 81: return "pwr limit";
    case 82: return "gears";
    case 83: return "bwmgr int";
    case 84: return "oc status";
    default: return "UNKNOWN";
  };
}


////////////////////////////////////////////////////////////////////////////////
// MRQ = 20 (reset)
////////////////////////////////////////////////////////////////////////////////
char const *Tegra_bpmp_if::Mrq_reset::Req::fn_to_str(unsigned fn)
{
  constexpr char const *const fn_str[5] =
  {
    "invalid: 0", "assert", "deassert", "assert+deassert", "get max id"
  };
  return fn < 5 ? fn_str[fn] : "\033[31mINVALID\033[m";
}

char const *Tegra_bpmp_if::Mrq_reset::Req::id_to_str(unsigned reset_id)
{
  switch (reset_id)
    {
    case  0x1: return "Actmon";
    case  0x2: return "Adsp_all";
    case  0x3: return "Dsi_core";
    case  0x4: return "Can1";
    case  0x5: return "Can2";
    case  0x6: return "Dla0";
    case  0x7: return "Dla1";
    case  0x8: return "Dpaux";
    case  0x9: return "Ofa";
    case  0xa: return "Nvjpg1";
    case  0xb: return "Pex1_core_6";
    case  0xc: return "Pex1_core_6_apb";
    case  0xd: return "Pex1_common_apb";
    case  0xe: return "Pex2_core_7";
    case  0xf: return "Pex2_core_7_apb";
    case 0x10: return "Nvdisplay";
    case 0x11: return "Eqos";
    case 0x12: return "Gpcdma";
    case 0x13: return "Gpu";
    case 0x14: return "Hda";
    case 0x15: return "Hdacodec";
    case 0x16: return "Eqos_macsec";
    case 0x17: return "Eqos_macsec_secure";
    case 0x18: return "I2c1";
    case 0x19: return "Pex2_core_8";
    case 0x1a: return "Pex2_core_8_apb";
    case 0x1b: return "Pex2_core_9";
    case 0x1c: return "Pex2_core_9_apb";
    case 0x1d: return "I2c2";
    case 0x1e: return "I2c3";
    case 0x1f: return "I2c4";
    case 0x20: return "I2c6";
    case 0x21: return "I2c7";
    case 0x22: return "I2c8";
    case 0x23: return "I2c9";
    case 0x24: return "Isp";
    case 0x25: return "Mipi_cal";
    case 0x26: return "Mphy_clk_ctl";
    case 0x27: return "Mphy_l0_rx";
    case 0x28: return "Mphy_l0_tx";
    case 0x29: return "Mphy_l1_rx";
    case 0x2a: return "Mphy_l1_tx";
    case 0x2b: return "Nvcsi";
    case 0x2c: return "Nvdec";
    case 0x2d: return "Mgbe0_pcs";
    case 0x2e: return "Mgbe0_mac";
    case 0x2f: return "Mgbe0_macsec";
    case 0x30: return "Mgbe0_macsec_secure";
    case 0x31: return "Mgbe1_pcs";
    case 0x32: return "Mgbe1_mac";
    case 0x33: return "Mgbe1_macsec";
    case 0x34: return "Mgbe1_macsec_secure";
    case 0x35: return "Mgbe2_pcs";
    case 0x36: return "Mgbe2_mac";
    case 0x37: return "Mgbe2_macsec";
    case 0x38: return "Pex2_core_10";
    case 0x39: return "Pex2_core_10_apb";
    case 0x3a: return "Pex2_common_apb";
    case 0x3b: return "Nvenc";
    case 0x3c: return "Mgbe2_macsec_secure";
    case 0x3d: return "Nvjpg";
    case 0x40: return "La";
    case 0x41: return "Hwpm";
    case 0x42: return "Pva0_all";
    case 0x43: return "Cec";
    case 0x44: return "Pwm1";
    case 0x45: return "Pwm2";
    case 0x46: return "Pwm3";
    case 0x47: return "Pwm4";
    case 0x48: return "Pwm5";
    case 0x49: return "Pwm6";
    case 0x4a: return "Pwm7";
    case 0x4b: return "Pwm8";
    case 0x4c: return "Qspi0";
    case 0x4d: return "Qspi1";
    case 0x4e: return "I2s7";
    case 0x4f: return "I2s8";
    case 0x50: return "Sce_all";
    case 0x51: return "Rce_all";
    case 0x52: return "Sdmmc1";
    case 0x53: return "Rsvd_83";
    case 0x54: return "Rsvd_84";
    case 0x55: return "Sdmmc4";
    case 0x56: return "Mgbe3_pcs";
    case 0x57: return "Mgbe3_mac";
    case 0x58: return "Mgbe3_macsec";
    case 0x59: return "Mgbe3_macsec_secure";
    case 0x5a: return "Spi1";
    case 0x5b: return "Spi2";
    case 0x5c: return "Spi3";
    case 0x5d: return "Spi4";
    case 0x5e: return "Tach0";
    case 0x5f: return "Tach1";
    case 0x60: return "Spi5";
    case 0x61: return "Tsec";
    case 0x62: return "Uarti";
    case 0x63: return "Uarta";
    case 0x64: return "Uartb";
    case 0x65: return "Uartc";
    case 0x66: return "Uartd";
    case 0x67: return "Uarte";
    case 0x68: return "Uartf";
    case 0x69: return "Uartj";
    case 0x6a: return "Uarth";
    case 0x6b: return "Ufshc";
    case 0x6c: return "Ufshc_axi_m";
    case 0x6d: return "Ufshc_lp_seq";
    case 0x6e: return "Rsvd_111";
    case 0x6f: return "Vi";
    case 0x70: return "Vic";
    case 0x71: return "Xusb_padctl";
    case 0x72: return "Vi2";
    case 0x73: return "Pex0_core_0";
    case 0x74: return "Pex0_core_1";
    case 0x75: return "Pex0_core_2";
    case 0x76: return "Pex0_core_3";
    case 0x77: return "Pex0_core_4";
    case 0x78: return "Pex0_core_0_apb";
    case 0x79: return "Pex0_core_1_apb";
    case 0x7a: return "Pex0_core_2_apb";
    case 0x7b: return "Pex0_core_3_apb";
    case 0x7c: return "Pex0_core_4_apb";
    case 0x7d: return "Pex0_common_apb";
    case 0x7e: return "Rsvd_127";
    case 0x7f: return "Nvhs_uphy_pll1";
    case 0x80: return "Pex1_core_5";
    case 0x81: return "Pex1_core_5_apb";
    case 0x82: return "Gbe_uphy";
    case 0x83: return "Gbe_uphy_pm";
    case 0x84: return "Nvhs_uphy";
    case 0x85: return "Nvhs_uphy_pll0";
    case 0x86: return "Nvhs_uphy_l0";
    case 0x87: return "Nvhs_uphy_l1";
    case 0x88: return "Nvhs_uphy_l2";
    case 0x89: return "Nvhs_uphy_l3";
    case 0x8a: return "Nvhs_uphy_l4";
    case 0x8b: return "Nvhs_uphy_l5";
    case 0x8c: return "Nvhs_uphy_l6";
    case 0x8d: return "Nvhs_uphy_l7";
    case 0x8e: return "Nvhs_uphy_pm";
    case 0x8f: return "Dmic5";
    case 0x90: return "Ape";
    case 0x91: return "Pex_usb_uphy";
    case 0x92: return "Pex_usb_uphy_l0";
    case 0x93: return "Pex_usb_uphy_l1";
    case 0x94: return "Pex_usb_uphy_l2";
    case 0x95: return "Pex_usb_uphy_l3";
    case 0x96: return "Pex_usb_uphy_l4";
    case 0x97: return "Pex_usb_uphy_l5";
    case 0x98: return "Pex_usb_uphy_l6";
    case 0x99: return "Pex_usb_uphy_l7";
    case 0x9e: return "Pex_usb_uphy_pll0";
    case 0x9f: return "Pex_usb_uphy_pll1";
    case 0xa0: return "Pex_usb_uphy_pll2";
    case 0xa1: return "Pex_usb_uphy_pll3";
    case 0xa2: return "Gbe_uphy_l0";
    case 0xa3: return "Gbe_uphy_l1";
    case 0xa4: return "Gbe_uphy_l2";
    case 0xa5: return "Gbe_uphy_l3";
    case 0xa6: return "Gbe_uphy_l4";
    case 0xa7: return "Gbe_uphy_l5";
    case 0xa8: return "Gbe_uphy_l6";
    case 0xa9: return "Gbe_uphy_l7";
    case 0xaa: return "Gbe_uphy_pll0";
    case 0xab: return "Gbe_uphy_pll1";
    case 0xac: return "Gbe_uphy_pll2";
    default:   return "UNKNOWN";
    }
}


////////////////////////////////////////////////////////////////////////////////
// MRQ = 22 (clocks)
////////////////////////////////////////////////////////////////////////////////
char const *Tegra_bpmp_if::Mrq_clk::Req::fn_to_str(unsigned fn)
{
  constexpr char const *const fn_str[17] =
  {
    "invalid: 0", "get rate", "set rate", "round rate", "get parent",
    "set parent", "is enabled", "enable", "disable", "properties",
    "possible parents", "num possible parents", "get possible_parents",
    "reset refcounts", "get all info", "get max id", "get fpmax at vmin"
  };

  return fn < 17 ? fn_str[fn] : "\033[31mINVALID\033[m";
}

char const *Tegra_bpmp_if::Mrq_clk::Req::id_to_str(unsigned clk_id)
{
  switch (clk_id)
    {
    case  0x01: return "Actmon";
    case  0x02: return "Adsp";
    case  0x03: return "Adspneon";
    case  0x04: return "Ahub";
    case  0x05: return "Apb2ape";
    case  0x06: return "Ape";
    case  0x07: return "Aud_mclk";
    case  0x08: return "Axi_cbb";
    case  0x09: return "Can1";
    case  0x0a: return "Can1_host";
    case  0x0b: return "Can2";
    case  0x0c: return "Can2_host";
    case  0x0e: return "Clk_m";
    case  0x0f: return "Dmic1";
    case  0x10: return "Dmic2";
    case  0x11: return "Dmic3";
    case  0x12: return "Dmic4";
    case  0x13: return "Dpaux";
    case  0x14: return "Nvjpg1";
    case  0x15: return "Aclk";
    case  0x16: return "Mss_encrypt";
    case  0x17: return "Eqos_rx_input";
    case  0x19: return "Aon_apb";
    case  0x1a: return "Aon_nic";
    case  0x1b: return "Aon_cpu_nic";
    case  0x1c: return "Pll_a1";
    case  0x1d: return "Dspk1";
    case  0x1e: return "Dspk2";
    case  0x1f: return "Emc";
    case  0x20: return "Eqos_axi";
    case  0x21: return "Eqos_ptp_ref";
    case  0x22: return "Eqos_rx";
    case  0x23: return "Eqos_tx";
    case  0x24: return "Extperiph1";
    case  0x25: return "Extperiph2";
    case  0x26: return "Extperiph3";
    case  0x27: return "Extperiph4";
    case  0x28: return "Fuse";
    case  0x29: return "Gpc0clk";
    case  0x2a: return "Gpu_pwr";
    case  0x2e: return "Host1x";
    case  0x2f: return "Xusb_hs_hsicp";
    case  0x30: return "I2c1";
    case  0x31: return "I2c2";
    case  0x32: return "I2c3";
    case  0x33: return "I2c4";
    case  0x34: return "I2c6";
    case  0x35: return "I2c7";
    case  0x36: return "I2c8";
    case  0x37: return "I2c9";
    case  0x38: return "I2s1";
    case  0x39: return "I2s1_sync_input";
    case  0x3a: return "I2s2";
    case  0x3b: return "I2s2_sync_input";
    case  0x3c: return "I2s3";
    case  0x3d: return "I2s3_sync_input";
    case  0x3e: return "I2s4";
    case  0x3f: return "I2s4_sync_input";
    case  0x40: return "I2s5";
    case  0x41: return "I2s5_sync_input";
    case  0x42: return "I2s6";
    case  0x43: return "I2s6_sync_input";
    case  0x45: return "Isp";
    case  0x46: return "Eqos_rx_m";
    case  0x47: return "Maud";
    case  0x48: return "Mipi_cal";
    case  0x49: return "Mphy_core_pll_fixed";
    case  0x4a: return "Mphy_l0_rx_ana";
    case  0x4b: return "Mphy_l0_rx_ls_bit";
    case  0x4c: return "Mphy_l0_rx_symb";
    case  0x4d: return "Mphy_l0_tx_ls_3xbit";
    case  0x4e: return "Mphy_l0_tx_symb";
    case  0x4f: return "Mphy_l1_rx_ana";
    case  0x50: return "Mphy_tx_1mhz_ref";
    case  0x51: return "Nvcsi";
    case  0x52: return "Nvcsilp";
    case  0x53: return "Nvdec";
    case  0x54: return "Hub";
    case  0x55: return "Disp";
    case  0x56: return "Nvdisplay_p0";
    case  0x57: return "Nvdisplay_p1";
    case  0x58: return "Dsc";
    case  0x5a: return "Nvjpg";
    case  0x5b: return "Osc";
    case  0x5c: return "Aon_touch";
    case  0x5d: return "Pll_a";
    case  0x5e: return "Pll_aon";
    case  0x64: return "Pll_e";
    case  0x65: return "Pll_p";
    case  0x66: return "Pllp_out0";
    case  0x67: return "Utmip_pll";
    case  0x68: return "Plla_out0";
    case  0x69: return "Pwm1";
    case  0x6a: return "Pwm2";
    case  0x6b: return "Pwm3";
    case  0x6c: return "Pwm4";
    case  0x6d: return "Pwm5";
    case  0x6e: return "Pwm6";
    case  0x6f: return "Pwm7";
    case  0x70: return "Pwm8";
    case  0x71: return "Rce_cpu_nic";
    case  0x72: return "Rce_nic";
    case  0x75: return "Aon_i2c_slow";
    case  0x76: return "Sce_cpu_nic";
    case  0x77: return "Sce_nic";
    case  0x78: return "Sdmmc1";
    case  0x79: return "Uphy_pll3";
    case  0x7b: return "Sdmmc4";
    case  0x7c: return "Se";
    case  0x7d: return "Sor0_pll_ref";
    case  0x7e: return "Sor0_ref";
    case  0x7f: return "Sor1_pll_ref";
    case  0x80: return "Pre_sor0_ref";
    case  0x81: return "Sor1_ref";
    case  0x82: return "Pre_sor1_ref";
    case  0x83: return "Sor_safe";
    case  0x84: return "Sor0_div";
    case  0x86: return "Dmic5";
    case  0x87: return "Spi1";
    case  0x88: return "Spi2";
    case  0x89: return "Spi3";
    case  0x8a: return "I2c_slow";
    case  0x8b: return "Dmic1_sync_clk";
    case  0x8c: return "Dmic2_sync_clk";
    case  0x8d: return "Dmic3_sync_clk";
    case  0x8e: return "Dmic4_sync_clk";
    case  0x8f: return "Dspk1_sync_clk";
    case  0x90: return "Dspk2_sync_clk";
    case  0x91: return "I2s1_sync_clk";
    case  0x92: return "I2s2_sync_clk";
    case  0x93: return "I2s3_sync_clk";
    case  0x94: return "I2s4_sync_clk";
    case  0x95: return "I2s5_sync_clk";
    case  0x96: return "I2s6_sync_clk";
    case  0x97: return "Mphy_flsm";
    case  0x98: return "Tach0";
    case  0x99: return "Tsec";
    case  0x9a: return "Tsec_pka";
    case  0x9b: return "Uarta";
    case  0x9c: return "Uartb";
    case  0x9d: return "Uartc";
    case  0x9e: return "Uartd";
    case  0x9f: return "Uarte";
    case  0xa0: return "Uartf";
    case  0xa1: return "Pex1_c6_core";
    case  0xa2: return "Uart_fst_mipi_cal";
    case  0xa3: return "Ufsdev_ref";
    case  0xa4: return "Ufshc";
    case  0xa5: return "Usb2_trk";
    case  0xa6: return "Vi";
    case  0xa7: return "Vic";
    case  0xa8: return "Csite";
    case  0xa9: return "Ist";
    case  0xab: return "Pex2_c7_core";
    case  0xac: return "Pex2_c8_core";
    case  0xad: return "Pex2_c9_core";
    case  0xb2: return "Sor0";
    case  0xb3: return "Sor1";
    case  0xb4: return "Sor_pad_input";
    case  0xb5: return "Pre_sf0";
    case  0xb6: return "Sf0";
    case  0xb7: return "Sf1";
    case  0xb8: return "Dsi_pad_input";
    case  0xbb: return "Pex2_c10_core";
    case  0xbc: return "Uarti";
    case  0xbd: return "Uartj";
    case  0xbe: return "Uarth";
    case  0xbf: return "Fuse_serial";
    case  0xc0: return "Qspi0_2x_pm";
    case  0xc1: return "Qspi1_2x_pm";
    case  0xc2: return "Qspi0_pm";
    case  0xc3: return "Qspi1_pm";
    case  0xc4: return "Vi_const";
    case  0xc5: return "Nafll_bpmp";
    case  0xc6: return "Nafll_sce";
    case  0xc7: return "Nafll_nvdec";
    case  0xc8: return "Nafll_nvjpg";
    case  0xc9: return "Nafll_tsec";
    case  0xcb: return "Nafll_vi";
    case  0xcc: return "Nafll_se";
    case  0xce: return "Nafll_isp";
    case  0xcf: return "Nafll_vic";
    case  0xd2: return "Nafll_nvjpg1";
    case  0xd5: return "Dbgapb";
    case  0xd6: return "Nafll_rce";
    case  0xd7: return "La";
    case  0xd8: return "Pllp_out_jtag";
    case  0xd9: return "Sdmmc4_axicif";
    case  0xdb: return "Sdmmc_legacy_tm";
    case  0xdc: return "Pex0_c0_core";
    case  0xdd: return "Pex0_c1_core";
    case  0xde: return "Pex0_c2_core";
    case  0xdf: return "Pex0_c3_core";
    case  0xe0: return "Pex0_c4_core";
    case  0xe1: return "Pex1_c5_core";
    case  0xe5: return "Pex0_c0_core_m";
    case  0xe6: return "Pex0_c1_core_m";
    case  0xe7: return "Pex0_c2_core_m";
    case  0xe8: return "Pex0_c3_core_m";
    case  0xe9: return "Pex0_c4_core_m";
    case  0xea: return "Pex1_c5_core_m";
    case  0xeb: return "Pex1_c6_core_m";
    case  0xed: return "Pll_c4";
    case  0xef: return "Pllc4_out1";
    case  0xf0: return "Pllc4_out2";
    case  0xf1: return "Pllc4_muxed";
    case  0xf2: return "Pllc4_vco_div2";
    case  0xf3: return "Pll_nvhs";
    case  0xf4: return "Pex2_c7_core_m";
    case  0xf5: return "Pex2_c8_core_m";
    case  0xf6: return "Pex2_c9_core_m";
    case  0xf7: return "Pex2_c10_core_m";
    case  0xf8: return "Mgbe0_rx_input";
    case  0xf9: return "Mgbe1_rx_input";
    case  0xfa: return "Mgbe2_rx_input";
    case  0xfb: return "Mgbe3_rx_input";
    case  0xfe: return "Pex_sata_usb_rx_byp";
    case  0xff: return "Pex_usb_pad_pll0_mgmt";
    case 0x100: return "Pex_usb_pad_pll1_mgmt";
    case 0x101: return "Pex_usb_pad_pll2_mgmt";
    case 0x102: return "Pex_usb_pad_pll3_mgmt";
    case 0x107: return "Nvhs_rx_byp_ref";
    case 0x108: return "Nvhs_pll0_mgmt";
    case 0x109: return "Xusb_core_dev";
    case 0x10a: return "Xusb_core_mux";
    case 0x10b: return "Xusb_core_host";
    case 0x10c: return "Xusb_core_ss";
    case 0x10d: return "Xusb_falcon";
    case 0x10e: return "Xusb_falcon_host";
    case 0x10f: return "Xusb_falcon_ss";
    case 0x110: return "Xusb_fs";
    case 0x111: return "Xusb_fs_host";
    case 0x112: return "Xusb_fs_dev";
    case 0x113: return "Xusb_ss";
    case 0x114: return "Xusb_ss_dev";
    case 0x115: return "Xusb_ss_superspeed";
    case 0x118: return "Nafll_cluster0_core";
    case 0x11a: return "Nafll_cluster2_core";
    case 0x11c: return "Can1_core";
    case 0x11d: return "Can2_core";
    case 0x11e: return "Plla1_out1";
    case 0x11f: return "Pllnvhs_hps";
    case 0x120: return "Pllrefe_vcoout";
    case 0x121: return "Clk_32k";
    case 0x123: return "Utmipll_clkout48";
    case 0x124: return "Utmipll_clkout480";
    case 0x126: return "Pll_nvcsi";
    case 0x12f: return "Aon_uart_fst_mipi_cal";
    case 0x130: return "Gpusysclk";
    case 0x132: return "Fr_se";
    case 0x13a: return "Pll_c";
    case 0x13b: return "Pll_c2";
    case 0x13e: return "Fuse_burn";
    case 0x13f: return "Pll_gbe";
    case 0x140: return "Pllgbe_hps";
    case 0x146: return "Plle_hps";
    case 0x147: return "Pllrefe_vcoout_gated";
    case 0x148: return "Pllp_div17";
    case 0x14b: return "Fr_seu1";
    case 0x14d: return "Nafll_ofa";
    case 0x14e: return "Ofa";
    case 0x14f: return "Nafll_seu1";
    case 0x150: return "Seu1";
    case 0x151: return "Spi4";
    case 0x152: return "Spi5";
    case 0x153: return "Dce_cpu_nic";
    case 0x154: return "Dce_nic";
    case 0x155: return "Nafll_dce";
    case 0x156: return "Mphy_l0_rx_ana_m";
    case 0x157: return "Mphy_l1_rx_ana_m";
    case 0x158: return "Mphy_l0_tx_pre_symb";
    case 0x159: return "Mphy_l0_tx_ls_symb_div";
    case 0x15a: return "Mphy_l0_tx_2x_symb";
    case 0x15b: return "Mphy_l0_tx_hs_symb_div";
    case 0x15c: return "Mphy_l0_tx_ls_3xbit_div";
    case 0x15d: return "Mphy_l0_tx_mux_symb_div";
    case 0x15e: return "Mphy_l0_tx_symb_m";
    case 0x15f: return "Mphy_l0_rx_ls_symb_div";
    case 0x160: return "Mphy_l0_rx_hs_symb_div";
    case 0x161: return "Mphy_l0_rx_ls_bit_div";
    case 0x162: return "Mphy_l0_rx_mux_symb_div";
    case 0x163: return "Mphy_l0_rx_symb_m";
    case 0x165: return "Mgbe0_rx_input_m";
    case 0x166: return "Mgbe1_rx_input_m";
    case 0x167: return "Mgbe2_rx_input_m";
    case 0x168: return "Mgbe3_rx_input_m";
    case 0x169: return "Mgbe0_rx_pcs_m";
    case 0x16a: return "Mgbe1_rx_pcs_m";
    case 0x16b: return "Mgbe2_rx_pcs_m";
    case 0x16c: return "Mgbe3_rx_pcs_m";
    case 0x16d: return "Tach1";
    case 0x16e: return "Mgbes_app";
    case 0x16f: return "Uphy_gbe_pll2_tx_ref";
    case 0x170: return "Uphy_gbe_pll2_xdig";
    case 0x171: return "Mgbe0_rx_pcs_input";
    case 0x172: return "Mgbe1_rx_pcs_input";
    case 0x173: return "Mgbe2_rx_pcs_input";
    case 0x174: return "Mgbe3_rx_pcs_input";
    case 0x175: return "Mgbe0_rx_pcs";
    case 0x176: return "Mgbe0_tx";
    case 0x177: return "Mgbe0_tx_pcs";
    case 0x178: return "Mgbe0_mac_divider";
    case 0x179: return "Mgbe0_mac";
    case 0x17a: return "Mgbe0_macsec";
    case 0x17b: return "Mgbe0_eee_pcs";
    case 0x17c: return "Mgbe0_app";
    case 0x17d: return "Mgbe0_ptp_ref";
    case 0x17e: return "Mgbe1_rx_pcs";
    case 0x17f: return "Mgbe1_tx";
    case 0x180: return "Mgbe1_tx_pcs";
    case 0x181: return "Mgbe1_mac_divider";
    case 0x182: return "Mgbe1_mac";
    case 0x183: return "Mgbe1_macsec";
    case 0x184: return "Mgbe1_eee_pcs";
    case 0x185: return "Mgbe1_app";
    case 0x186: return "Mgbe1_ptp_ref";
    case 0x187: return "Mgbe2_rx_pcs";
    case 0x188: return "Mgbe2_tx";
    case 0x189: return "Mgbe2_tx_pcs";
    case 0x18a: return "Mgbe2_mac_divider";
    case 0x18b: return "Mgbe2_mac";
    case 0x18c: return "Mgbe2_macsec";
    case 0x18d: return "Mgbe2_eee_pcs";
    case 0x18e: return "Mgbe2_app";
    case 0x18f: return "Mgbe2_ptp_ref";
    case 0x190: return "Mgbe3_rx_pcs";
    case 0x191: return "Mgbe3_tx";
    case 0x192: return "Mgbe3_tx_pcs";
    case 0x193: return "Mgbe3_mac_divider";
    case 0x194: return "Mgbe3_mac";
    case 0x195: return "Mgbe3_macsec";
    case 0x196: return "Mgbe3_eee_pcs";
    case 0x197: return "Mgbe3_app";
    case 0x198: return "Mgbe3_ptp_ref";
    case 0x199: return "Gbe_rx_byp_ref";
    case 0x19a: return "Gbe_pll0_mgmt";
    case 0x19b: return "Gbe_pll1_mgmt";
    case 0x19c: return "Gbe_pll2_mgmt";
    case 0x19d: return "Eqos_macsec_rx";
    case 0x19e: return "Eqos_macsec_tx";
    case 0x19f: return "Eqos_tx_divider";
    case 0x1a0: return "Nvhs_pll1_mgmt";
    case 0x1a1: return "Emchub";
    case 0x1a2: return "I2s7_sync_input";
    case 0x1a3: return "I2s7_sync_clk";
    case 0x1a4: return "I2s7";
    case 0x1a5: return "I2s7_pad_m";
    case 0x1a6: return "I2s8_sync_input";
    case 0x1a7: return "I2s8_sync_clk";
    case 0x1a8: return "I2s8";
    case 0x1a9: return "I2s8_pad_m";
    case 0x1aa: return "Nafll_gpc0";
    case 0x1ac: return "Nafll_gpusys";
    case 0x1ad: return "Nafll_cluster0_dsu";
    case 0x1af: return "Nafll_cluster2_dsu";
    case 0x1b0: return "Sce_cpu";
    case 0x1b1: return "Rce_cpu";
    case 0x1b2: return "Dce_cpu";
    case 0x1b3: return "Dsipll_vco";
    case 0x1b4: return "Dsipll_clkoutpn";
    case 0x1b5: return "Dsipll_clkouta";
    case 0x1b6: return "Sppll0_vco";
    case 0x1b7: return "Sppll0_clkoutpn";
    case 0x1b8: return "Sppll0_clkouta";
    case 0x1b9: return "Sppll0_clkoutb";
    case 0x1ba: return "Sppll0_div10";
    case 0x1bb: return "Sppll0_div25";
    case 0x1bc: return "Sppll0_div27pn";
    case 0x1bd: return "Sppll1_vco";
    case 0x1be: return "Sppll1_clkoutpn";
    case 0x1bf: return "Sppll1_div27pn";
    case 0x1c0: return "Vpll0_ref";
    case 0x1c1: return "Vpll0";
    case 0x1c2: return "Vpll1";
    case 0x1c3: return "Nvdisplay_p0_ref";
    case 0x1c4: return "Rg0";
    case 0x1c5: return "Rg1";
    case 0x1c6: return "Disppll";
    case 0x1c7: return "Disphubpll";
    case 0x1c8: return "Dsi_lp";
    case 0x1c9: return "Aza_2xbit";
    case 0x1ca: return "Aza_bit";
    case 0x1cb: return "Dsi_core";
    case 0x1cc: return "Dsi_pixel";
    case 0x1cd: return "Pre_sor0";
    case 0x1ce: return "Pre_sor1";
    case 0x1cf: return "Dp_link_ref";
    case 0x1d0: return "Sor_linka_input";
    case 0x1d1: return "Sor_linka_afifo";
    case 0x1d2: return "Sor_linka_afifo_m";
    case 0x1d3: return "Rg0_m";
    case 0x1d4: return "Rg1_m";
    case 0x1d5: return "Sor0_m";
    case 0x1d6: return "Sor1_m";
    case 0x1d7: return "Pll_hub";
    case 0x1d8: return "Mchub";
    default:    return "UNKNOWN";
    }
}


////////////////////////////////////////////////////////////////////////////////
// MRQ = 27 (thermal)
////////////////////////////////////////////////////////////////////////////////
char const *Tegra_bpmp_if::Mrq_thermal::Req::fn_to_str(unsigned fn)
{
  constexpr char const *const fn_str[5] =
  {
    "query abi", "get temperature", "set trip", "get num zones", "get thermtrip"
  };

  return fn < 5 ? fn_str[fn] : "\033[31mINVALID\033[m";
}


////////////////////////////////////////////////////////////////////////////////
// MRQ = 66 (power gates)
////////////////////////////////////////////////////////////////////////////////
char const *Tegra_bpmp_if::Mrq_pg::Req::fn_to_str(unsigned fn)
{
  constexpr char const *const fn_str[5] =
  {
    "query abi", "set state", "get state", "get name", "get max id"
  };

  return fn < 5 ? fn_str[fn] : "\033[31mINVALID\033[m";
}

char const *Tegra_bpmp_if::Mrq_pg::Req::id_to_str(unsigned pg_id)
{
  switch (pg_id)
    {
    case 0x01: return "Ofa";
    case 0x02: return "Aud";
    case 0x03: return "Disp";
    case 0x05: return "Pciex8a";
    case 0x06: return "Pciex4a";
    case 0x07: return "Pciex4ba";
    case 0x08: return "Pciex4bb";
    case 0x09: return "Pciex1a";
    case 0x0a: return "Xusba";
    case 0x0b: return "Xusbb";
    case 0x0c: return "Xusbc";
    case 0x0d: return "Pciex4ca";
    case 0x0e: return "Pciex4cb";
    case 0x0f: return "Pciex4cc";
    case 0x10: return "Pciex8b";
    case 0x11: return "Mgbea";
    case 0x12: return "Mgbeba";
    case 0x13: return "Mgbebb";
    case 0x14: return "Mgbebc";
    case 0x16: return "Ispa";
    case 0x17: return "Nvdec";
    case 0x18: return "Nvjpga";
    case 0x1c: return "Vi";
    case 0x1d: return "Vic";
    case 0x23: return "Gpu";
    default:   return "UNKNOWN";
    }
}


////////////////////////////////////////////////////////////////////////////////
// MRQ = 75 (debug)
////////////////////////////////////////////////////////////////////////////////
char const *Tegra_bpmp_if::Mrq_debug::Req::fn_to_str() const
{
  constexpr char const *const fn_str[5] =
  {
    "open ro", "open rw", "read", "write", "close"
  };

  return cmd.cmd < 5 ? fn_str[cmd.cmd] : "\033[31mINVALID\033[m";
}
