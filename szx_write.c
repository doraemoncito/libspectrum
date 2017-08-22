/* szx_write.c: Routines for writing .szx snapshots
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

/* The major version number we will write */
static const libspectrum_byte SZX_VERSION_MAJOR = 1;

/* The minor version number we will write */
static const libspectrum_byte SZX_VERSION_MINOR = 5;

static libspectrum_error
write_file_header( libspectrum_buffer *buffer, int *out_flags,
                   libspectrum_snap *snap );

static void
write_crtr_chunk( libspectrum_buffer *buffer, libspectrum_buffer *crtr_data,
                  libspectrum_creator *creator );
static void
write_z80r_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );
static void
write_spcr_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );
static void
write_amxm_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );
static void
write_joy_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                 int *out_flags, libspectrum_snap *snap );
static void
write_keyb_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  int *out_flags, libspectrum_snap *snap );
static void
write_ram_pages( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                 libspectrum_snap *snap, int compress );
static void
write_ramp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress );
static void
write_ram_page( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                const char *id, const libspectrum_byte *data,
                size_t data_length, int page, int compress, int extra_flags );
static libspectrum_error
write_rom_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                 int *out_flags, libspectrum_snap *snap, int compress );
static void
write_ay_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                libspectrum_snap *snap );
static void
write_scld_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );
static void
write_b128_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress );
static libspectrum_error
write_opus_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress  );
static libspectrum_error
write_plsd_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress  );
static libspectrum_error
write_if1_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		 libspectrum_snap *snap, int compress  );
static void
write_zxat_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );
static void
write_atrp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
		  libspectrum_snap *snap, int page, int compress );
static void
write_zxcf_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap );
static void
write_cfrp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress );
static void
write_side_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
		  libspectrum_snap *snap );
static void
write_drum_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );
static void
write_snet_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress );
static void
write_snef_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress );
static void
write_sner_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress );
static libspectrum_error
write_mfce_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress );

#ifdef HAVE_ZLIB_H

static libspectrum_error
write_if2r_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap );

#endif				/* #ifdef HAVE_ZLIB_H */

static void
write_dock_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
		  libspectrum_snap *snap, int exrom_dock,
                  const libspectrum_byte *data, int page, int writeable,
                  int compress );
static libspectrum_error
write_dide_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress );
static void
write_dirp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress );
static void
write_zxpr_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  int *out_flags, libspectrum_snap *snap );
static void
write_covx_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap );

static void
write_chunk( libspectrum_buffer *buffer, const char *id,
             libspectrum_buffer *block_data );

static int
compress_data( libspectrum_buffer *dest, const libspectrum_byte *src_data,
               size_t src_data_length, int compress );

