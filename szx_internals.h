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

/* Used for passing internal data around */

typedef struct szx_context {

  int swap_af;

} szx_context;

/* The machine numbers used in the .szx format */

typedef enum szx_machine_type {

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

} szx_machine_type;

static const char * const signature = "ZXST";
static const size_t signature_length = 4;

static const libspectrum_byte ZXSTMF_ALTERNATETIMINGS = 1;

static const char * const libspectrum_string = "libspectrum: ";

static const libspectrum_byte SZX_VERSION_MAJOR = 1;
static const libspectrum_byte SZX_VERSION_MINOR = 5;

/* Constants etc for each chunk type */

#define ZXSTBID_CREATOR "CRTR"

#define ZXSTBID_Z80REGS "Z80R"
static const libspectrum_byte ZXSTZF_EILAST = 1;
static const libspectrum_byte ZXSTZF_HALTED = 2;
static const libspectrum_byte ZXSTZF_FSET = 4;

#define ZXSTBID_SPECREGS "SPCR"

#define ZXSTBID_RAMPAGE "RAMP"
static const libspectrum_word ZXSTRF_COMPRESSED = 1;

#define ZXSTBID_AY "AY\0\0"
static const libspectrum_byte ZXSTAYF_FULLERBOX = 1;
static const libspectrum_byte ZXSTAYF_128AY = 2;

#define ZXSTBID_MULTIFACE "MFCE"
static const libspectrum_byte ZXSTMF_PAGEDIN = 1;
static const libspectrum_byte ZXSTMF_COMPRESSED = 2;
static const libspectrum_byte ZXSTMF_SOFTWARELOCKOUT = 4;
static const libspectrum_byte ZXSTMF_REDBUTTONDISABLED = 8;
static const libspectrum_byte ZXSTMF_DISABLED = 16;
static const libspectrum_byte ZXSTMF_16KRAMMODE = 32;
static const libspectrum_byte ZXSTMFM_1 = 0;
static const libspectrum_byte ZXSTMFM_128 = 1;

#define ZXSTBID_USPEECH "USPE"
#define ZXSTBID_SPECDRUM "DRUM"
#define ZXSTBID_ZXTAPE "TAPE"

#define ZXSTBID_KEYBOARD "KEYB"
static const libspectrum_dword ZXSTKF_ISSUE2 = 1;

#define ZXSTBID_JOYSTICK "JOY\0"
static const libspectrum_dword ZXSTJOYF_ALWAYSPORT31 = 1;

typedef enum szx_joystick_type {

  ZXJT_KEMPSTON = 0,
  ZXJT_FULLER,
  ZXJT_CURSOR,
  ZXJT_SINCLAIR1,
  ZXJT_SINCLAIR2,
  ZXJT_SPECTRUMPLUS,
  ZXJT_TIMEX1,
  ZXJT_TIMEX2,
  ZXJT_NONE,

} szx_joystick_type;

#define ZXSTBID_IF2ROM "IF2R"

#define ZXSTBID_MOUSE "AMXM"
typedef enum szx_mouse_type {

  ZXSTM_NONE = 0,
  ZXSTM_AMX,
  ZXSTM_KEMPSTON,

} szx_mouse_type;

#define ZXSTBID_ROM "ROM\0"

#define ZXSTBID_ZXPRINTER "ZXPR"
static const libspectrum_word ZXSTPRF_ENABLED = 1;

#define ZXSTBID_IF1 "IF1\0"
static const libspectrum_word ZXSTIF1F_ENABLED = 1;
static const libspectrum_word ZXSTIF1F_COMPRESSED = 2;
static const libspectrum_word ZXSTIF1F_PAGED = 4;

#define ZXSTBID_MICRODRIVE "MDRV"
#define ZXSTBID_PLUS3DISK "+3\0\0"
#define ZXSTBID_DSKFILE "DSK\0"
#define ZXSTBID_LEC "LEC\0"
/* static const libspectrum_word ZXSTLECF_PAGED = 1; */

#define ZXSTBID_LECRAMPAGE "LCRP"
/* static const libspectrum_word ZXSTLCRPF_COMPRESSED = 1; */

#define ZXSTBID_TIMEXREGS "SCLD"

