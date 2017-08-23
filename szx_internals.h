/* szx_internals.h: shared code needed only by the SZX code
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

#ifndef LIBSPECTRUM_SZX_INTERNALS_H
#define LIBSPECTRUM_SZX_INTERNALS_H

/* The machine numbers used in the .szx format */

typedef enum libspectrum_szx_machine_type {

  SZX_MACHINE_16 = 0,
  SZX_MACHINE_48,
  SZX_MACHINE_128,
  SZX_MACHINE_PLUS2,
  SZX_MACHINE_PLUS2A,
  SZX_MACHINE_PLUS3,
  SZX_MACHINE_PLUS3E,
  SZX_MACHINE_PENTAGON,
  SZX_MACHINE_TC2048,
  SZX_MACHINE_TC2068,
  SZX_MACHINE_SCORPION,
  SZX_MACHINE_SE,
  SZX_MACHINE_TS2068,
  SZX_MACHINE_PENTAGON512,
  SZX_MACHINE_PENTAGON1024,
  SZX_MACHINE_48_NTSC,
  SZX_MACHINE_128KE,

} libspectrum_szx_machine_type;

/* A mapping from an SZX machine IDs to a libspectrum machine IDs */
struct libspectrum_szx_machine_mapping_t {
  /* The constant used in the SZX format to identify this machine */
  libspectrum_szx_machine_type szx;

  /* The constant used in libspectrum to identify this machine */
  libspectrum_machine libspectrum;
};

/* The mappings from SZX to libspectrum machine IDs */
extern struct libspectrum_szx_machine_mapping_t libspectrum_szx_machine_mappings[];

/* The size of the above array */
extern size_t libspectrum_szx_machine_mappings_count;

/* The signature which identifies an SZX file */
extern const char * const libspectrum_szx_signature;

/* The length of the above signature */
extern const size_t libspectrum_szx_signature_length;

static const libspectrum_byte ZXSTMF_ALTERNATETIMINGS = 1;

/* How to (de)compose a flags field */
struct libspectrum_szx_flag_composition {
  /* The single bit */
  libspectrum_dword flag;

  /* The function to call to get the current value for this setting */
  int (*getter)( libspectrum_snap *snap );

  /* The function to call to set the current value for this setting */
  void (*setter)( libspectrum_snap *snap, int value );
};

/* Decompose a flags field, calling the appropriate setters */
/* Constants etc for each chunk type */

#define LIBSPECTRUM_ZXSTBID_MOUSE "AMXM"

#define LIBSPECTRUM_ZXSTBID_ZXATASPRAMPAGE "ATRP"

#define LIBSPECTRUM_ZXSTBID_AY "AY\0\0"
#define LIBSPECTRUM_ZXSTAYF_FULLERBOX 1
#define LIBSPECTRUM_ZXSTAYF_128AY 2
extern struct libspectrum_szx_flag_composition libspectrum_szx_ay_flags[];

#define LIBSPECTRUM_ZXSTBID_BETA128 "B128"
#define LIBSPECTRUM_ZXSTBETAF_CONNECTED 1
#define LIBSPECTRUM_ZXSTBETAF_CUSTOMROM 2
#define LIBSPECTRUM_ZXSTBETAF_PAGED 4
#define LIBSPECTRUM_ZXSTBETAF_AUTOBOOT 8
#define LIBSPECTRUM_ZXSTBETAF_SEEKLOWER 16
#define LIBSPECTRUM_ZXSTBETAF_COMPRESSED 32
extern struct libspectrum_szx_flag_composition libspectrum_szx_b128_flags[];

#define LIBSPECTRUM_ZXSTBID_BETADISK "BDSK"

#define LIBSPECTRUM_ZXSTBID_ZXCFRAMPAGE "CFRP"

#define LIBSPECTRUM_ZXSTBID_COVOX "COVX"

#define LIBSPECTRUM_ZXSTBID_CREATOR "CRTR"

#define LIBSPECTRUM_ZXSTBID_DIVIDE "DIDE"
#define LIBSPECTRUM_ZXSTDIVIDE_EPROM_WRITEPROTECT 1
#define LIBSPECTRUM_ZXSTDIVIDE_PAGED 2
#define LIBSPECTRUM_ZXSTDIVIDE_COMPRESSED 4
extern struct libspectrum_szx_flag_composition libspectrum_szx_dide_flags[];

#define LIBSPECTRUM_ZXSTBID_DIVIDERAMPAGE "DIRP"