libspectrum_error
libspectrum_szx_write( libspectrum_buffer *buffer, int *out_flags,
                       libspectrum_snap *snap, libspectrum_creator *creator,
                       int in_flags )
{
  int capabilities, compress;
  libspectrum_error error;
  size_t i;
  libspectrum_buffer *block_data;

  *out_flags = 0;

  /* We don't save the uSource state at all */
  if( libspectrum_snap_usource_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the DISCiPLE state at all */
  if( libspectrum_snap_disciple_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  /* We don't save the Didaktik80 state at all */
  if( libspectrum_snap_didaktik80_active( snap ) )
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;

  capabilities =
    libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );

  compress = !( in_flags & LIBSPECTRUM_FLAG_SNAPSHOT_NO_COMPRESSION );

  error = write_file_header( buffer, out_flags, snap );
  if( error ) return error;

  block_data = libspectrum_buffer_alloc();

  if( creator ) {
    write_crtr_chunk( buffer, block_data, creator );
  }

  write_z80r_chunk( buffer, block_data, snap );
  write_spcr_chunk( buffer, block_data, snap );
  write_joy_chunk( buffer, block_data, out_flags, snap );
  write_keyb_chunk( buffer, block_data, out_flags, snap );

  if( libspectrum_snap_custom_rom( snap ) ) {
    error = write_rom_chunk( buffer, block_data, out_flags, snap, compress );
    if( error ) {
      libspectrum_buffer_free( block_data );
      return error;
    }
  }

  write_ram_pages( buffer, block_data, snap, compress );

  if( libspectrum_snap_fuller_box_active( snap ) ||
      libspectrum_snap_melodik_active( snap ) ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_AY ) {
    write_ay_chunk( buffer, block_data, snap );
  }

  if( capabilities & ( LIBSPECTRUM_MACHINE_CAPABILITY_TIMEX_MEMORY |
                       LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY ) ) {
    write_scld_chunk( buffer, block_data, snap );
  }

  if( libspectrum_snap_beta_active( snap ) ) {
    write_b128_chunk( buffer, block_data, snap, compress );
  }

  if( libspectrum_snap_zxatasp_active( snap ) ) {
    write_zxat_chunk( buffer, block_data, snap );

    for( i = 0; i < libspectrum_snap_zxatasp_pages( snap ); i++ ) {
      write_atrp_chunk( buffer, block_data, snap, i, compress );
    }
  }

  if( libspectrum_snap_zxcf_active( snap ) ) {
    write_zxcf_chunk( buffer, block_data, snap );

    for( i = 0; i < libspectrum_snap_zxcf_pages( snap ); i++ ) {
      write_cfrp_chunk( buffer, block_data, snap, i, compress );
    }
  }

  if( libspectrum_snap_interface2_active( snap ) ) {
#ifdef HAVE_ZLIB_H
    error = write_if2r_chunk( buffer, block_data, snap );
    if( error ) {
      libspectrum_buffer_free( block_data );
      return error;
    }
#else
    /* IF2R blocks only support writing compressed images */
    *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
#endif                         /* #ifdef HAVE_ZLIB_H */
  }

  if( libspectrum_snap_dock_active( snap ) ) {
    for( i = 0; i < 8; i++ ) {
      if( libspectrum_snap_exrom_cart( snap, i ) ) {
        write_dock_chunk( buffer, block_data, snap, 0,
                          libspectrum_snap_exrom_cart( snap, i ), i,
                          libspectrum_snap_exrom_ram( snap, i ),
                          compress );
      }
      if( libspectrum_snap_dock_cart( snap, i ) ) {
        write_dock_chunk( buffer, block_data, snap, 1,
                          libspectrum_snap_dock_cart( snap, i ), i,
                          libspectrum_snap_dock_ram( snap, i ),
                          compress );
      }
    }
  }

  if( libspectrum_snap_interface1_active( snap ) ) {
    error = write_if1_chunk( buffer, block_data, snap, compress );
    if( error ) {
      libspectrum_buffer_free( block_data );
      return error;
    }
  }

  if( libspectrum_snap_opus_active( snap ) ) {
    error = write_opus_chunk( buffer, block_data, snap, compress );
    if( error ) {
      libspectrum_buffer_free( block_data );
      return error;
    }
  }

  if( libspectrum_snap_plusd_active( snap ) ) {
    error = write_plsd_chunk( buffer, block_data, snap, compress );
    if( error ) {
      libspectrum_buffer_free( block_data );
      return error;
    }
  }

  if( libspectrum_snap_kempston_mouse_active( snap ) ) {
    write_amxm_chunk( buffer, block_data, snap );
  }

  if( libspectrum_snap_simpleide_active( snap ) ) {
    write_side_chunk( buffer, block_data, snap );
  }

  if( libspectrum_snap_specdrum_active( snap ) ) {
    write_drum_chunk( buffer, block_data, snap );
  }

  if( libspectrum_snap_divide_active( snap ) ) {
    error = write_dide_chunk( buffer, block_data, snap, compress );
    if( error ) {
      libspectrum_buffer_free( block_data );
      return error;
    }

    for( i = 0; i < libspectrum_snap_divide_pages( snap ); i++ ) {
      write_dirp_chunk( buffer, block_data, snap, i, compress );
    }
  }

  if( libspectrum_snap_spectranet_active( snap ) ) {
    write_snet_chunk( buffer, block_data, snap, compress );
    write_snef_chunk( buffer, block_data, snap, compress );
    write_sner_chunk( buffer, block_data, snap, compress );
  }

  write_zxpr_chunk( buffer, block_data, out_flags, snap );

  if( libspectrum_snap_covox_active( snap ) ) {
    write_covx_chunk( buffer, block_data, snap );
  }

  if( libspectrum_snap_multiface_active( snap ) ) {
    error = write_mfce_chunk( buffer, block_data, snap, compress );
    if( error ) return error;
  }

  libspectrum_buffer_free( block_data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_file_header( libspectrum_buffer *buffer, int *out_flags,
                   libspectrum_snap *snap )
{
  libspectrum_byte flags;
  size_t i;
  int done = 0;
  libspectrum_machine machine;

  libspectrum_buffer_write( buffer, libspectrum_szx_signature, libspectrum_szx_signature_length );
  
  libspectrum_buffer_write_byte( buffer, SZX_VERSION_MAJOR );
  libspectrum_buffer_write_byte( buffer, SZX_VERSION_MINOR );

  machine = libspectrum_snap_machine( snap );
  for( i = 0; !done && i < libspectrum_szx_machine_mappings_count; i++ ) {
    if( machine == libspectrum_szx_machine_mappings[i].libspectrum ) {
      flags = libspectrum_szx_machine_mappings[i].szx;
      done = 1;
    }
  }

  if( !done ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "Emulated machine type is set to 'unknown'!" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  libspectrum_buffer_write_byte( buffer, flags );

  /* Flags byte */
  flags = 0;
  if( libspectrum_snap_late_timings( snap ) ) flags |= ZXSTMF_ALTERNATETIMINGS;
  libspectrum_buffer_write_byte( buffer, flags );

  return LIBSPECTRUM_ERROR_NONE;
}

static void
write_crtr_chunk( libspectrum_buffer *buffer, libspectrum_buffer *crtr_data,
                  libspectrum_creator *creator )
{
  size_t custom_length;

  libspectrum_buffer_write( crtr_data, libspectrum_creator_program( creator ),
                            32 );
  libspectrum_buffer_write_word( crtr_data, libspectrum_creator_major( creator ) );
  libspectrum_buffer_write_word( crtr_data, libspectrum_creator_minor( creator ) );

  custom_length = libspectrum_creator_custom_length( creator );
  if( custom_length ) {
    libspectrum_buffer_write( crtr_data, libspectrum_creator_custom( creator ),
                              custom_length );
  }

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_CREATOR, crtr_data );
}

static void
write_z80r_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  libspectrum_dword tstates;
  libspectrum_byte flags, tstates_remaining;

  libspectrum_buffer_write_byte( data, libspectrum_snap_f   ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_a   ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_bc  ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_de  ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_hl  ( snap ) );

  libspectrum_buffer_write_byte( data, libspectrum_snap_f_  ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_a_  ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_bc_ ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_de_ ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_hl_ ( snap ) );

  libspectrum_buffer_write_word( data, libspectrum_snap_ix  ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_iy  ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_sp  ( snap ) );
  libspectrum_buffer_write_word( data, libspectrum_snap_pc  ( snap ) );

  libspectrum_buffer_write_byte( data, libspectrum_snap_i   ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_r   ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_iff1( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_iff2( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_im  ( snap ) );

  tstates = libspectrum_snap_tstates( snap );

  libspectrum_buffer_write_dword( data, tstates );

  /* Number of tstates remaining in which an interrupt can occur */
  if( tstates < 48 ) {
    tstates_remaining = (unsigned char)(48 - tstates);
  } else {
    tstates_remaining = '\0';
  }
  libspectrum_buffer_write_byte( data, tstates_remaining );

  flags = '\0';
  if( libspectrum_snap_last_instruction_ei( snap ) ) flags |= LIBSPECTRUM_ZXSTZF_EILAST;
  if( libspectrum_snap_halted( snap ) ) flags |= LIBSPECTRUM_ZXSTZF_HALTED;
  if( libspectrum_snap_last_instruction_set_f( snap ) ) flags |= LIBSPECTRUM_ZXSTZF_FSET;
  libspectrum_buffer_write_byte( data, flags );

  libspectrum_buffer_write_word( data, libspectrum_snap_memptr( snap ) );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_Z80REGS, data );
}

static void
write_spcr_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  int capabilities;

  capabilities =
    libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );

  /* Border colour */
  libspectrum_buffer_write_byte( data,
                                 libspectrum_snap_out_ula( snap ) & 0x07 );

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) {
    libspectrum_buffer_write_byte( data,
                                 libspectrum_snap_out_128_memoryport( snap ) );
  } else {
    libspectrum_buffer_write_byte( data, '\0' );
  }
  
  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY    || 
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY    ||
      capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PENT1024_MEMORY    ) {
    libspectrum_buffer_write_byte( data,
                               libspectrum_snap_out_plus3_memoryport( snap ) );
  } else {
    libspectrum_buffer_write_byte( data, '\0' );
  }

  libspectrum_buffer_write_byte( data, libspectrum_snap_out_ula( snap ) );

  /* Reserved bytes */
  libspectrum_buffer_write_dword( data, 0 );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_SPECREGS, data );
}

