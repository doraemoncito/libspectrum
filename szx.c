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

/* DOCK chunk constants */
const libspectrum_word LIBSPECTRUM_ZXSTDOCKF_RAM = 2;
const libspectrum_word LIBSPECTRUM_ZXSTDOCKF_EXROMDOCK = 4;

/* LCRP chunk constants */
const libspectrum_word LIBSPECTRUM_ZXSTLCRPF_COMPRESSED = 1;

/* LEC chunk constants */
const libspectrum_word LIBSPECTRUM_ZXSTLECF_PAGED = 1;

/* MFCE chunk constants */
const libspectrum_byte LIBSPECTRUM_ZXSTMFM_1 = 0;
const libspectrum_byte LIBSPECTRUM_ZXSTMFM_128 = 1;

/* PLSD chunk constants */
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

/* Z80R chunk constants */
const libspectrum_byte LIBSPECTRUM_ZXSTZF_EILAST = 1;
const libspectrum_byte LIBSPECTRUM_ZXSTZF_HALTED = 2;
const libspectrum_byte LIBSPECTRUM_ZXSTZF_FSET = 4;
