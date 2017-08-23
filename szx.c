/* szx.c: SZX information shared between reading and writing code
   Copyright (c) 1998-2017 Philip Kendall, Fredrick Meunier, Stuart Brady

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <config.h>

#include <string.h>

#include "internals.h"
#include "szx_internals.h"

const char * const libspectrum_szx_signature = "ZXST";

const size_t libspectrum_szx_signature_length = 4;

struct libspectrum_szx_machine_mapping_t libspectrum_szx_machine_mappings[] = {
  { SZX_MACHINE_16, LIBSPECTRUM_MACHINE_16 },
  { SZX_MACHINE_48, LIBSPECTRUM_MACHINE_48 },
  { SZX_MACHINE_48_NTSC, LIBSPECTRUM_MACHINE_48_NTSC },
  { SZX_MACHINE_128, LIBSPECTRUM_MACHINE_128 },
  { SZX_MACHINE_PLUS2, LIBSPECTRUM_MACHINE_PLUS2 },
  { SZX_MACHINE_PLUS2A, LIBSPECTRUM_MACHINE_PLUS2A },
  { SZX_MACHINE_PLUS3, LIBSPECTRUM_MACHINE_PLUS3 },
  { SZX_MACHINE_PLUS3E, LIBSPECTRUM_MACHINE_PLUS3E },
  { SZX_MACHINE_PENTAGON, LIBSPECTRUM_MACHINE_PENT },
  { SZX_MACHINE_TC2048, LIBSPECTRUM_MACHINE_TC2048 },
  { SZX_MACHINE_TC2068, LIBSPECTRUM_MACHINE_TC2068 },
  { SZX_MACHINE_TS2068, LIBSPECTRUM_MACHINE_TS2068 },
  { SZX_MACHINE_SCORPION, LIBSPECTRUM_MACHINE_SCORP },
  { SZX_MACHINE_SE, LIBSPECTRUM_MACHINE_SE },
  { SZX_MACHINE_PENTAGON512, LIBSPECTRUM_MACHINE_PENT512 },
  { SZX_MACHINE_PENTAGON1024, LIBSPECTRUM_MACHINE_PENT1024 },
  { SZX_MACHINE_128KE, LIBSPECTRUM_MACHINE_128E },
};

size_t libspectrum_szx_machine_mappings_count =
  ARRAY_SIZE( libspectrum_szx_machine_mappings );

/* Helper macro to name the snap getter and setter */
#define GET_SET(x) libspectrum_snap_##x, libspectrum_snap_set_##x

/* AY chunk constants */
struct libspectrum_szx_flag_composition libspectrum_szx_ay_flags[] = {
  { LIBSPECTRUM_ZXSTAYF_FULLERBOX, GET_SET(fuller_box_active) },
  { LIBSPECTRUM_ZXSTAYF_128AY, GET_SET(melodik_active) },
  { 0, 0 }
};

/* B128 chunk constants */
static int
get_beta_direction_inverted( libspectrum_snap *snap )
{
  return libspectrum_snap_beta_direction( snap );
}

static void
set_beta_direction_inverted( libspectrum_snap *snap, int value )
{
  libspectrum_snap_set_beta_direction( snap, !value );
}

struct libspectrum_szx_flag_composition libspectrum_szx_b128_flags[] = {
  { LIBSPECTRUM_ZXSTBETAF_PAGED, GET_SET(beta_paged) },
  { LIBSPECTRUM_ZXSTBETAF_AUTOBOOT, GET_SET(beta_autoboot) },
  { LIBSPECTRUM_ZXSTBETAF_SEEKLOWER, get_beta_direction_inverted, set_beta_direction_inverted },
  { LIBSPECTRUM_ZXSTBETAF_CUSTOMROM, GET_SET(beta_custom_rom) },
  { 0, 0 }
};

/* DIDE chunk constants */
struct libspectrum_szx_flag_composition libspectrum_szx_dide_flags[] = {
  { LIBSPECTRUM_ZXSTDIVIDE_EPROM_WRITEPROTECT, GET_SET(divide_eprom_writeprotect) },
  { LIBSPECTRUM_ZXSTDIVIDE_PAGED, GET_SET(divide_paged) },
  { 0, 0 }
};

/* DOCK chunk constants */
const libspectrum_word LIBSPECTRUM_ZXSTDOCKF_RAM = 2;
const libspectrum_word LIBSPECTRUM_ZXSTDOCKF_EXROMDOCK = 4;

/* IF1 chunk constants */
struct libspectrum_szx_flag_composition libspectrum_szx_if1_flags[] = {
  { LIBSPECTRUM_ZXSTIF1F_ENABLED, GET_SET(interface1_active) },
  { LIBSPECTRUM_ZXSTIF1F_PAGED, GET_SET(interface1_paged) },
  { 0, 0 }
};

/* KEYB chunk constants */
struct libspectrum_szx_flag_composition libspectrum_szx_keyb_flags[] = {
  { LIBSPECTRUM_ZXSTKF_ISSUE2, GET_SET(issue2) },
  { 0, 0 }
};

/* LCRP chunk constants */
const libspectrum_word LIBSPECTRUM_ZXSTLCRPF_COMPRESSED = 1;

/* LEC chunk constants */
const libspectrum_word LIBSPECTRUM_ZXSTLECF_PAGED = 1;