static void
write_joystick( libspectrum_buffer *buffer, int *out_flags,
                libspectrum_snap *snap, const int connection )
{
  size_t num_joysticks = libspectrum_snap_joystick_active_count( snap );
  int found = 0;
  int i;
  libspectrum_byte type = LIBSPECTRUM_ZXJT_NONE;

  for( i = 0; i < num_joysticks; i++ ) {
    if( libspectrum_snap_joystick_inputs( snap, i ) & connection ) {
      switch( libspectrum_snap_joystick_list( snap, i ) ) {
      case LIBSPECTRUM_JOYSTICK_CURSOR:
        if( !found ) { type = LIBSPECTRUM_ZXJT_CURSOR; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_KEMPSTON:
        if( !found ) { type = LIBSPECTRUM_ZXJT_KEMPSTON; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_SINCLAIR_1:
        if( !found ) { type = LIBSPECTRUM_ZXJT_SINCLAIR1; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_SINCLAIR_2:
        if( !found ) { type = LIBSPECTRUM_ZXJT_SINCLAIR2; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_TIMEX_1:
        if( !found ) { type = LIBSPECTRUM_ZXJT_TIMEX1; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_TIMEX_2:
        if( !found ) { type = LIBSPECTRUM_ZXJT_TIMEX2; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_FULLER:
        if( !found ) { type = LIBSPECTRUM_ZXJT_FULLER; found = 1; }
        else *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MINOR_INFO_LOSS;
        break;
      case LIBSPECTRUM_JOYSTICK_NONE: /* Shouldn't happen */
      default:
        type = LIBSPECTRUM_ZXJT_NONE;
        break;
      }
    }
  }

  libspectrum_buffer_write_byte( buffer, type );
}

static void
write_joy_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                 int *out_flags, libspectrum_snap *snap )
{
  libspectrum_dword flags;
  size_t num_joysticks = libspectrum_snap_joystick_active_count( snap );
  int i;

  flags = 0;
  for( i = 0; i < num_joysticks; i++ ) {
    if( libspectrum_snap_joystick_list( snap, i ) ==
        LIBSPECTRUM_JOYSTICK_KEMPSTON )
      flags |= ZXSTJOYF_ALWAYSPORT31;
  }
  libspectrum_buffer_write_dword( data, flags );

  write_joystick( data, out_flags, snap, LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
  write_joystick( data, out_flags, snap, LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_JOYSTICK, data );
}

static void
write_amxm_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  if( libspectrum_snap_kempston_mouse_active( snap ) )
    libspectrum_buffer_write_byte( data, LIBSPECTRUM_ZXSTM_KEMPSTON );
  else
    libspectrum_buffer_write_byte( data, LIBSPECTRUM_ZXSTM_NONE );

  /* Z80 PIO CTRLA registers for AMX mouse */
  libspectrum_buffer_write_byte( data, '\0' );
  libspectrum_buffer_write_byte( data, '\0' );
  libspectrum_buffer_write_byte( data, '\0' );

  /* Z80 PIO CTRLB registers for AMX mouse */
  libspectrum_buffer_write_byte( data, '\0' );
  libspectrum_buffer_write_byte( data, '\0' );
  libspectrum_buffer_write_byte( data, '\0' );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_MOUSE, data );
}

static void
write_keyb_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  int *out_flags, libspectrum_snap *snap )
{
  libspectrum_dword flags;

  flags = 0;
  if( libspectrum_snap_issue2( snap ) ) flags |= LIBSPECTRUM_ZXSTKF_ISSUE2;

  libspectrum_buffer_write_dword( data, flags );

  write_joystick( data, out_flags, snap, LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_KEYBOARD, data );
}
  
static void
write_zxpr_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  int *out_flags, libspectrum_snap *snap )
{
  libspectrum_word flags;

  flags = 0;
  if( libspectrum_snap_zx_printer_active( snap ) ) flags |= LIBSPECTRUM_ZXSTPRF_ENABLED;

  libspectrum_buffer_write_word( data, flags );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_ZXPRINTER, data );
}
  
static libspectrum_error
write_rom_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                 int *out_flags, libspectrum_snap *snap, int compress )
{
  size_t i, data_length = 0;
  libspectrum_buffer *data, *rom_buffer;
  int flags = 0;
  int use_compression;

  for( i = 0; i< libspectrum_snap_custom_rom_pages( snap ); i++ ) {
    data_length += libspectrum_snap_rom_length( snap, i );
  }

  /* Check that we have the expected number of ROMs per the machine type */
  switch( libspectrum_snap_machine( snap ) ) {

  case LIBSPECTRUM_MACHINE_16:
  case LIBSPECTRUM_MACHINE_48:
  case LIBSPECTRUM_MACHINE_48_NTSC:
  case LIBSPECTRUM_MACHINE_TC2048:
    /* 1 ROM = 16k */
    if( ( libspectrum_snap_custom_rom_pages( snap ) != 1 ||
          data_length != 0x4000 ) ) {
      *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
      return LIBSPECTRUM_ERROR_NONE;
    }
    break;
  case LIBSPECTRUM_MACHINE_128:
  case LIBSPECTRUM_MACHINE_128E:
  case LIBSPECTRUM_MACHINE_PENT:
  case LIBSPECTRUM_MACHINE_PLUS2:
  case LIBSPECTRUM_MACHINE_SE:
    /* 2 ROMs = 32k */
    if( ( libspectrum_snap_custom_rom_pages( snap ) != 2 ||
          data_length != 0x8000 ) ) {
      *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
      return LIBSPECTRUM_ERROR_NONE;
    }
    break;
  case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
  case LIBSPECTRUM_MACHINE_PLUS3E:
    /* 4 ROMs = 64k */
    if( ( libspectrum_snap_custom_rom_pages( snap ) != 4 ||
          data_length != 0x10000 ) ) {
      *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
      return LIBSPECTRUM_ERROR_NONE;
    }
    break;
  case LIBSPECTRUM_MACHINE_PENT512:
  case LIBSPECTRUM_MACHINE_PENT1024:
    /* 3 ROMs = 48k */
    if( ( libspectrum_snap_custom_rom_pages( snap ) != 3 ||
          data_length != 0xc000 ) ) {
      *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
      return LIBSPECTRUM_ERROR_NONE;
    }
    break;
  case LIBSPECTRUM_MACHINE_TC2068:
  case LIBSPECTRUM_MACHINE_TS2068:
    /* 2 ROMs = 24k */
    if( ( libspectrum_snap_custom_rom_pages( snap ) != 2 ||
          data_length != 0x6000 ) ) {
      *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
      return LIBSPECTRUM_ERROR_NONE;
    }
    break;
  case LIBSPECTRUM_MACHINE_SCORP:
    /* 4 ROMs = 64k */
    if( ( libspectrum_snap_custom_rom_pages( snap ) != 4 ||
          data_length != 0x10000 ) ) {
      *out_flags |= LIBSPECTRUM_FLAG_SNAPSHOT_MAJOR_INFO_LOSS;
      return LIBSPECTRUM_ERROR_NONE;
    }
    break;

  case LIBSPECTRUM_MACHINE_UNKNOWN:
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "Emulated machine type is set to 'unknown'!" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  data = libspectrum_buffer_alloc();

  /* Copy the rom data into a single block ready for putting in the szx */
  for( i = 0; i< libspectrum_snap_custom_rom_pages( snap ); i++ ) {
    libspectrum_buffer_write( data, libspectrum_snap_roms( snap, i ),
                              libspectrum_snap_rom_length( snap, i ) );
  }

  rom_buffer = libspectrum_buffer_alloc();
  use_compression = compress_data( rom_buffer,
                                   libspectrum_buffer_get_data( data ),
                                   libspectrum_buffer_get_data_size( data ),
                                   compress );

  if( use_compression ) flags |= LIBSPECTRUM_ZXSTRF_COMPRESSED;
  libspectrum_buffer_write_word( block_data, flags );
  libspectrum_buffer_write_dword( block_data,
                                  libspectrum_buffer_get_data_size( data ) );

  libspectrum_buffer_write_buffer( block_data, rom_buffer );

  libspectrum_buffer_free( data );
  libspectrum_buffer_free( rom_buffer );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_ROM, block_data );

  return LIBSPECTRUM_ERROR_NONE;
}

