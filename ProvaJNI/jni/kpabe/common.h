/*!	\file common.h
 *
 *	\brief Routines for the KP-ABE tools.
 *	Include glib.h and pbc.h before including this file.
 *
 *	Copyright 2011 Yao Zheng.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
	TODO if ssl sha1 not available, use built in one (sha1.c)
*/

char*       suck_file_str( char* file );
char*       suck_stdin();
GByteArray* suck_file( char* file );

void        spit_file( char* file, GByteArray* b, int free );

void read_kpabe_file( char* file,    GByteArray** cph_buf,
											int* file_len, GByteArray** aes_buf );

void write_kpabe_file( char* file,   GByteArray* cph_buf,
											 int file_len, GByteArray* aes_buf );

void die(char* fmt, ...);

GByteArray* aes_128_cbc_encrypt( GByteArray* pt, element_t k );
GByteArray* aes_128_cbc_decrypt( GByteArray* ct, element_t k );

#define KPABE_VERSION PACKAGE_NAME "%s " PACKAGE_VERSION "\n" \
"\n" \
"This is free software released under the GPL, see the source for copying\n" \
"conditions. There is NO warranty; not even for MERCHANTABILITY or FITNESS\n" \
"FOR A PARTICULAR PURPOSE.\n" \
"\n" \
"Report bugs to Yao Zheng <zhengyao@wpi.edu>.\n"
