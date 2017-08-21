/* szx_read.c: Routines for reading .szx snapshots
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

static libspectrum_error
read_chunk( libspectrum_snap *snap, libspectrum_word version,
	    const libspectrum_byte **buffer, const libspectrum_byte *end,
            szx_context *ctx );

typedef libspectrum_error (*read_chunk_fn)( libspectrum_snap *snap,
					    libspectrum_word version,
					    const libspectrum_byte **buffer,
					    const libspectrum_byte *end,
					    size_t data_length,
                                            szx_context *ctx );

static libspectrum_error
read_ram_page( libspectrum_byte **data, size_t *page,
	       const libspectrum_byte **buffer, size_t data_length,
	       size_t uncompressed_length, libspectrum_word *flags )
{
#ifdef HAVE_ZLIB_H

  libspectrum_error error;

#endif			/* #ifdef HAVE_ZLIB_H */

  if( data_length < 3 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_ram_page: length %lu too short",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  *flags = libspectrum_read_word( buffer );

  *page = **buffer; (*buffer)++;

  if( *flags & ZXSTRF_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

    error = libspectrum_zlib_inflate( *buffer, data_length - 3, data,
				      &uncompressed_length );
    if( error ) return error;

    *buffer += data_length - 3;

#else			/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_ram_page: zlib needed for decompression\n",
      __FILE__
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

  } else {

    if( data_length < 3 + uncompressed_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_ram_page: length %lu too short",
			       __FILE__, (unsigned long)data_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    *data = libspectrum_new( libspectrum_byte, uncompressed_length );
    memcpy( *data, *buffer, uncompressed_length );
    *buffer += uncompressed_length;

  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_atrp_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;

  error = read_ram_page( &data, &page, buffer, data_length, 0x4000, &flags );
  if( error ) return error;

  if( page >= SNAPSHOT_ZXATASP_PAGES ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_atrp_chunk: unknown page number %lu",
			     __FILE__, (unsigned long)page );
    libspectrum_free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_zxatasp_ram( snap, page, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_ay_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
	       const libspectrum_byte **buffer,
	       const libspectrum_byte *end GCC_UNUSED, size_t data_length,
               szx_context *ctx GCC_UNUSED )
{
  size_t i;
  libspectrum_byte flags;

  if( data_length != 18 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_ay_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = **buffer; (*buffer)++;
  libspectrum_snap_set_fuller_box_active( snap, flags & ZXSTAYF_FULLERBOX );
  libspectrum_snap_set_melodik_active( snap, !!( flags & ZXSTAYF_128AY ) );

  libspectrum_snap_set_out_ay_registerport( snap, **buffer ); (*buffer)++;

  for( i = 0; i < 16; i++ ) {
    libspectrum_snap_set_ay_registers( snap, i, **buffer ); (*buffer)++;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_b128_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
#ifdef HAVE_ZLIB_H
  libspectrum_error error;
#endif
  libspectrum_byte *rom_data = NULL;
  libspectrum_dword flags;
  const size_t expected_length = 0x4000;

  if( data_length < 10 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_b128_chunk: length %lu too short",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_dword( buffer );
  libspectrum_snap_set_beta_active( snap, 1 );
  libspectrum_snap_set_beta_paged( snap, !!( flags & ZXSTBETAF_PAGED ) );
  libspectrum_snap_set_beta_autoboot( snap, !!( flags & ZXSTBETAF_AUTOBOOT ) );
  libspectrum_snap_set_beta_direction( snap,
				       !( flags & ZXSTBETAF_SEEKLOWER ) );

  libspectrum_snap_set_beta_custom_rom( snap,
                                        !!( flags & ZXSTBETAF_CUSTOMROM ) );

  libspectrum_snap_set_beta_drive_count( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_system( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_track ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_sector( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_data  ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_beta_status( snap, **buffer ); (*buffer)++;

  if( libspectrum_snap_beta_custom_rom( snap ) ) {

    if( flags & ZXSTBETAF_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

      size_t uncompressed_length = 0;

      error = libspectrum_zlib_inflate( *buffer, data_length - 10, &rom_data,
					&uncompressed_length );
      if( error ) return error;

      if( uncompressed_length != expected_length ) {
	libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
				 "%s:read_b128_chunk: invalid ROM length "
				 "in compressed file, should be %lu, file "
				 "has %lu",
				 __FILE__,
				 (unsigned long)expected_length,
				 (unsigned long)uncompressed_length );
	return LIBSPECTRUM_ERROR_UNKNOWN;
      }

#else

      libspectrum_print_error(
	LIBSPECTRUM_ERROR_UNKNOWN,
	"%s:read_b128_chunk: zlib needed for decompression\n",
	__FILE__
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;

#endif

    } else {

      if( data_length < 10 + expected_length ) {
        libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
				 "%s:read_b128_chunk: length %lu too short, "
				 "expected %lu",
				 __FILE__, (unsigned long)data_length,
				 (unsigned long)10 + expected_length );
	return LIBSPECTRUM_ERROR_UNKNOWN;
      }

      rom_data = libspectrum_new( libspectrum_byte, expected_length );
      memcpy( rom_data, *buffer, expected_length );

    }

  }

  libspectrum_snap_set_beta_rom( snap, 0, rom_data );

  /* Skip any extra data (most likely a custom ROM) */
  *buffer += data_length - 10;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_covx_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
		 szx_context *ctx )
{
  if( data_length != 4 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_covx_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_covox_dac( snap, *(*buffer)++ );

  libspectrum_snap_set_covox_active( snap, 1 );

  *buffer += 3;			/* Skip 'reserved' data */

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_crtr_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx )
{
  if( data_length < 36 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_crtr_chunk: length %lu too short",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  *buffer += 36;
  data_length -= 36;

  /* This is ugly, but I can't see a better way to do it */
  if( sizeof( libspectrum_byte ) == sizeof( char ) ) {
    char *custom = libspectrum_new( char, data_length + 1 );
    char *libspectrum;

    memcpy( custom, *buffer, data_length );
    custom[data_length] = 0;

    libspectrum = strstr( custom, libspectrum_string );
    if( libspectrum ) {
      int matches, v1, v2, v3;
      libspectrum += strlen( libspectrum_string );
      matches = sscanf( libspectrum, "%d.%d.%d", &v1, &v2, &v3 );
      if( matches == 3 ) {
        if( v1 == 0 ) {
          if( v2 < 5 || ( v2 == 5 && v3 == 0 ) ) {
            ctx->swap_af = 1;
          }
        }
      }
    }

    libspectrum_free( custom );
  }

  *buffer += data_length;

  return LIBSPECTRUM_ERROR_NONE;
}
     
static libspectrum_error
read_opus_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
#ifdef HAVE_ZLIB_H
  libspectrum_error error;
#endif
  libspectrum_byte *ram_data = NULL, *rom_data = NULL;
  libspectrum_dword flags;
  size_t disc_ram_length;
  size_t disc_rom_length;
  size_t expected_length = 0x800;

  if( data_length < 23 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_opus_chunk: length %lu too short",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_opus_active( snap, 1 );

  flags = libspectrum_read_dword( buffer );
  libspectrum_snap_set_opus_paged( snap, flags & ZXSTOPUSF_PAGED );
  libspectrum_snap_set_opus_direction( snap,
				       !( flags & ZXSTOPUSF_SEEKLOWER ) );

  disc_ram_length = libspectrum_read_dword( buffer );
  disc_rom_length = libspectrum_read_dword( buffer );

  libspectrum_snap_set_opus_custom_rom( snap,
                                        !!( flags & ZXSTOPUSF_CUSTOMROM ) );
  if( libspectrum_snap_opus_custom_rom( snap ) && !disc_rom_length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_opus_chunk: block flagged as custom "
                             "ROM but there is no custom ROM stored in the "
                             "snapshot" );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_opus_control_a( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_data_reg_a( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_data_dir_a( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_control_b( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_data_reg_b( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_data_dir_b( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_drive_count( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_track ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_sector( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_data  ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_opus_status( snap, **buffer ); (*buffer)++;

  if( flags & ZXSTOPUSF_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

    size_t uncompressed_length = 0;

    if( (!libspectrum_snap_opus_custom_rom( snap ) &&
         disc_rom_length != 0 ) ||
        (libspectrum_snap_opus_custom_rom( snap ) &&
         disc_rom_length == 0 ) ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_opus_chunk: invalid ROM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               0UL,
                               (unsigned long)disc_rom_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    if( data_length < 23 + disc_ram_length + disc_rom_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_opus_chunk: length %lu too short, "
			       "expected %lu" ,
			       __FILE__, (unsigned long)data_length,
			       (unsigned long)23 + disc_ram_length +
                                 disc_rom_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    error = libspectrum_zlib_inflate( *buffer, disc_ram_length, &ram_data,
				      &uncompressed_length );
    if( error ) return error;

    if( uncompressed_length != expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_opus_chunk: invalid RAM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               (unsigned long)expected_length,
                               (unsigned long)uncompressed_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    *buffer += disc_ram_length;

    if( libspectrum_snap_opus_custom_rom( snap ) ) {
      uncompressed_length = 0;

      error = libspectrum_zlib_inflate( *buffer, disc_rom_length, &rom_data,
                                        &uncompressed_length );
      if( error ) return error;

      expected_length = 0x2000;

      if( uncompressed_length != expected_length ) {
        libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                                 "%s:read_opus_chunk: invalid ROM length "
                                 "in compressed file, should be %lu, file "
                                 "has %lu",
                                 __FILE__, 
                                 (unsigned long)expected_length,
                                 (unsigned long)uncompressed_length );
        return LIBSPECTRUM_ERROR_UNKNOWN;
      }

      *buffer += disc_rom_length;
    }

#else			/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_opus_chunk: zlib needed for decompression\n",
      __FILE__
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

  } else {

    if( disc_ram_length != expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_opus_chunk: invalid RAM length "
                               "in uncompressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               (unsigned long)expected_length,
                               (unsigned long)disc_ram_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    expected_length = 0x2000;

    if( (libspectrum_snap_opus_custom_rom( snap ) &&
         disc_rom_length != expected_length ) ||
        (!libspectrum_snap_opus_custom_rom( snap ) &&
         disc_rom_length != 0 ) ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_opus_chunk: invalid ROM length "
                               "in uncompressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               libspectrum_snap_opus_custom_rom( snap ) ?
                                 expected_length : 0UL,
                               (unsigned long)disc_rom_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    if( data_length < 23 + disc_ram_length + disc_rom_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_opus_chunk: length %lu too short, "
			       "expected %lu" ,
			       __FILE__, (unsigned long)data_length,
			       (unsigned long)23 + disc_ram_length + disc_rom_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    expected_length = 0x800;

    ram_data = libspectrum_new( libspectrum_byte, expected_length );
    memcpy( ram_data, *buffer, expected_length );
    *buffer += expected_length;

    if( libspectrum_snap_opus_custom_rom( snap ) ) {
      expected_length = 0x2000;
      rom_data = libspectrum_new( libspectrum_byte, expected_length );
      memcpy( rom_data, *buffer, expected_length );
      *buffer += expected_length;
    }

  }

  libspectrum_snap_set_opus_ram( snap, 0, ram_data );
  libspectrum_snap_set_opus_rom( snap, 0, rom_data );

  return LIBSPECTRUM_ERROR_NONE;
}
     
static libspectrum_error
read_plsd_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		  const libspectrum_byte **buffer,
		  const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
#ifdef HAVE_ZLIB_H
  libspectrum_error error;
#endif
  libspectrum_byte *ram_data = NULL, *rom_data = NULL;
  libspectrum_byte rom_type;
  libspectrum_dword flags;
  size_t disc_ram_length;
  size_t disc_rom_length;
  const size_t expected_length = 0x2000;

  if( data_length < 19 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_plusd_chunk: length %lu too short",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_plusd_active( snap, 1 );

  flags = libspectrum_read_dword( buffer );
  libspectrum_snap_set_plusd_paged( snap, flags & ZXSTPLUSDF_PAGED );
  libspectrum_snap_set_plusd_direction( snap,
				       !( flags & ZXSTPLUSDF_SEEKLOWER ) );

  disc_ram_length = libspectrum_read_dword( buffer );
  disc_rom_length = libspectrum_read_dword( buffer );
  rom_type = *(*buffer)++;

  libspectrum_snap_set_plusd_custom_rom( snap, rom_type == ZXSTPDRT_CUSTOM );
  if( libspectrum_snap_plusd_custom_rom( snap ) && !disc_rom_length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_plusd_chunk: block flagged as custom "
                             "ROM but there is no custom ROM stored in the "
                             "snapshot" );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_plusd_control( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_plusd_drive_count( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_plusd_track ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_plusd_sector( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_plusd_data  ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_plusd_status( snap, **buffer ); (*buffer)++;

  if( flags & ZXSTPLUSDF_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

    size_t uncompressed_length = 0;

    if( (!libspectrum_snap_plusd_custom_rom( snap ) &&
         disc_rom_length != 0 ) ||
        (libspectrum_snap_plusd_custom_rom( snap ) &&
         disc_rom_length == 0 ) ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_plsd_chunk: invalid ROM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               0UL,
                               (unsigned long)disc_rom_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    if( data_length < 19 + disc_ram_length + disc_rom_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_plsd_chunk: length %lu too short, "
			       "expected %lu" ,
			       __FILE__, (unsigned long)data_length,
			       (unsigned long)19 + disc_ram_length +
                                 disc_rom_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    error = libspectrum_zlib_inflate( *buffer, disc_ram_length, &ram_data,
				      &uncompressed_length );
    if( error ) return error;

    if( uncompressed_length != expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_plsd_chunk: invalid RAM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               (unsigned long)expected_length,
                               (unsigned long)uncompressed_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    *buffer += disc_ram_length;

    if( libspectrum_snap_plusd_custom_rom( snap ) ) {
      uncompressed_length = 0;

      error = libspectrum_zlib_inflate( *buffer, disc_rom_length, &rom_data,
                                        &uncompressed_length );
      if( error ) return error;

      if( uncompressed_length != expected_length ) {
        libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                                 "%s:read_plsd_chunk: invalid ROM length "
                                 "in compressed file, should be %lu, file "
                                 "has %lu",
                                 __FILE__, 
                                 (unsigned long)expected_length,
                                 (unsigned long)uncompressed_length );
        return LIBSPECTRUM_ERROR_UNKNOWN;
      }

      *buffer += disc_rom_length;
    }

#else			/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_plsd_chunk: zlib needed for decompression\n",
      __FILE__
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

  } else {

    if( disc_ram_length != expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_plsd_chunk: invalid RAM length "
                               "in uncompressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               (unsigned long)expected_length,
                               (unsigned long)disc_ram_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    if( (libspectrum_snap_plusd_custom_rom( snap ) &&
         disc_rom_length != expected_length ) ||
        (!libspectrum_snap_plusd_custom_rom( snap ) &&
         disc_rom_length != 0 ) ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_plsd_chunk: invalid ROM length "
                               "in uncompressed file, should be %lu, file "
                               "has %lu",
			       __FILE__, 
                               libspectrum_snap_plusd_custom_rom( snap ) ?
                                 expected_length : 0UL,
                               (unsigned long)disc_rom_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    if( data_length < 19 + disc_ram_length + disc_rom_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			       "%s:read_plsd_chunk: length %lu too short, "
			       "expected %lu" ,
			       __FILE__, (unsigned long)data_length,
			       (unsigned long)19 + disc_ram_length + disc_rom_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    ram_data = libspectrum_new( libspectrum_byte, expected_length );
    memcpy( ram_data, *buffer, expected_length );
    *buffer += expected_length;

    if( libspectrum_snap_plusd_custom_rom( snap ) ) {
      rom_data = libspectrum_new( libspectrum_byte, expected_length );
      memcpy( rom_data, *buffer, expected_length );
      *buffer += expected_length;
    }

  }

  libspectrum_snap_set_plusd_ram( snap, 0, ram_data );
  libspectrum_snap_set_plusd_rom( snap, 0, rom_data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_cfrp_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;

  error = read_ram_page( &data, &page, buffer, data_length, 0x4000, &flags );
  if( error ) return error;

  if( page >= SNAPSHOT_ZXCF_PAGES ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_cfrp_chunk: unknown page number %lu",
			     __FILE__, (unsigned long)page );
    libspectrum_free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_zxcf_ram( snap, page, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_side_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  if( data_length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_side_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_simpleide_active( snap, 1 );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_drum_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte volume;

  if( data_length != 1 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_drum_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  volume = *(*buffer)++;

  libspectrum_snap_set_specdrum_dac( snap, volume - 128 );

  libspectrum_snap_set_specdrum_active( snap, 1 );

  return LIBSPECTRUM_ERROR_NONE;
}


static void
add_joystick( libspectrum_snap *snap, libspectrum_joystick type, int inputs )
{
  size_t i;
  size_t num_joysticks = libspectrum_snap_joystick_active_count( snap );

  for( i = 0; i < num_joysticks; i++ ) {
    if( libspectrum_snap_joystick_list( snap, i ) == type ) {
      libspectrum_snap_set_joystick_inputs( snap, i, inputs |
                                  libspectrum_snap_joystick_inputs( snap, i ) );
      return;
    }
  }

  libspectrum_snap_set_joystick_list( snap, num_joysticks, type );
  libspectrum_snap_set_joystick_inputs( snap, num_joysticks, inputs );
  libspectrum_snap_set_joystick_active_count( snap, num_joysticks + 1 );
}

static libspectrum_error
read_joy_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_dword flags;

  if( data_length != 6 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_joy_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_dword( buffer );
  if( flags & ZXSTJOYF_ALWAYSPORT31 ) {
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_KEMPSTON,
                  LIBSPECTRUM_JOYSTICK_INPUT_NONE );
  }

  switch( **buffer ) {
  case ZXJT_KEMPSTON:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_KEMPSTON,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_FULLER:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_FULLER,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_CURSOR:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_CURSOR,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_SINCLAIR1:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_SINCLAIR_1,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_SINCLAIR2:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_SINCLAIR_2,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_TIMEX1:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_TIMEX_1,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_TIMEX2:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_TIMEX_2,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_1 );
    break;
  case ZXJT_NONE:
    break;
  }
  (*buffer)++;

  switch( **buffer ) {
  case ZXJT_KEMPSTON:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_KEMPSTON,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_FULLER:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_FULLER,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_CURSOR:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_CURSOR,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_SINCLAIR1:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_SINCLAIR_1,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_SINCLAIR2:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_SINCLAIR_2,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_TIMEX1:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_TIMEX_1,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_TIMEX2:
    add_joystick( snap, LIBSPECTRUM_JOYSTICK_TIMEX_2,
                  LIBSPECTRUM_JOYSTICK_INPUT_JOYSTICK_2 );
    break;
  case ZXJT_NONE:
    break;
  }
  (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_keyb_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  size_t expected_length;
  libspectrum_dword flags;

  expected_length = version >= 0x0101 ? 5 : 4;

  if( data_length != expected_length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_keyb_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_dword( buffer );
  libspectrum_snap_set_issue2( snap, !!( flags & ZXSTKF_ISSUE2 ) );

  if( expected_length >= 5 ) {
    switch( **buffer ) {
    case ZXJT_KEMPSTON:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_KEMPSTON,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_FULLER:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_FULLER,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_CURSOR:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_CURSOR,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_SINCLAIR1:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_SINCLAIR_1,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_SINCLAIR2:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_SINCLAIR_2,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_TIMEX1:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_TIMEX_1,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_TIMEX2:
      add_joystick( snap, LIBSPECTRUM_JOYSTICK_TIMEX_2,
                    LIBSPECTRUM_JOYSTICK_INPUT_KEYBOARD );
      break;
    case ZXJT_SPECTRUMPLUS: /* Actually, no joystick at all */
      break;
    }
    (*buffer)++;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_amxm_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  if( data_length != 7 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "read_amxm_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  switch( **buffer ) {
  case ZXSTM_AMX:
    break;
  case ZXSTM_KEMPSTON:
    libspectrum_snap_set_kempston_mouse_active( snap, 1 );
    break;
  case ZXSTM_NONE:
  default:
    break;
  }
  (*buffer)++;

  /* Z80 PIO CTRLA registers for AMX mouse */
  (*buffer)++; (*buffer)++; (*buffer)++;

  /* Z80 PIO CTRLB registers for AMX mouse */
  (*buffer)++; (*buffer)++; (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_ramp_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;


  error = read_ram_page( &data, &page, buffer, data_length, 0x4000, &flags );
  if( error ) return error;

  if( page > 63 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_ramp_chunk: unknown page number %lu",
			     __FILE__, (unsigned long)page );
    libspectrum_free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_pages( snap, page, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_scld_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  if( data_length != 2 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_scld_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_out_scld_hsr( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_out_scld_dec( snap, **buffer ); (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_spcr_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte out_ula;
  int capabilities;

  if( data_length != 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "szx_read_spcr_chunk: unknown length %lu", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  capabilities =
    libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );

  out_ula = **buffer & 0x07; (*buffer)++;

  libspectrum_snap_set_out_128_memoryport( snap, **buffer ); (*buffer)++;

  if( ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY )    ||
      ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_SCORP_MEMORY )    ||
      ( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PENT1024_MEMORY )    )
    libspectrum_snap_set_out_plus3_memoryport( snap, **buffer );
  (*buffer)++;

  if( version >= 0x0101 ) out_ula |= **buffer & 0xf8;
  (*buffer)++;

  libspectrum_snap_set_out_ula( snap, out_ula );

  *buffer += 4;			/* Skip 'reserved' data */

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_z80r_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx )
{
  if( data_length != 37 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_z80r_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  if( ctx->swap_af ) {
    libspectrum_snap_set_a( snap, **buffer ); (*buffer)++;
    libspectrum_snap_set_f( snap, **buffer ); (*buffer)++;
  } else {
    libspectrum_snap_set_f( snap, **buffer ); (*buffer)++;
    libspectrum_snap_set_a( snap, **buffer ); (*buffer)++;
  }

  libspectrum_snap_set_bc  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_de  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_hl  ( snap, libspectrum_read_word( buffer ) );

  if( ctx->swap_af ) {
    libspectrum_snap_set_a_( snap, **buffer ); (*buffer)++;
    libspectrum_snap_set_f_( snap, **buffer ); (*buffer)++;
  } else {
    libspectrum_snap_set_f_( snap, **buffer ); (*buffer)++;
    libspectrum_snap_set_a_( snap, **buffer ); (*buffer)++;
  }

  libspectrum_snap_set_bc_ ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_de_ ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_hl_ ( snap, libspectrum_read_word( buffer ) );

  libspectrum_snap_set_ix  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_iy  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_sp  ( snap, libspectrum_read_word( buffer ) );
  libspectrum_snap_set_pc  ( snap, libspectrum_read_word( buffer ) );

  libspectrum_snap_set_i   ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_r   ( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_iff1( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_iff2( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_im  ( snap, **buffer ); (*buffer)++;

  libspectrum_snap_set_tstates( snap, libspectrum_read_dword( buffer ) );

  if( version >= 0x0101 ) {
    (*buffer)++;		/* Skip chHoldIntReqCycles */
    
    /* Flags */
    libspectrum_snap_set_last_instruction_ei( snap, **buffer & ZXSTZF_EILAST );
    libspectrum_snap_set_halted( snap, **buffer & ZXSTZF_HALTED );
    libspectrum_snap_set_last_instruction_set_f( snap, **buffer & ZXSTZF_FSET );
    (*buffer)++;

    if( version >= 0x0104 ) {
      libspectrum_snap_set_memptr( snap, libspectrum_read_word( buffer ) );
    } else {
      (*buffer)++;		/* Skip the hidden register */
      (*buffer)++;		/* Skip the reserved byte */
    }

  } else {
    *buffer += 4;		/* Skip the reserved dword */
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_zxat_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_word flags;

  if( data_length != 8 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_zxat_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_zxatasp_active( snap, 1 );

  flags = libspectrum_read_word( buffer );
  libspectrum_snap_set_zxatasp_upload( snap, flags & ZXSTZXATF_UPLOAD );
  libspectrum_snap_set_zxatasp_writeprotect( snap,
    !!( flags & ZXSTZXATF_WRITEPROTECT ) );

  libspectrum_snap_set_zxatasp_port_a( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_port_b( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_port_c( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_control( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_pages( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxatasp_current_page( snap, **buffer ); (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_zxcf_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_word flags;

  if( data_length != 4 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "read_zxcf_chunk: unknown length %lu",
			     (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_zxcf_active( snap, 1 );

  flags = libspectrum_read_word( buffer );
  libspectrum_snap_set_zxcf_upload( snap, flags & ZXSTZXCFF_UPLOAD );

  libspectrum_snap_set_zxcf_memctl( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_zxcf_pages( snap, **buffer ); (*buffer)++;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_if1_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		const libspectrum_byte **buffer,
		const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                szx_context *ctx GCC_UNUSED )
{
  libspectrum_word flags;
  libspectrum_word expected_length;
  libspectrum_byte *rom_data = NULL; 

  if( data_length < 40 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_if1_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_word( buffer );
  libspectrum_snap_set_interface1_drive_count( snap, **buffer ); (*buffer)++;
  *buffer += 3;		/* Skip reserved byte space */
  *buffer += sizeof( libspectrum_dword ) * 8; /* Skip reserved dword space */
  expected_length = libspectrum_read_word( buffer );

  libspectrum_snap_set_interface1_active( snap, flags & ZXSTIF1F_ENABLED );

  libspectrum_snap_set_interface1_paged( snap, !!( flags & ZXSTIF1F_PAGED ) );

  if( expected_length ) {
    if( expected_length != 0x2000 && expected_length != 0x4000 ) {
        libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                                 "%s:read_if1_chunk: invalid ROM length "
                                 "in file, should be 8192 or 16384 bytes, "
                                 "file has %lu",
                                 __FILE__, 
                                 (unsigned long)expected_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    libspectrum_snap_set_interface1_custom_rom( snap, 1 );

    if( flags & ZXSTIF1F_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

      size_t uncompressed_length = 0;

      libspectrum_error error =
              libspectrum_zlib_inflate( *buffer, data_length - 40, &rom_data,
                                        &uncompressed_length );
      if( error ) return error;

      if( uncompressed_length != expected_length ) {
        libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                                 "%s:read_if1_chunk: invalid ROM length "
                                 "in compressed file, should be %lu, file "
                                 "has %lu",
                                 __FILE__, 
                                 (unsigned long)expected_length,
                                 (unsigned long)uncompressed_length );
        return LIBSPECTRUM_ERROR_UNKNOWN;
      }

      libspectrum_snap_set_interface1_rom( snap, 0, rom_data );
      libspectrum_snap_set_interface1_rom_length( snap, 0,
                                                  uncompressed_length );

      *buffer += data_length - 40;

#else			/* #ifdef HAVE_ZLIB_H */

      libspectrum_print_error(
        LIBSPECTRUM_ERROR_UNKNOWN,
        "%s:read_if1_chunk: zlib needed for decompression\n",
        __FILE__
      );
      return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

    } else {

      if( data_length < 40 + expected_length ) {
        libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                                 "%s:read_if1_chunk: length %lu too short, "
                                 "expected %lu" ,
                                 __FILE__, (unsigned long)data_length,
                                 (unsigned long)40 + expected_length );
        return LIBSPECTRUM_ERROR_UNKNOWN;
      }

      rom_data = libspectrum_new( libspectrum_byte, expected_length );
      memcpy( rom_data, *buffer, expected_length );

      libspectrum_snap_set_interface1_rom_length( snap, 0, expected_length );

      *buffer += expected_length;

    }
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static void
szx_set_custom_rom( libspectrum_snap *snap, int page_no,
                    libspectrum_byte *rom_data, libspectrum_word length )
{
  if( length ) {
    libspectrum_byte *page = libspectrum_new( libspectrum_byte, length );
    memcpy( page, rom_data, length );

    libspectrum_snap_set_roms( snap, page_no, page );
    libspectrum_snap_set_rom_length( snap, page_no, length );
  }
}

static libspectrum_error
szx_extract_roms( libspectrum_snap *snap, libspectrum_byte *rom_data,
                  libspectrum_dword length, libspectrum_dword expected_length )
{
  int i;
  const size_t standard_rom_length = 0x4000;
  size_t num_16k_roms, additional_rom_length;

  if( length != expected_length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                             "%s:szx_extract_roms: invalid ROM length %u, "
                             "expected %u",
                             __FILE__, length, expected_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  num_16k_roms = length / standard_rom_length;
  additional_rom_length = length % standard_rom_length;

  for( i = 0; i < num_16k_roms; i++ )
    szx_set_custom_rom( snap, i, rom_data + (i * standard_rom_length), standard_rom_length );

  /* Timex 2068 machines have a 16k and an 8k ROM, all other machines just have
     multiples of 16k ROMs */
  if( additional_rom_length )
    szx_set_custom_rom( snap, i, rom_data + (i * standard_rom_length), additional_rom_length );

  libspectrum_snap_set_custom_rom_pages( snap, num_16k_roms +
                                         !!additional_rom_length );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_rom_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		const libspectrum_byte **buffer,
		const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_word flags;
  libspectrum_dword expected_length;
  libspectrum_byte *rom_data = NULL; 
  libspectrum_error retval = LIBSPECTRUM_ERROR_NONE;

  if( data_length < 6 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_rom_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_word( buffer );
  expected_length = libspectrum_read_dword( buffer );

  if( flags & ZXSTRF_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

    size_t uncompressed_length = 0;

    libspectrum_error error =
            libspectrum_zlib_inflate( *buffer, data_length - 6, &rom_data,
                                      &uncompressed_length );
    if( error ) return error;

    if( uncompressed_length != expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                               "%s:read_rom_chunk: invalid ROM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
                               __FILE__, 
                               (unsigned long)expected_length,
                               (unsigned long)uncompressed_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    *buffer += data_length - 6;

#else			/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_rom_chunk: zlib needed for decompression\n",
      __FILE__
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

  } else {

    if( data_length < 6 + expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                               "%s:read_rom_chunk: length %lu too short, "
                               "expected %lu" ,
                               __FILE__, (unsigned long)data_length,
                               (unsigned long)6 + expected_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    rom_data = libspectrum_new( libspectrum_byte, expected_length );
    memcpy( rom_data, *buffer, expected_length );
    *buffer += expected_length;
  }

  libspectrum_snap_set_custom_rom( snap, 1 );

  switch ( libspectrum_snap_machine( snap ) ) {
  case LIBSPECTRUM_MACHINE_16:
  case LIBSPECTRUM_MACHINE_48:
  case LIBSPECTRUM_MACHINE_TC2048:
    retval = szx_extract_roms( snap, rom_data, expected_length, 0x4000 );
    break;
  case LIBSPECTRUM_MACHINE_128:
  case LIBSPECTRUM_MACHINE_PLUS2:
  case LIBSPECTRUM_MACHINE_SE:
    retval = szx_extract_roms( snap, rom_data, expected_length, 0x8000 );
    break;
  case LIBSPECTRUM_MACHINE_PLUS2A:
  case LIBSPECTRUM_MACHINE_PLUS3:
  case LIBSPECTRUM_MACHINE_PLUS3E:
    retval = szx_extract_roms( snap, rom_data, expected_length, 0x10000 );
    break;
  case LIBSPECTRUM_MACHINE_PENT:
    /* FIXME: This is a conflict with Fuse - szx specs say Pentagon 128k snaps
       will total 32k, Fuse also has the 'gluck.rom' */
    retval = szx_extract_roms( snap, rom_data, expected_length, 0x8000 );
    break;
  case LIBSPECTRUM_MACHINE_TC2068:
  case LIBSPECTRUM_MACHINE_TS2068:
    retval = szx_extract_roms( snap, rom_data, expected_length, 0x6000 );
    break;
  case LIBSPECTRUM_MACHINE_SCORP:
  case LIBSPECTRUM_MACHINE_PENT512:
  case LIBSPECTRUM_MACHINE_PENT1024:
    retval = szx_extract_roms( snap, rom_data, expected_length, 0x10000 );
    break;
  default:
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                             "%s:read_rom_chunk: don't know correct custom ROM "
                             "length for this machine",
                             __FILE__ );
    retval = LIBSPECTRUM_ERROR_UNKNOWN;
    break;
  }

  libspectrum_free( rom_data );

  return retval;
}

static libspectrum_error
read_zxpr_chunk( libspectrum_snap *snap, libspectrum_word version,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_word flags;

  if( data_length != 2 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_zxpr_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_word( buffer );
  libspectrum_snap_set_zx_printer_active( snap, flags & ZXSTPRF_ENABLED );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_if2r_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{

#ifdef HAVE_ZLIB_H

  libspectrum_byte *buffer2;

  size_t uncompressed_length;

  libspectrum_error error;

  if( data_length < 4 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_if2r_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  /* Skip the compressed length as we never actually use it - bug? */
  libspectrum_read_dword( buffer );

  uncompressed_length = 0x4000;

  error = libspectrum_zlib_inflate( *buffer, data_length - 4, &buffer2,
                                    &uncompressed_length );
  if( error ) return error;

  *buffer += data_length - 4;

  libspectrum_snap_set_interface2_active( snap, 1 );

  libspectrum_snap_set_interface2_rom( snap, 0, buffer2 );

  return LIBSPECTRUM_ERROR_NONE;

#else			/* #ifdef HAVE_ZLIB_H */

  libspectrum_print_error(
    LIBSPECTRUM_ERROR_UNKNOWN,
    "%s:read_if2r_chunk: zlib needed for decompression\n",
    __FILE__
  );
  return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

}

static libspectrum_error
read_dock_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;
  libspectrum_byte writeable;

  error = read_ram_page( &data, &page, buffer, data_length, 0x2000, &flags );
  if( error ) return error;

  if( page > 7 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_dock_chunk: unknown page number %ld",
			     __FILE__, (unsigned long)page );
    libspectrum_free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_dock_active( snap, 1 );

  writeable = flags & ZXSTDOCKF_RAM;

  if( flags & ZXSTDOCKF_EXROMDOCK ) {
    libspectrum_snap_set_dock_ram( snap, page, writeable );
    libspectrum_snap_set_dock_cart( snap, page, data );
  } else {
    libspectrum_snap_set_exrom_ram( snap, page, writeable );
    libspectrum_snap_set_exrom_cart( snap, page, data );
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_dide_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
#ifdef HAVE_ZLIB_H
  libspectrum_error error;
#endif
  libspectrum_word flags;
  libspectrum_byte *eprom_data = NULL;
  const size_t expected_length = 0x2000;

  if( data_length < 4 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "%s:read_dide_chunk: unknown length %lu",
			     __FILE__, (unsigned long)data_length );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = libspectrum_read_word( buffer );
  libspectrum_snap_set_divide_active( snap, 1 );
  libspectrum_snap_set_divide_eprom_writeprotect(
                                      snap,
                                      !!(flags & ZXSTDIVIDE_EPROM_WRITEPROTECT)
                                    );
  libspectrum_snap_set_divide_paged( snap, !!(flags & ZXSTDIVIDE_PAGED) );

  libspectrum_snap_set_divide_control( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_divide_pages( snap, **buffer ); (*buffer)++;

  if( flags & ZXSTDIVIDE_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

    size_t uncompressed_length = 0;

    error = libspectrum_zlib_inflate( *buffer, data_length - 4, &eprom_data,
                                      &uncompressed_length );
    if( error ) return error;

    if( uncompressed_length != expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                               "%s:read_dide_chunk: invalid EPROM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
                               __FILE__,
                               (unsigned long)expected_length,
                               (unsigned long)uncompressed_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    *buffer += data_length - 4;

#else

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_dide_chunk: zlib needed for decompression\n",
      __FILE__
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif

  } else {

    if( data_length < 4 + expected_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                               "%s:read_dide_chunk: length %lu too short, "
                               "expected %lu",
                               __FILE__, (unsigned long)data_length,
                               (unsigned long)4 + expected_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    eprom_data = libspectrum_new( libspectrum_byte, expected_length );
    memcpy( eprom_data, *buffer, expected_length );

    *buffer += expected_length;

  }

  libspectrum_snap_set_divide_eprom( snap, 0, eprom_data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_dirp_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte *data;
  size_t page;
  libspectrum_error error;
  libspectrum_word flags;

  error = read_ram_page( &data, &page, buffer, data_length, 0x2000, &flags );
  if( error ) return error;

  if( page >= SNAPSHOT_DIVIDE_PAGES ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_CORRUPT,
			     "%s:read_dirp_chunk: unknown page number %lu",
			     __FILE__, (unsigned long)page );
    libspectrum_free( data );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  libspectrum_snap_set_divide_ram( snap, page, data );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_snet_memory( libspectrum_snap *snap, const libspectrum_byte **buffer,
  int compressed, size_t *data_remaining,
  void (*setter)(libspectrum_snap*, int, libspectrum_byte*) )
{
  size_t data_length;
  libspectrum_byte *data_out;
  const libspectrum_byte *data;

  if( *data_remaining < 4 ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_snet_memory: not enough data for length", __FILE__ );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  data_length = libspectrum_read_dword( buffer );
  *data_remaining -= 4;

  if( *data_remaining < data_length ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_snet_memory: not enough data", __FILE__ );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }
  *data_remaining -= data_length;

  if( compressed ) {

#ifdef HAVE_ZLIB_H
    libspectrum_error error;
    size_t uncompressed_length = 0;
    libspectrum_byte *uncompressed_data;

    error = libspectrum_zlib_inflate( *buffer, data_length, &uncompressed_data,
        &uncompressed_length );
    if( error ) return error;

    *buffer += data_length;

    if( uncompressed_length != 0x20000 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
          "%s:read_snet_memory: data decompressed to %lu but should be 0x20000",
          __FILE__, (unsigned long)uncompressed_length );
      libspectrum_free( uncompressed_data );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    data = uncompressed_data;

#else

    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
        "%s:read_snet_memory: zlib needed for decompression\n", __FILE__ );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif

  } else {
    if( data_length != 0x20000 ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
          "%s:read_snet_memory: data has length %lu but should be 0x20000",
          __FILE__, (unsigned long)data_length );
      return LIBSPECTRUM_ERROR_NONE;
    }

    data = *buffer;
    *buffer += data_length;
  }

  data_out = libspectrum_new( libspectrum_byte, 0x20000 );
  memcpy( data_out, data, 0x20000 );
  setter( snap, 0, data_out );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_snet_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_word flags;
  libspectrum_byte *w5100;

  if( data_length < 54 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_snet_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_spectranet_active( snap, 1 );

  flags = libspectrum_read_word( buffer );
  libspectrum_snap_set_spectranet_paged( snap, !!( flags & ZXSTSNET_PAGED ) );
  libspectrum_snap_set_spectranet_paged_via_io( snap,
    !!( flags & ZXSTSNET_PAGED_VIA_IO ) );
  libspectrum_snap_set_spectranet_programmable_trap_active( snap,
    !!( flags & ZXSTSNET_PROGRAMMABLE_TRAP_ACTIVE ) );
  libspectrum_snap_set_spectranet_programmable_trap_msb( snap,
    !!( flags & ZXSTSNET_PROGRAMMABLE_TRAP_MSB ) );
  libspectrum_snap_set_spectranet_all_traps_disabled( snap,
    !!( flags & ZXSTSNET_ALL_DISABLED ) );
  libspectrum_snap_set_spectranet_rst8_trap_disabled( snap,
    !!( flags & ZXSTSNET_RST8_DISABLED ) );
  libspectrum_snap_set_spectranet_deny_downstream_a15( snap,
    !!( flags & ZXSTSNET_DENY_DOWNSTREAM_A15 ) );
  libspectrum_snap_set_spectranet_nmi_flipflop( snap,
    !!( flags & ZXSTSNET_NMI_FLIPFLOP ) );

  libspectrum_snap_set_spectranet_page_a( snap, **buffer ); (*buffer)++;
  libspectrum_snap_set_spectranet_page_b( snap, **buffer ); (*buffer)++;

  libspectrum_snap_set_spectranet_programmable_trap( snap,
    libspectrum_read_word( buffer ) );

  w5100 = libspectrum_new( libspectrum_byte, 0x30 );
  libspectrum_snap_set_spectranet_w5100( snap, 0, w5100 );
  memcpy( w5100, *buffer, 0x30 );
  (*buffer) += 0x30;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_snef_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte flags;
  int flash_compressed;
  libspectrum_error error;
  size_t data_remaining;

  if( data_length < 5 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_snef_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = **buffer; (*buffer)++;
  flash_compressed = flags & ZXSTSNEF_FLASH_COMPRESSED;

  data_remaining = data_length - 1;

  error = read_snet_memory( snap, buffer, flash_compressed, &data_remaining,
    libspectrum_snap_set_spectranet_flash );
  if( error )
    return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_sner_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
		 const libspectrum_byte **buffer,
		 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
  libspectrum_byte flags;
  int ram_compressed;
  libspectrum_error error;
  size_t data_remaining;

  if( data_length < 5 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_sner_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = **buffer; (*buffer)++;
  ram_compressed = flags & ZXSTSNER_RAM_COMPRESSED;

  data_remaining = data_length - 1;

  error = read_snet_memory( snap, buffer, ram_compressed, &data_remaining,
    libspectrum_snap_set_spectranet_ram );
  if( error )
    return error;

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_mfce_chunk( libspectrum_snap *snap, libspectrum_word version GCC_UNUSED,
                 const libspectrum_byte **buffer,
                 const libspectrum_byte *end GCC_UNUSED, size_t data_length,
                 szx_context *ctx GCC_UNUSED )
{
#ifdef HAVE_ZLIB_H
  libspectrum_error error;
#endif
  libspectrum_byte *ram_data = NULL;
  libspectrum_byte multiface_model;
  libspectrum_byte flags;
  int capabilities;
  size_t expected_ram_length, disc_ram_length;

  if( data_length < 2 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "read_mfce_chunk: length %lu too short", (unsigned long)data_length
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  libspectrum_snap_set_multiface_active( snap, 1 );

  multiface_model = **buffer; (*buffer)++;

  if( multiface_model == ZXSTMFM_1 )
    libspectrum_snap_set_multiface_model_one( snap, 1 );
  else if ( multiface_model == ZXSTMFM_128 ) {
    capabilities =
      libspectrum_machine_capabilities( libspectrum_snap_machine( snap ) );

    if( capabilities & LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_MEMORY )
      libspectrum_snap_set_multiface_model_3( snap, 1 );
    else
      libspectrum_snap_set_multiface_model_128( snap, 1 );
  }

  flags = **buffer; (*buffer)++;
  libspectrum_snap_set_multiface_paged( snap, !!( flags & ZXSTMF_PAGEDIN ) );
  libspectrum_snap_set_multiface_software_lockout( snap,
    !!( flags & ZXSTMF_SOFTWARELOCKOUT ) );
  libspectrum_snap_set_multiface_red_button_disabled( snap,
    !!( flags & ZXSTMF_REDBUTTONDISABLED ) );
  libspectrum_snap_set_multiface_disabled( snap,
    !!( flags & ZXSTMF_DISABLED ) );

  expected_ram_length = flags & ZXSTMF_16KRAMMODE ? 0x4000 : 0x2000;
  disc_ram_length = data_length - 2;

  if( flags & ZXSTMF_COMPRESSED ) {

#ifdef HAVE_ZLIB_H

    size_t uncompressed_length = 0;

    error = libspectrum_zlib_inflate( *buffer, disc_ram_length, &ram_data,
                                      &uncompressed_length );
    if( error ) return error;

    if( uncompressed_length != expected_ram_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                               "%s:read_mfce_chunk: invalid RAM length "
                               "in compressed file, should be %lu, file "
                               "has %lu",
                               __FILE__,
                               (unsigned long)expected_ram_length,
                               (unsigned long)uncompressed_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    *buffer += disc_ram_length;

#else			/* #ifdef HAVE_ZLIB_H */

    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "%s:read_mfce_chunk: zlib needed for decompression\n",
      __FILE__
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;

#endif			/* #ifdef HAVE_ZLIB_H */

  } else {

    if( disc_ram_length != expected_ram_length ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
                               "%s:read_mfce_chunk: invalid RAM length "
                               "in uncompressed file, should be %lu, file "
                               "has %lu",
                               __FILE__,
                               (unsigned long)expected_ram_length,
                               (unsigned long)disc_ram_length );
      return LIBSPECTRUM_ERROR_UNKNOWN;
    }

    ram_data = libspectrum_new( libspectrum_byte, expected_ram_length );
    memcpy( ram_data, *buffer, expected_ram_length );
    *buffer += expected_ram_length;
  }

  libspectrum_snap_set_multiface_ram( snap, 0, ram_data );
  libspectrum_snap_set_multiface_ram_length( snap, 0, expected_ram_length );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
skip_chunk( libspectrum_snap *snap GCC_UNUSED,
	    libspectrum_word version GCC_UNUSED,
	    const libspectrum_byte **buffer,
	    const libspectrum_byte *end GCC_UNUSED,
	    size_t data_length,
            szx_context *ctx GCC_UNUSED )
{
  *buffer += data_length;
  return LIBSPECTRUM_ERROR_NONE;
}

struct read_chunk_t {

  const char *id;
  read_chunk_fn function;

};

static struct read_chunk_t read_chunks[] = {

  { ZXSTBID_AY,		         read_ay_chunk   },
  { ZXSTBID_BETA128,	         read_b128_chunk },
  { ZXSTBID_BETADISK,	         skip_chunk      },
  { ZXSTBID_COVOX,	         read_covx_chunk },
  { ZXSTBID_CREATOR,	         read_crtr_chunk },
  { ZXSTBID_DIVIDE,	         read_dide_chunk },
  { ZXSTBID_DIVIDERAMPAGE,       read_dirp_chunk },
  { ZXSTBID_DOCK,	         read_dock_chunk },
  { ZXSTBID_DSKFILE,	         skip_chunk      },
  { ZXSTBID_LEC,                 skip_chunk      },
  { ZXSTBID_LECRAMPAGE,          skip_chunk      },
  { ZXSTBID_GS,		         skip_chunk      },
  { ZXSTBID_GSRAMPAGE,	         skip_chunk      },
  { ZXSTBID_IF1,	         read_if1_chunk  },
  { ZXSTBID_IF2ROM,	         read_if2r_chunk },
  { ZXSTBID_JOYSTICK,	         read_joy_chunk  },
  { ZXSTBID_KEYBOARD,	         read_keyb_chunk },
  { ZXSTBID_MICRODRIVE,	         skip_chunk      },
  { ZXSTBID_MOUSE,	         read_amxm_chunk },
  { ZXSTBID_MULTIFACE,	         read_mfce_chunk },
  { ZXSTBID_OPUS,	         read_opus_chunk },
  { ZXSTBID_OPUSDISK,	         skip_chunk      },
  { ZXSTBID_PALETTE,	         skip_chunk      },
  { ZXSTBID_PLUS3DISK,	         skip_chunk      },
  { ZXSTBID_PLUSD,	         read_plsd_chunk },
  { ZXSTBID_PLUSDDISK,	         skip_chunk      },
  { ZXSTBID_RAMPAGE,	         read_ramp_chunk },
  { ZXSTBID_ROM,	         read_rom_chunk  },
  { ZXSTBID_SIMPLEIDE,	         read_side_chunk },
  { ZXSTBID_SPECDRUM,	         read_drum_chunk },
  { ZXSTBID_SPECREGS,	         read_spcr_chunk },
  { ZXSTBID_SPECTRANET,          read_snet_chunk },
  { ZXSTBID_SPECTRANETFLASHPAGE, read_snef_chunk },
  { ZXSTBID_SPECTRANETRAMPAGE,   read_sner_chunk },
  { ZXSTBID_TIMEXREGS,	         read_scld_chunk },
  { ZXSTBID_USPEECH,	         skip_chunk      },
  { ZXSTBID_Z80REGS,	         read_z80r_chunk },
  { ZXSTBID_ZXATASPRAMPAGE,      read_atrp_chunk },
  { ZXSTBID_ZXATASP,	         read_zxat_chunk },
  { ZXSTBID_ZXCF,	         read_zxcf_chunk },
  { ZXSTBID_ZXCFRAMPAGE,         read_cfrp_chunk },
  { ZXSTBID_ZXPRINTER,	         read_zxpr_chunk },
  { ZXSTBID_ZXTAPE,	         skip_chunk      },

};

static libspectrum_error
read_chunk_header( char *id, libspectrum_dword *data_length, 
		   const libspectrum_byte **buffer,
		   const libspectrum_byte *end )
{
  if( end - *buffer < 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "szx_read_chunk_header: not enough data for chunk header"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  memcpy( id, *buffer, 4 ); id[4] = '\0'; *buffer += 4;
  *data_length = libspectrum_read_dword( buffer );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
read_chunk( libspectrum_snap *snap, libspectrum_word version,
	    const libspectrum_byte **buffer, const libspectrum_byte *end,
            szx_context *ctx )
{
  char id[5];
  libspectrum_dword data_length;
  libspectrum_error error;
  size_t i; int done;

  error = read_chunk_header( id, &data_length, buffer, end );
  if( error ) return error;

  if( end - *buffer < data_length ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "szx_read_chunk: chunk length goes beyond end of file"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  done = 0;

  for( i = 0; !done && i < ARRAY_SIZE( read_chunks ); i++ ) {

    if( !memcmp( id, read_chunks[i].id, 4 ) ) {
      error = read_chunks[i].function( snap, version, buffer, end,
				       data_length, ctx );
      if( error ) return error;
      done = 1;
    }

  }

  if( !done ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_UNKNOWN,
			     "szx_read_chunk: unknown chunk id '%s'", id );
    *buffer += data_length;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

/* A mapping from an SZX machine IDs to a libspectrum machine IDs */
struct machine_mapping_t {
  /* The constant used in the SZX format to identify this machine */
  szx_machine_type szx;

  /* The constant used in libspectrum to identify this machine */
  libspectrum_machine libspectrum;
};

/* The mappings from SZX to libspectrum machine IDs */
static struct machine_mapping_t machine_mappings[] = {
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

libspectrum_error
libspectrum_szx_read( libspectrum_snap *snap, const libspectrum_byte *buffer,
		      size_t length )
{
  libspectrum_word version;
  libspectrum_byte machine;
  libspectrum_byte flags;

  libspectrum_error error;
  const libspectrum_byte *end = buffer + length;
  szx_context *ctx;
  size_t i;
  int done = 0;

  if( end - buffer < 8 ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_CORRUPT,
      "libspectrum_szx_read: not enough data for SZX header"
    );
    return LIBSPECTRUM_ERROR_CORRUPT;
  }

  if( memcmp( buffer, signature, signature_length ) ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_SIGNATURE,
      "libspectrum_szx_read: wrong signature"
    );
    return LIBSPECTRUM_ERROR_SIGNATURE;
  }
  buffer += signature_length;

  version = (*buffer++) << 8; version |= *buffer++;

  machine = *buffer++;

  for( i = 0; !done && i < ARRAY_SIZE( machine_mappings ); i++ ) {
    if( machine == machine_mappings[i].szx ) {
      libspectrum_snap_set_machine( snap, machine_mappings[i].libspectrum );
      done = 1;
    }
  }

  if( !done ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_UNKNOWN,
      "libspectrum_szx_read: unknown machine type %d", (int)*buffer
    );
    return LIBSPECTRUM_ERROR_UNKNOWN;
  }

  flags = *buffer++;

  switch( machine ) {

  case SZX_MACHINE_16:
  case SZX_MACHINE_48:
  case SZX_MACHINE_48_NTSC:
  case SZX_MACHINE_128:
    libspectrum_snap_set_late_timings( snap, flags & ZXSTMF_ALTERNATETIMINGS );
    break;

  default:
    break;
  }

  ctx = libspectrum_new( szx_context, 1 );
  ctx->swap_af = 0;

  while( buffer < end ) {
    error = read_chunk( snap, version, &buffer, end, ctx );
    if( error ) {
      libspectrum_free( ctx );
      return error;
    }
  }

  libspectrum_free( ctx );
  return LIBSPECTRUM_ERROR_NONE;
}