static void
write_ram_pages( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                 libspectrum_snap *snap, int compress )
{
  libspectrum_machine machine;
  int i, capabilities; 

  machine = libspectrum_snap_machine( snap );
  capabilities = libspectrum_machine_capabilities( machine );

  write_ramp_chunk( buffer, block_data, snap, 5, compress );

  if( machine != LIBSPECTRUM_MACHINE_16 ) {
    write_ramp_chunk( buffer, block_data, snap, 2, compress );
    write_ramp_chunk( buffer, block_data, snap, 0, compress );
  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_128_MEMORY ) {
    write_ramp_chunk( buffer, block_data, snap, 1, compress );
    write_ramp_chunk( buffer, block_data, snap, 3, compress );
    write_ramp_chunk( buffer, block_data, snap, 4, compress );
    write_ramp_chunk( buffer, block_data, snap, 6, compress );
    write_ramp_chunk( buffer, block_data, snap, 7, compress );

    if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY ) {
      for( i = 8; i < 16; i++ ) {
        write_ramp_chunk( buffer, block_data, snap, i, compress );
      }
    } else if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PENT512_MEMORY ) {
      for( i = 8; i < 32; i++ ) {
        write_ramp_chunk( buffer, block_data, snap, i, compress );
      }

      if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PENT1024_MEMORY ) {
	for( i = 32; i < 64; i++ ) {
	  write_ramp_chunk( buffer, block_data, snap, i, compress );
	}
      }
    }

  }

  if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SE_MEMORY ) {
    write_ramp_chunk( buffer, block_data, snap, 8, compress );
  }
}

static void
write_ramp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress )
{
  const libspectrum_byte *data = libspectrum_snap_pages( snap, page );

  write_ram_page( buffer, block_data, LIBSPECTRUM_ZXSTBID_RAMPAGE, data, 0x4000, page,
                  compress, 0x00 );
}

static void
write_ram_page( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                const char *id, const libspectrum_byte *data,
                size_t data_length, int page, int compress, int extra_flags )
{
  libspectrum_buffer *data_buffer;
  int use_compression;

  if( !data ) return;

  data_buffer = libspectrum_buffer_alloc();
  use_compression = compress_data( data_buffer, data, data_length, compress );

  if( use_compression ) extra_flags |= LIBSPECTRUM_ZXSTRF_COMPRESSED;

  libspectrum_buffer_write_word( block_data, extra_flags );

  libspectrum_buffer_write_byte( block_data, (libspectrum_byte)page );

  libspectrum_buffer_write_buffer( block_data, data_buffer );

  libspectrum_buffer_free( data_buffer );

  write_chunk( buffer, id, block_data );
}

static void
write_ay_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                libspectrum_snap *snap )
{
  size_t i;
  libspectrum_byte flags;

  flags = 0;
  if( libspectrum_snap_fuller_box_active( snap ) ) flags |= LIBSPECTRUM_ZXSTAYF_FULLERBOX;
  if( libspectrum_snap_melodik_active( snap ) ) flags |= LIBSPECTRUM_ZXSTAYF_128AY;
  libspectrum_buffer_write_byte( data, flags );

  libspectrum_buffer_write_byte( data,
                                 libspectrum_snap_out_ay_registerport( snap ) );

  for( i = 0; i < 16; i++ )
    libspectrum_buffer_write_byte( data,
                                   libspectrum_snap_ay_registers( snap, i ) );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_AY, data );
}