/* MFCE chunk constants */
struct libspectrum_szx_flag_composition libspectrum_szx_mfce_flags[] = {
  { LIBSPECTRUM_ZXSTMF_PAGEDIN, GET_SET(multiface_paged) },
  { LIBSPECTRUM_ZXSTMF_SOFTWARELOCKOUT, GET_SET(multiface_software_lockout) },
  { LIBSPECTRUM_ZXSTMF_REDBUTTONDISABLED, GET_SET(multiface_red_button_disabled) },
  { LIBSPECTRUM_ZXSTMF_DISABLED, GET_SET(multiface_disabled) },
  { 0, 0, 0 }
};

const libspectrum_byte LIBSPECTRUM_ZXSTMFM_1 = 0;
const libspectrum_byte LIBSPECTRUM_ZXSTMFM_128 = 1;

/* OPUS chunk constants */
static int
get_opus_direction_inverted( libspectrum_snap *snap )
{
  return libspectrum_snap_opus_direction( snap );
}

static void
set_opus_direction_inverted( libspectrum_snap *snap, int value )
{
  libspectrum_snap_set_opus_direction( snap, !value );
}

struct libspectrum_szx_flag_composition libspectrum_szx_opus_flags[] = {
  { LIBSPECTRUM_ZXSTOPUSF_PAGED, GET_SET(opus_paged) },
  { LIBSPECTRUM_ZXSTOPUSF_SEEKLOWER, get_opus_direction_inverted, set_opus_direction_inverted },
  { LIBSPECTRUM_ZXSTOPUSF_CUSTOMROM, GET_SET(opus_custom_rom) },
  { 0, 0 }
};

/* PLSD chunk constants */
static int
get_plusd_direction_inverted( libspectrum_snap *snap )
{
  return !libspectrum_snap_plusd_direction( snap );
}

static void
set_plusd_direction_inverted( libspectrum_snap *snap, int value )
{
  libspectrum_snap_set_plusd_direction( snap, !value );
}

struct libspectrum_szx_flag_composition libspectrum_szx_plsd_flags[] = {
  { LIBSPECTRUM_ZXSTPLUSDF_PAGED, GET_SET(plusd_paged) },
  { LIBSPECTRUM_ZXSTPLUSDF_SEEKLOWER, get_plusd_direction_inverted, set_plusd_direction_inverted },
  { 0, 0 }
};

const libspectrum_byte LIBSPECTRUM_ZXSTPDRT_GDOS = 0;
const libspectrum_byte LIBSPECTRUM_ZXSTPDRT_UNIDOS = 1;
const libspectrum_byte LIBSPECTRUM_ZXSTPDRT_CUSTOM = 2;

/* RAMP chunk constants */
const libspectrum_word LIBSPECTRUM_ZXSTRF_COMPRESSED = 1;

/* SIDE chunk constants */
const libspectrum_word LIBSPECTRUM_ZXSTSIDE_ENABLED = 1;

/* SNEF chunk constants */
const libspectrum_byte LIBSPECTRUM_ZXSTSNEF_FLASH_COMPRESSED = 1;

/* SNER chunk constants */
const libspectrum_byte LIBSPECTRUM_ZXSTSNER_RAM_COMPRESSED = 1;

/* SNET chunk constants */
struct libspectrum_szx_flag_composition libspectrum_szx_snet_flags[] = {
  { LIBSPECTRUM_ZXSTSNET_PAGED, GET_SET(spectranet_paged) },
  { LIBSPECTRUM_ZXSTSNET_PAGED_VIA_IO, GET_SET(spectranet_paged_via_io) },
  { LIBSPECTRUM_ZXSTSNET_PROGRAMMABLE_TRAP_ACTIVE, GET_SET(spectranet_programmable_trap_active) },
  { LIBSPECTRUM_ZXSTSNET_PROGRAMMABLE_TRAP_MSB, GET_SET(spectranet_programmable_trap_msb) },
  { LIBSPECTRUM_ZXSTSNET_ALL_DISABLED, GET_SET(spectranet_all_traps_disabled) },
  { LIBSPECTRUM_ZXSTSNET_RST8_DISABLED, GET_SET(spectranet_rst8_trap_disabled) },
  { LIBSPECTRUM_ZXSTSNET_DENY_DOWNSTREAM_A15, GET_SET(spectranet_deny_downstream_a15) },
  { LIBSPECTRUM_ZXSTSNET_NMI_FLIPFLOP, GET_SET(spectranet_nmi_flipflop) },
  { 0, 0 }
};

/* Z80R chunk constants */
const libspectrum_byte LIBSPECTRUM_ZXSTZF_EILAST = 1;
const libspectrum_byte LIBSPECTRUM_ZXSTZF_HALTED = 2;
const libspectrum_byte LIBSPECTRUM_ZXSTZF_FSET = 4;

/* ZXAT chunk constants */
struct libspectrum_szx_flag_composition libspectrum_szx_zxat_flags[] = {
  { LIBSPECTRUM_ZXSTZXATF_UPLOAD, GET_SET(zxatasp_upload) },
  { LIBSPECTRUM_ZXSTZXATF_WRITEPROTECT, GET_SET(zxatasp_writeprotect) },
  { 0, 0 }
};

/* ZXCF chunk constants */
struct libspectrum_szx_flag_composition libspectrum_szx_zxcf_flags[] = {
  { LIBSPECTRUM_ZXSTZXCFF_UPLOAD, GET_SET(zxcf_upload) },
  { 0, 0 }
};

/* ZXPR chunk constants */
struct libspectrum_szx_flag_composition libspectrum_szx_zxpr_flags[] = {
  { LIBSPECTRUM_ZXSTPRF_ENABLED, GET_SET(zx_printer_active) },
  { 0, 0 }
};