#define ZXSTBID_BETA128 "B128"
static const libspectrum_dword ZXSTBETAF_CONNECTED = 1;
static const libspectrum_dword ZXSTBETAF_CUSTOMROM = 2;
static const libspectrum_dword ZXSTBETAF_PAGED = 4;
static const libspectrum_dword ZXSTBETAF_AUTOBOOT = 8;
static const libspectrum_dword ZXSTBETAF_SEEKLOWER = 16;
static const libspectrum_dword ZXSTBETAF_COMPRESSED = 32;

#define ZXSTBID_BETADISK "BDSK"
#define ZXSTBID_GS "GS\0\0"
#define ZXSTBID_GSRAMPAGE "GSRP"
#define ZXSTBID_COVOX "COVX"

#define ZXSTBID_DOCK "DOCK"
static const libspectrum_word ZXSTDOCKF_RAM = 2;
static const libspectrum_word ZXSTDOCKF_EXROMDOCK = 4;

#define ZXSTBID_ZXATASP "ZXAT"
static const libspectrum_word ZXSTZXATF_UPLOAD = 1;
static const libspectrum_word ZXSTZXATF_WRITEPROTECT = 2;

#define ZXSTBID_ZXATASPRAMPAGE "ATRP"

#define ZXSTBID_ZXCF "ZXCF"
static const libspectrum_word ZXSTZXCFF_UPLOAD = 1;

#define ZXSTBID_ZXCFRAMPAGE "CFRP"

#define ZXSTBID_PLUSD "PLSD"
static const libspectrum_dword ZXSTPLUSDF_PAGED = 1;
static const libspectrum_dword ZXSTPLUSDF_COMPRESSED = 2;
static const libspectrum_dword ZXSTPLUSDF_SEEKLOWER = 4;
static const libspectrum_byte ZXSTPDRT_GDOS = 0;
/* static const libspectrum_byte ZXSTPDRT_UNIDOS = 1; */
static const libspectrum_byte ZXSTPDRT_CUSTOM = 2;

#define ZXSTBID_PLUSDDISK "PDSK"

#define ZXSTBID_OPUS "OPUS"
static const libspectrum_dword ZXSTOPUSF_PAGED = 1;
static const libspectrum_dword ZXSTOPUSF_COMPRESSED = 2;
static const libspectrum_dword ZXSTOPUSF_SEEKLOWER = 4;
static const libspectrum_dword ZXSTOPUSF_CUSTOMROM = 8;

#define ZXSTBID_OPUSDISK "ODSK"

#define ZXSTBID_SIMPLEIDE "SIDE"
/* static const libspectrum_word ZXSTSIDE_ENABLED = 1; */

#define ZXSTBID_DIVIDE "DIDE"
static const libspectrum_word ZXSTDIVIDE_EPROM_WRITEPROTECT = 1;
static const libspectrum_word ZXSTDIVIDE_PAGED = 2;
static const libspectrum_word ZXSTDIVIDE_COMPRESSED = 4;

#define ZXSTBID_DIVIDERAMPAGE "DIRP"

#define ZXSTBID_SPECTRANET "SNET"
static const libspectrum_word ZXSTSNET_PAGED = 1;
static const libspectrum_word ZXSTSNET_PAGED_VIA_IO = 2;
static const libspectrum_word ZXSTSNET_PROGRAMMABLE_TRAP_ACTIVE = 4;
static const libspectrum_word ZXSTSNET_PROGRAMMABLE_TRAP_MSB = 8;
static const libspectrum_word ZXSTSNET_ALL_DISABLED = 16;
static const libspectrum_word ZXSTSNET_RST8_DISABLED = 32;
static const libspectrum_word ZXSTSNET_DENY_DOWNSTREAM_A15 = 64;
static const libspectrum_word ZXSTSNET_NMI_FLIPFLOP = 128;

#define ZXSTBID_SPECTRANETFLASHPAGE "SNEF"
static const libspectrum_byte ZXSTSNEF_FLASH_COMPRESSED = 1;

#define ZXSTBID_SPECTRANETRAMPAGE "SNER"
static const libspectrum_byte ZXSTSNER_RAM_COMPRESSED = 1;

#define ZXSTBID_PALETTE "PLTT"

#endif				/* #ifndef LIBSPECTRUM_SZX_INTERNALS_H */