static void
write_scld_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  libspectrum_buffer_write_byte( data, libspectrum_snap_out_scld_hsr( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_out_scld_dec( snap ) );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_TIMEXREGS, data );
}

static void
write_b128_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress )
{
  libspectrum_byte *rom_data = NULL;
  libspectrum_buffer *rom_buffer = NULL;
  libspectrum_word beta_rom_length = 0;
  libspectrum_dword flags;
  int use_compression = 0;

  if( libspectrum_snap_beta_custom_rom( snap ) ) {
    rom_data = libspectrum_snap_beta_rom( snap, 0 );
    beta_rom_length = 0x4000;

    rom_buffer = libspectrum_buffer_alloc();
    use_compression = compress_data( rom_buffer, rom_data,
                                     beta_rom_length, compress );
  }

  flags = LIBSPECTRUM_ZXSTBETAF_CONNECTED;	/* Betadisk interface connected */
  if( libspectrum_snap_beta_paged( snap ) ) flags |= LIBSPECTRUM_ZXSTBETAF_PAGED;
  if( libspectrum_snap_beta_autoboot( snap ) ) flags |= LIBSPECTRUM_ZXSTBETAF_AUTOBOOT;
  if( !libspectrum_snap_beta_direction( snap ) ) flags |= LIBSPECTRUM_ZXSTBETAF_SEEKLOWER;
  if( libspectrum_snap_beta_custom_rom( snap ) ) flags |= LIBSPECTRUM_ZXSTBETAF_CUSTOMROM;
  if( use_compression ) flags |= LIBSPECTRUM_ZXSTBETAF_COMPRESSED;
  libspectrum_buffer_write_dword( data, flags );

  libspectrum_buffer_write_byte( data, libspectrum_snap_beta_drive_count( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_beta_system( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_beta_track ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_beta_sector( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_beta_data  ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_beta_status( snap ) );

  if( libspectrum_snap_beta_custom_rom( snap ) && rom_data ) {
    libspectrum_buffer_write_buffer( data, rom_buffer );
  }

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_BETA128, data );

  if( rom_buffer ) libspectrum_buffer_free( rom_buffer );
}

static int
compress_data( libspectrum_buffer *dest, const libspectrum_byte *src_data,
               size_t src_data_length, int compress )
{
  libspectrum_byte *compressed_data = NULL;
  int use_compression = 0;

#ifdef HAVE_ZLIB_H

  if( src_data && compress ) {

    libspectrum_error error;
    size_t compressed_length;

    error = libspectrum_zlib_compress( src_data, src_data_length,
				       &compressed_data, &compressed_length );

    if( error == LIBSPECTRUM_ERROR_NONE &&
        ( compress & LIBSPECTRUM_FLAG_SNAPSHOT_ALWAYS_COMPRESS ||
          compressed_length < src_data_length ) ) {
      use_compression = 1;
      src_data = compressed_data;
      src_data_length = compressed_length;
    }

  }

#endif				/* #ifdef HAVE_ZLIB_H */

  libspectrum_buffer_write( dest, src_data, src_data_length );

  if( compressed_data ) libspectrum_free( compressed_data );

  return use_compression;
}

static libspectrum_error
write_if1_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		 libspectrum_snap *snap, int compress  )
{
  libspectrum_byte *rom_data = NULL; 
  libspectrum_buffer *rom_buffer;
  libspectrum_word disk_rom_length = 0;
  libspectrum_word uncompressed_rom_length = 0;
  libspectrum_word flags = 0;
  int use_compression;
  int i;
  libspectrum_byte drive_count = 8;

  if( libspectrum_snap_interface1_custom_rom( snap ) ) {
    if( !(libspectrum_snap_interface1_rom_length( snap, 0 ) == 0x2000 ||
          libspectrum_snap_interface1_rom_length( snap, 0 ) == 0x4000 )) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                               "Interface 1 custom ROM must be 8192 or 16384 "
                               "bytes, supplied ROM is %lu bytes",
                               (unsigned long)
			       libspectrum_snap_interface1_rom_length(
                                 snap, 0 ) );
      return LIBSPECTRUM_ERROR_LOGIC;
    }
    rom_data = libspectrum_snap_interface1_rom( snap, 0 );
    if( rom_data == NULL ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                              "Interface 1 custom ROM specified to be %lu "
                              "bytes but NULL pointer provided",
                              (unsigned long)
                              libspectrum_snap_interface1_rom_length(
                                 snap, 0 ) );
      return LIBSPECTRUM_ERROR_LOGIC;
    }
    uncompressed_rom_length = disk_rom_length =
      libspectrum_snap_interface1_rom_length( snap, 0 );
  }

  rom_buffer = libspectrum_buffer_alloc();
  use_compression = compress_data( rom_buffer, rom_data,
                                   uncompressed_rom_length, compress );

  flags |= LIBSPECTRUM_ZXSTIF1F_ENABLED;
  if( libspectrum_snap_interface1_paged( snap ) ) flags |= LIBSPECTRUM_ZXSTIF1F_PAGED;
  if( use_compression ) flags |= LIBSPECTRUM_ZXSTIF1F_COMPRESSED;
  libspectrum_buffer_write_word( data, flags );

  if( libspectrum_snap_interface1_drive_count( snap ) )
    drive_count = libspectrum_snap_interface1_drive_count( snap );
  else
    drive_count = 8;	/* guess 8 drives connected */
  libspectrum_buffer_write_byte( data, drive_count );

  /* Skip 'reserved' data */
  for( i = 0; i < 3; i++ ) libspectrum_buffer_write_byte( data, 0 );

  /* Skip 'reserved' data */
  for( i = 0; i < 8; i++ ) libspectrum_buffer_write_dword( data, 0 );

  libspectrum_buffer_write_word( data, uncompressed_rom_length );

  if( libspectrum_snap_interface1_custom_rom( snap ) &&
      libspectrum_buffer_is_not_empty( rom_buffer ) ) {
    libspectrum_buffer_write_buffer( data, rom_buffer );
  }

  libspectrum_buffer_free( rom_buffer );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_IF1, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_opus_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress  )
{
  libspectrum_byte *rom_data, *ram_data; 
  libspectrum_buffer *rom_buffer, *ram_buffer;
  size_t disk_rom_length, disk_ram_length;
  libspectrum_dword flags = 0;
  int rom_compression = 0, ram_compression = 0;

  rom_data = libspectrum_snap_opus_rom( snap, 0 );
  if( rom_data == NULL ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                            "Opus ROM must be 8192 bytes but NULL pointer "
                            "provided" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  disk_rom_length = 0x2000;
  ram_data = libspectrum_snap_opus_ram( snap, 0 );
  disk_ram_length = 0x800;

  rom_buffer = libspectrum_buffer_alloc();
  rom_compression = compress_data( rom_buffer, rom_data, disk_rom_length,
                                   compress );
  ram_buffer = libspectrum_buffer_alloc();
  ram_compression = compress_data( ram_buffer, ram_data, disk_ram_length,
                                   compress );

  /* If we wanted to compress, only use a compressed buffer if both were
     compressed as we only have one flag */
  if( compress && !( rom_compression && ram_compression ) ) {
    libspectrum_buffer_clear( rom_buffer );
    libspectrum_buffer_write( rom_buffer, rom_data, disk_rom_length );
    libspectrum_buffer_clear( ram_buffer );
    libspectrum_buffer_write( ram_buffer, ram_data, disk_ram_length );
  }

  if( libspectrum_snap_opus_paged( snap ) ) flags |= LIBSPECTRUM_ZXSTOPUSF_PAGED;
  if( rom_compression && ram_compression ) flags |= LIBSPECTRUM_ZXSTOPUSF_COMPRESSED;
  if( !libspectrum_snap_opus_direction( snap ) ) flags |= LIBSPECTRUM_ZXSTOPUSF_SEEKLOWER;
  if( libspectrum_snap_opus_custom_rom( snap ) ) flags |= LIBSPECTRUM_ZXSTOPUSF_CUSTOMROM;
  libspectrum_buffer_write_dword( data, flags );

  libspectrum_buffer_write_dword( data,
                              libspectrum_buffer_get_data_size( ram_buffer ) );
  if( libspectrum_snap_opus_custom_rom( snap ) ) {
    libspectrum_buffer_write_dword( data,
                              libspectrum_buffer_get_data_size( rom_buffer ) );
  } else {
    libspectrum_buffer_write_dword( data, 0 );
  }
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_control_a( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_data_reg_a( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_data_dir_a( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_control_b( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_data_reg_b( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_data_dir_b( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_drive_count( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_track  ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_sector ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_data   ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_opus_status ( snap ) );

  libspectrum_buffer_write_buffer( data, ram_buffer );

  if( libspectrum_snap_opus_custom_rom( snap ) ) {
    libspectrum_buffer_write_buffer( data, rom_buffer );
  }

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_OPUS, data );

  libspectrum_buffer_free( rom_buffer );
  libspectrum_buffer_free( ram_buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
write_plsd_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress  )
{
  libspectrum_byte *rom_data, *ram_data; 
  libspectrum_buffer *rom_buffer, *ram_buffer;
  size_t disk_rom_length, disk_ram_length;
  libspectrum_dword flags = 0;
  int rom_compression = 0, ram_compression = 0;

  rom_data = libspectrum_snap_plusd_rom( snap, 0 );
  if( rom_data == NULL ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                            "+D ROM must be 8192 bytes but NULL pointer "
                            "provided" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  disk_rom_length = 0x2000;
  ram_data = libspectrum_snap_plusd_ram( snap, 0 );
  disk_ram_length = 0x2000;

  rom_buffer = libspectrum_buffer_alloc();
  rom_compression = compress_data( rom_buffer, rom_data, disk_rom_length,
                                   compress );
  ram_buffer = libspectrum_buffer_alloc();
  ram_compression = compress_data( ram_buffer, ram_data, disk_ram_length,
                                   compress );

  /* If we wanted to compress, only use a compressed buffer if both were
     compressed as we only have one flag */
  if( compress && !( rom_compression && ram_compression ) ) {
    libspectrum_buffer_clear( rom_buffer );
    libspectrum_buffer_write( rom_buffer, rom_data, disk_rom_length );
    libspectrum_buffer_clear( ram_buffer );
    libspectrum_buffer_write( ram_buffer, ram_data, disk_ram_length );
  }

  if( libspectrum_snap_plusd_paged( snap ) ) flags |= LIBSPECTRUM_ZXSTPLUSDF_PAGED;
  if( rom_compression && ram_compression ) flags |= LIBSPECTRUM_ZXSTPLUSDF_COMPRESSED;
  if( !libspectrum_snap_plusd_direction( snap ) ) flags |= LIBSPECTRUM_ZXSTPLUSDF_SEEKLOWER;
  libspectrum_buffer_write_dword( data, flags );

  libspectrum_buffer_write_dword( data,
                              libspectrum_buffer_get_data_size( ram_buffer ) );
  if( libspectrum_snap_plusd_custom_rom( snap ) ) {
    libspectrum_buffer_write_dword( data,
                              libspectrum_buffer_get_data_size( rom_buffer ) );
    libspectrum_buffer_write_byte( data, LIBSPECTRUM_ZXSTPDRT_CUSTOM );
  } else {
    libspectrum_buffer_write_dword( data, 0 );
    libspectrum_buffer_write_byte( data, LIBSPECTRUM_ZXSTPDRT_GDOS );
  }
  libspectrum_buffer_write_byte( data, libspectrum_snap_plusd_control( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_plusd_drive_count( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_plusd_track  ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_plusd_sector ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_plusd_data   ( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_plusd_status ( snap ) );

  libspectrum_buffer_write_buffer( data, ram_buffer );

  if( libspectrum_snap_plusd_custom_rom( snap ) ) {
    libspectrum_buffer_write_buffer( data, rom_buffer );
  }

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_PLUSD, data );

  libspectrum_buffer_free( rom_buffer );
  libspectrum_buffer_free( ram_buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static void
write_zxat_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  libspectrum_word flags;

  flags = 0;
  if( libspectrum_snap_zxatasp_upload ( snap ) ) flags |= LIBSPECTRUM_ZXSTZXATF_UPLOAD;
  if( libspectrum_snap_zxatasp_writeprotect( snap ) )
    flags |= LIBSPECTRUM_ZXSTZXATF_WRITEPROTECT;
  libspectrum_buffer_write_word( data, flags );

  libspectrum_buffer_write_byte( data, libspectrum_snap_zxatasp_port_a( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_zxatasp_port_b( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_zxatasp_port_c( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_zxatasp_control( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_zxatasp_pages( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_zxatasp_current_page( snap ) );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_ZXATASP, data );
}

static void
write_atrp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
		  libspectrum_snap *snap, int page, int compress )
{
  const libspectrum_byte *data = libspectrum_snap_zxatasp_ram( snap, page );

  write_ram_page( buffer, block_data, LIBSPECTRUM_ZXSTBID_ZXATASPRAMPAGE, data, 0x4000,
                  page, compress, 0x00 );
}

static void
write_zxcf_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap )
{
  libspectrum_word flags;

  flags = 0;
  if( libspectrum_snap_zxcf_upload( snap ) ) flags |= LIBSPECTRUM_ZXSTZXCFF_UPLOAD;
  libspectrum_buffer_write_word( data, flags );

  libspectrum_buffer_write_byte( data, libspectrum_snap_zxcf_memctl( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_zxcf_pages( snap ) );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_ZXCF, data );
}

static void
write_cfrp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress )
{
  const libspectrum_byte *data = libspectrum_snap_zxcf_ram( snap, page );

  write_ram_page( buffer, block_data, LIBSPECTRUM_ZXSTBID_ZXCFRAMPAGE, data, 0x4000, page,
                  compress, 0x00 );
}

#ifdef HAVE_ZLIB_H

static libspectrum_error
write_if2r_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap )
{
  libspectrum_error error;
  libspectrum_byte *data, *compressed_data;
  size_t data_length, compressed_length;

  data = libspectrum_snap_interface2_rom( snap, 0 ); data_length = 0x4000;
  compressed_data = NULL;

  error = libspectrum_zlib_compress( data, data_length,
                                     &compressed_data, &compressed_length );
  if( error ) return error;

  libspectrum_buffer_write_dword( block_data, compressed_length );
  libspectrum_buffer_write( block_data, compressed_data, compressed_length );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_IF2ROM, block_data );

  if( compressed_data ) libspectrum_free( compressed_data );

  return LIBSPECTRUM_ERROR_NONE;
}

#endif                         /* #ifdef HAVE_ZLIB_H */

static void
write_dock_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
		  libspectrum_snap *snap, int exrom_dock,
                  const libspectrum_byte *data, int page, int writeable,
                  int compress )
{
  libspectrum_byte extra_flags = 0;

  if( writeable  ) extra_flags |= LIBSPECTRUM_ZXSTDOCKF_RAM;
  if( exrom_dock ) extra_flags |= LIBSPECTRUM_ZXSTDOCKF_EXROMDOCK;

  write_ram_page( buffer, block_data, LIBSPECTRUM_ZXSTBID_DOCK, data, 0x2000, page,
                  compress, extra_flags );
}

static void
write_side_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
		  libspectrum_snap *snap )
{
  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_SIMPLEIDE, NULL );
}

static void
write_drum_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  libspectrum_buffer_write_byte( data,
                                 libspectrum_snap_specdrum_dac( snap ) + 128 );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_SPECDRUM, data );
}

static void
write_covx_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap )
{
  libspectrum_buffer_write_byte( data, libspectrum_snap_covox_dac( snap ) );

  /* Write 'reserved' data */
  libspectrum_buffer_write_byte( data, 0 );
  libspectrum_buffer_write_byte( data, 0 );
  libspectrum_buffer_write_byte( data, 0 );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_COVOX, data );
}

static libspectrum_error
write_dide_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress )
{
  libspectrum_byte *eprom_data = NULL;
  libspectrum_buffer *eprom_buffer;
  libspectrum_word flags = 0;
  libspectrum_word uncompressed_eprom_length = 0;
  int use_compression;

  eprom_data = libspectrum_snap_divide_eprom( snap, 0 );
  if( !eprom_data ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                             "DivIDE EPROM data is missing" );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  uncompressed_eprom_length = 0x2000;

  eprom_buffer = libspectrum_buffer_alloc();
  use_compression = compress_data( eprom_buffer, eprom_data,
                                   uncompressed_eprom_length, compress );

  if( libspectrum_snap_divide_eprom_writeprotect( snap ) )
    flags |= LIBSPECTRUM_ZXSTDIVIDE_EPROM_WRITEPROTECT;
  if( libspectrum_snap_divide_paged( snap ) ) flags |= LIBSPECTRUM_ZXSTDIVIDE_PAGED;
  if( use_compression ) flags |= LIBSPECTRUM_ZXSTDIVIDE_COMPRESSED;
  libspectrum_buffer_write_word( data, flags );

  libspectrum_buffer_write_byte( data, libspectrum_snap_divide_control( snap ) );
  libspectrum_buffer_write_byte( data, libspectrum_snap_divide_pages( snap ) );

  libspectrum_buffer_write_buffer( data, eprom_buffer );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_DIVIDE, data );

  libspectrum_buffer_free( eprom_buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static void
write_dirp_chunk( libspectrum_buffer *buffer, libspectrum_buffer *block_data,
                  libspectrum_snap *snap, int page, int compress )
{
  const libspectrum_byte *data = libspectrum_snap_divide_ram( snap, page );

  write_ram_page( buffer, block_data, LIBSPECTRUM_ZXSTBID_DIVIDERAMPAGE, data, 0x2000,
                  page, compress, 0x00 );
}

static void
write_snet_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
                  libspectrum_snap *snap, int compress )
{
  libspectrum_word flags = 0;

  if( libspectrum_snap_spectranet_paged( snap ) )
    flags |= LIBSPECTRUM_ZXSTSNET_PAGED;
  if( libspectrum_snap_spectranet_paged_via_io( snap ) )
    flags |= LIBSPECTRUM_ZXSTSNET_PAGED_VIA_IO;
  if( libspectrum_snap_spectranet_programmable_trap_active( snap ) )
    flags |= LIBSPECTRUM_ZXSTSNET_PROGRAMMABLE_TRAP_ACTIVE;
  if( libspectrum_snap_spectranet_programmable_trap_msb( snap ) )
    flags |= LIBSPECTRUM_ZXSTSNET_PROGRAMMABLE_TRAP_MSB;
  if( libspectrum_snap_spectranet_all_traps_disabled( snap ) )
    flags |= LIBSPECTRUM_ZXSTSNET_ALL_DISABLED;
  if( libspectrum_snap_spectranet_rst8_trap_disabled( snap ) )
    flags |= LIBSPECTRUM_ZXSTSNET_RST8_DISABLED;
  if( libspectrum_snap_spectranet_deny_downstream_a15( snap ) )
    flags |= LIBSPECTRUM_ZXSTSNET_DENY_DOWNSTREAM_A15;
  if( libspectrum_snap_spectranet_nmi_flipflop( snap ) )
    flags |= LIBSPECTRUM_ZXSTSNET_NMI_FLIPFLOP;
  libspectrum_buffer_write_word( data, flags );

  libspectrum_buffer_write_byte( data,
                                 libspectrum_snap_spectranet_page_a( snap ) );
  libspectrum_buffer_write_byte( data,
                                 libspectrum_snap_spectranet_page_b( snap ) );

  libspectrum_buffer_write_word( data,
    libspectrum_snap_spectranet_programmable_trap( snap ) );

  libspectrum_buffer_write( data, libspectrum_snap_spectranet_w5100( snap, 0 ),
                            0x30 );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_SPECTRANET, data );
}

static void
write_snef_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress )
{
  size_t flash_length;
  libspectrum_byte *flash_data;
  libspectrum_buffer *flash_buffer;
  int flash_compressed = 0;
  libspectrum_byte flags = 0;

  flash_data = libspectrum_snap_spectranet_flash( snap, 0 );
  flash_length = 0x20000;

  flash_buffer = libspectrum_buffer_alloc();
  flash_compressed = compress_data( flash_buffer, flash_data, flash_length,
                                    compress );

  if( flash_compressed )
    flags |= LIBSPECTRUM_ZXSTSNEF_FLASH_COMPRESSED;
  libspectrum_buffer_write_byte( data, flags );

  libspectrum_buffer_write_dword( data,
                            libspectrum_buffer_get_data_size( flash_buffer ) );
  libspectrum_buffer_write_buffer( data, flash_buffer );

  libspectrum_buffer_free( flash_buffer );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_SPECTRANETFLASHPAGE, data );
}