#define LIBSPECTRUM_ZXSTBID_DOCK "DOCK"
extern const libspectrum_word LIBSPECTRUM_ZXSTDOCKF_RAM;
extern const libspectrum_word LIBSPECTRUM_ZXSTDOCKF_EXROMDOCK;

#define LIBSPECTRUM_ZXSTBID_DSKFILE "DSK\0"

#define LIBSPECTRUM_ZXSTBID_SPECDRUM "DRUM"

#define LIBSPECTRUM_ZXSTBID_GS "GS\0\0"

#define LIBSPECTRUM_ZXSTBID_GSRAMPAGE "GSRP"

#define LIBSPECTRUM_ZXSTBID_IF1 "IF1\0"
#define LIBSPECTRUM_ZXSTIF1F_ENABLED 1
#define LIBSPECTRUM_ZXSTIF1F_COMPRESSED 2
#define LIBSPECTRUM_ZXSTIF1F_PAGED 4
extern struct libspectrum_szx_flag_composition libspectrum_szx_if1_flags[];

#define LIBSPECTRUM_ZXSTBID_IF2ROM "IF2R"

#define LIBSPECTRUM_ZXSTBID_JOYSTICK "JOY\0"
static const libspectrum_dword ZXSTJOYF_ALWAYSPORT31 = 1;

#define LIBSPECTRUM_ZXSTBID_KEYBOARD "KEYB"
#define LIBSPECTRUM_ZXSTKF_ISSUE2 1
extern struct libspectrum_szx_flag_composition libspectrum_szx_keyb_flags[];

#define LIBSPECTRUM_ZXSTBID_LECRAMPAGE "LCRP"
extern const libspectrum_word ZXSTLCRPF_COMPRESSED;

#define LIBSPECTRUM_ZXSTBID_LEC "LEC\0"
extern const libspectrum_word ZXSTLECF_PAGED;

#define LIBSPECTRUM_ZXSTBID_MICRODRIVE "MDRV"

#define LIBSPECTRUM_ZXSTBID_MULTIFACE "MFCE"
#define LIBSPECTRUM_ZXSTMF_PAGEDIN 1
#define LIBSPECTRUM_ZXSTMF_COMPRESSED 2
#define LIBSPECTRUM_ZXSTMF_SOFTWARELOCKOUT 4
#define LIBSPECTRUM_ZXSTMF_REDBUTTONDISABLED 8
#define LIBSPECTRUM_ZXSTMF_DISABLED 16
#define LIBSPECTRUM_ZXSTMF_16KRAMMODE 32
extern struct libspectrum_szx_flag_composition libspectrum_szx_mfce_flags[];

extern const libspectrum_byte LIBSPECTRUM_ZXSTMFM_1;
extern const libspectrum_byte LIBSPECTRUM_ZXSTMFM_128;

#define LIBSPECTRUM_ZXSTBID_OPUSDISK "ODSK"

#define LIBSPECTRUM_ZXSTBID_OPUS "OPUS"
#define LIBSPECTRUM_ZXSTOPUSF_PAGED 1
#define LIBSPECTRUM_ZXSTOPUSF_COMPRESSED 2
#define LIBSPECTRUM_ZXSTOPUSF_SEEKLOWER 4
#define LIBSPECTRUM_ZXSTOPUSF_CUSTOMROM 8
extern struct libspectrum_szx_flag_composition libspectrum_szx_opus_flags[];

#define LIBSPECTRUM_ZXSTBID_PLUSD "PLSD"
#define LIBSPECTRUM_ZXSTPLUSDF_PAGED 1
#define LIBSPECTRUM_ZXSTPLUSDF_COMPRESSED 2
#define LIBSPECTRUM_ZXSTPLUSDF_SEEKLOWER 4
extern struct libspectrum_szx_flag_composition libspectrum_szx_plsd_flags[];

extern const libspectrum_byte LIBSPECTRUM_ZXSTPDRT_GDOS;
extern const libspectrum_byte LIBSPECTRUM_ZXSTPDRT_UNIDOS;
extern const libspectrum_byte LIBSPECTRUM_ZXSTPDRT_CUSTOM;

#define LIBSPECTRUM_ZXSTBID_PLUSDDISK "PDSK"

#define LIBSPECTRUM_ZXSTBID_PALETTE "PLTT"

#define LIBSPECTRUM_ZXSTBID_RAMPAGE "RAMP"
extern const libspectrum_word LIBSPECTRUM_ZXSTRF_COMPRESSED;