static void
write_sner_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress )
{
  size_t ram_length;
  libspectrum_byte *ram_data;
  libspectrum_buffer *ram_buffer;
  int ram_compressed = 0;
  libspectrum_byte flags = 0;

  ram_data = libspectrum_snap_spectranet_ram( snap, 0 );
  ram_length = 0x20000;

  ram_buffer = libspectrum_buffer_alloc();
  ram_compressed = compress_data( ram_buffer, ram_data,
                                  ram_length, compress );

  if( ram_compressed )
    flags |= LIBSPECTRUM_ZXSTSNER_RAM_COMPRESSED;
  libspectrum_buffer_write_byte( data, flags );

  libspectrum_buffer_write_dword( data,
                              libspectrum_buffer_get_data_size( ram_buffer ) );
  libspectrum_buffer_write_buffer( data, ram_buffer );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_SPECTRANETRAMPAGE, data );

  libspectrum_buffer_free( ram_buffer );
}

static libspectrum_error
write_mfce_chunk( libspectrum_buffer *buffer, libspectrum_buffer *data,
		  libspectrum_snap *snap, int compress )
{
  libspectrum_byte *ram_data;
  libspectrum_buffer *ram_buffer;
  size_t ram_length;
  libspectrum_byte flags = 0;
  libspectrum_byte model;
  int use_compression = 0;

  ram_length = libspectrum_snap_multiface_ram_length( snap, 0 );
  if( ram_length != 0x2000 && ram_length != 0x4000 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                               "%s:write_mfce_chunk: invalid RAM length, "
                               "should be 8192 or 16384 bytes, "
                               "provided snap has %lu",
                               __FILE__,
                               (unsigned long)ram_length );
    return LIBSPECTRUM_ERROR_LOGIC;
  }
  ram_data = libspectrum_snap_multiface_ram( snap, 0 );
  if( ram_data == NULL ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
                            "Multiface RAM specified to be %lu "
                            "bytes but NULL pointer provided in snap",
                            (unsigned long)ram_length );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  ram_buffer = libspectrum_buffer_alloc();
  use_compression = compress_data( ram_buffer, ram_data, ram_length, compress );

  if( libspectrum_snap_multiface_model_one( snap ) )
    model = LIBSPECTRUM_ZXSTMFM_1;
  else
    model = LIBSPECTRUM_ZXSTMFM_128;
  libspectrum_buffer_write_byte( data, model );

  if( libspectrum_snap_multiface_paged( snap ) ) flags |= LIBSPECTRUM_ZXSTMF_PAGEDIN;
  if( use_compression ) flags |= LIBSPECTRUM_ZXSTMF_COMPRESSED;
  if( libspectrum_snap_multiface_software_lockout( snap ) )
    flags |= LIBSPECTRUM_ZXSTMF_SOFTWARELOCKOUT;
  if( libspectrum_snap_multiface_red_button_disabled( snap ) )
    flags |= LIBSPECTRUM_ZXSTMF_REDBUTTONDISABLED;
  if( libspectrum_snap_multiface_disabled( snap ) ) flags |= LIBSPECTRUM_ZXSTMF_DISABLED;
  if( ram_length == 0x4000 ) flags |= LIBSPECTRUM_ZXSTMF_16KRAMMODE;
  libspectrum_buffer_write_byte( data, flags );

  libspectrum_buffer_write_buffer( data, ram_buffer );

  write_chunk( buffer, LIBSPECTRUM_ZXSTBID_MULTIFACE, data );

  libspectrum_buffer_free( ram_buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static void
write_chunk( libspectrum_buffer *buffer, const char *id,
             libspectrum_buffer *block_data )
{
  size_t data_length = libspectrum_buffer_get_data_size( block_data );
  libspectrum_buffer_write( buffer, id, 4 );
  libspectrum_buffer_write_dword( buffer, data_length );
  libspectrum_buffer_write_buffer( buffer, block_data );
  libspectrum_buffer_clear( block_data );
}