#define LIBSPECTRUM_ZXSTBID_ROM "ROM\0"

#define LIBSPECTRUM_ZXSTBID_TIMEXREGS "SCLD"

#define LIBSPECTRUM_ZXSTBID_SIMPLEIDE "SIDE"
extern const libspectrum_word LIBSPECTRUM_ZXSTSIDE_ENABLED;

#define LIBSPECTRUM_ZXSTBID_SPECTRANETFLASHPAGE "SNEF"
extern const libspectrum_byte LIBSPECTRUM_ZXSTSNEF_FLASH_COMPRESSED;

#define LIBSPECTRUM_ZXSTBID_SPECTRANETRAMPAGE "SNER"
extern const libspectrum_byte LIBSPECTRUM_ZXSTSNER_RAM_COMPRESSED;

#define LIBSPECTRUM_ZXSTBID_SPECTRANET "SNET"
#define LIBSPECTRUM_ZXSTSNET_PAGED 1
#define LIBSPECTRUM_ZXSTSNET_PAGED_VIA_IO 2
#define LIBSPECTRUM_ZXSTSNET_PROGRAMMABLE_TRAP_ACTIVE 4
#define LIBSPECTRUM_ZXSTSNET_PROGRAMMABLE_TRAP_MSB 8
#define LIBSPECTRUM_ZXSTSNET_ALL_DISABLED 16
#define LIBSPECTRUM_ZXSTSNET_RST8_DISABLED 32
#define LIBSPECTRUM_ZXSTSNET_DENY_DOWNSTREAM_A15 64
#define LIBSPECTRUM_ZXSTSNET_NMI_FLIPFLOP 128
extern struct libspectrum_szx_flag_composition libspectrum_szx_snet_flags[];

#define LIBSPECTRUM_ZXSTBID_SPECREGS "SPCR"

#define LIBSPECTRUM_ZXSTBID_ZXTAPE "TAPE"

#define LIBSPECTRUM_ZXSTBID_USPEECH "USPE"

#define LIBSPECTRUM_ZXSTBID_Z80REGS "Z80R"
extern const libspectrum_byte LIBSPECTRUM_ZXSTZF_EILAST;
extern const libspectrum_byte LIBSPECTRUM_ZXSTZF_HALTED;
extern const libspectrum_byte LIBSPECTRUM_ZXSTZF_FSET;

#define LIBSPECTRUM_ZXSTBID_ZXATASP "ZXAT"
#define LIBSPECTRUM_ZXSTZXATF_UPLOAD 1
#define LIBSPECTRUM_ZXSTZXATF_WRITEPROTECT 2
extern struct libspectrum_szx_flag_composition libspectrum_szx_zxat_flags[];

#define LIBSPECTRUM_ZXSTBID_ZXCF "ZXCF"
#define LIBSPECTRUM_ZXSTZXCFF_UPLOAD 1
extern struct libspectrum_szx_flag_composition libspectrum_szx_zxcf_flags[];

#define LIBSPECTRUM_ZXSTBID_ZXPRINTER "ZXPR"
#define LIBSPECTRUM_ZXSTPRF_ENABLED 1
extern struct libspectrum_szx_flag_composition libspectrum_szx_zxpr_flags[];

#define LIBSPECTRUM_ZXSTBID_PLUS3DISK "+3\0\0"

typedef enum libspectrum_szx_joystick_type {

  LIBSPECTRUM_ZXJT_KEMPSTON = 0,
  LIBSPECTRUM_ZXJT_FULLER,
  LIBSPECTRUM_ZXJT_CURSOR,
  LIBSPECTRUM_ZXJT_SINCLAIR1,
  LIBSPECTRUM_ZXJT_SINCLAIR2,
  LIBSPECTRUM_ZXJT_SPECTRUMPLUS,
  LIBSPECTRUM_ZXJT_TIMEX1,
  LIBSPECTRUM_ZXJT_TIMEX2,
  LIBSPECTRUM_ZXJT_NONE,

} libspectrum_szx_joystick_type;

typedef enum libspectrum_szx_mouse_type {

  LIBSPECTRUM_ZXSTM_NONE = 0,
  LIBSPECTRUM_ZXSTM_AMX,
  LIBSPECTRUM_ZXSTM_KEMPSTON,

} libspectrum_szx_mouse_type;

#endif				/* #ifndef LIBSPECTRUM_SZX_INTERNALS_H */
