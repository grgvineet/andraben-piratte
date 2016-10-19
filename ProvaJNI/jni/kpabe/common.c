/*!	\file common.c
 *
 *	\brief Routines for the KP-ABE tools.
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

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib/glib.h>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <include/pbc.h>
#include "common.h"

/*!
 * Initialize parameters for AES symmetric-key encryption
 *
 * @param k				Sercet message from KP-ABE
 * @param enc			Set encrypt key when enc = 1; Set decrypt key when enc = 0;
 * @param key			Pointer to the AES_KEY
 * @param iv			Salt
 * @return				None
 */

void
init_aes( element_t k, int enc, AES_KEY* key, unsigned char* iv )
{
  int key_len;
  unsigned char* key_buf;

  key_len = element_length_in_bytes(k) < 17 ? 17 : element_length_in_bytes(k);
  key_buf = (unsigned char*) malloc(key_len);
  element_to_bytes(key_buf, k);

  if( enc )
    AES_set_encrypt_key(key_buf + 1, 128, key);
  else
    AES_set_decrypt_key(key_buf + 1, 128, key);
  free(key_buf);

  memset(iv, 0, 16);
}

/*!
 * AES 128bit CBC mode encryption
 *
 * @param pt			GByteArrary of plaintext
 * @param k				Sercet message from KP-ABE
 * @return				GByteArray of ciphertext
 */

GByteArray*
aes_128_cbc_encrypt( GByteArray* pt, element_t k )
{
  AES_KEY key;
  unsigned char iv[16];
  GByteArray* ct;
  guint8 len[4];
  guint8 zero;

  init_aes(k, 1, &key, iv);

  /* TODO make less crufty */

  /* stuff in real length (big endian) before padding */
  len[0] = (pt->len & 0xff000000)>>24;
  len[1] = (pt->len & 0xff0000)>>16;
  len[2] = (pt->len & 0xff00)>>8;
  len[3] = (pt->len & 0xff)>>0;
  g_byte_array_prepend(pt, len, 4);

  /* pad out to multiple of 128 bit (16 byte) blocks */
  zero = 0;
  while( pt->len % 16 )
    g_byte_array_append(pt, &zero, 1);

  ct = g_byte_array_new();
  g_byte_array_set_size(ct, pt->len);

  AES_cbc_encrypt(pt->data, ct->data, pt->len, &key, iv, AES_ENCRYPT);

  return ct;
}

/*!
 * AES 128bit CBC mode decryption
 *
 * @param pt			GByteArrary of ciphertext
 * @param k				Sercet message from KP-ABE
 * @return				GByteArray of plaintext
 */

GByteArray*
aes_128_cbc_decrypt( GByteArray* ct, element_t k )
{
  AES_KEY key;
  unsigned char iv[16];
  GByteArray* pt;
  unsigned int len;

  init_aes(k, 0, &key, iv);

  pt = g_byte_array_new();
  g_byte_array_set_size(pt, ct->len);

  AES_cbc_encrypt(ct->data, pt->data, ct->len, &key, iv, AES_DECRYPT);

  /* TODO make less crufty */

  /* get real length */
  len = 0;
  len = len
    | ((pt->data[0])<<24) | ((pt->data[1])<<16)
    | ((pt->data[2])<<8)  | ((pt->data[3])<<0);
  g_byte_array_remove_index(pt, 0);
  g_byte_array_remove_index(pt, 0);
  g_byte_array_remove_index(pt, 0);
  g_byte_array_remove_index(pt, 0);

  /* truncate any garbage from the padding */
  g_byte_array_set_size(pt, len);

  return pt;
}

/*!
 * Open file with read mode or die
 *
 * @param file			File name
 * @return				File handler
 */

FILE*
fopen_read_or_die( char* file )
{
	FILE* f;

	if( !(f = fopen(file, "r")) )
		die("can't read file: %s\n", file);

	return f;
}

/*!
 * Open file with write mode or die
 *
 * @param file			File name
 * @return				File handler
 */

FILE*
fopen_write_or_die( char* file )
{
	FILE* f;

	if( !(f = fopen(file, "w")) )
		die("can't write file: %s\n", file);

	return f;
}

/*!
 * Open file and turn it into a GByteArray
 *
 * @param file			File name
 * @return				GByteArray
 */

GByteArray*
suck_file( char* file )
{
	FILE* f;
	GByteArray* a;
	struct stat s;

	a = g_byte_array_new();
	stat(file, &s);
	g_byte_array_set_size(a, s.st_size);

	f = fopen_read_or_die(file);
	fread(a->data, 1, s.st_size, f);
	fclose(f);

	return a;
}

/*!
 * Open file and turn it into a String
 *
 * @param file			File name
 * @return				String
 */

char*
suck_file_str( char* file )
{
	GByteArray* a;
	char* s;
	unsigned char zero;

	a = suck_file(file);
	zero = 0;
	g_byte_array_append(a, &zero, 1);
	s = (char*) a->data;
	g_byte_array_free(a, 0);

	return s;
}

/*!
 * Get input from stdin and turn it into a String
 *
 * @return				String
 */

char*
suck_stdin()
{
	GString* s;
	char* r;
	int c;

	s = g_string_new("");
	while( (c = fgetc(stdin)) != EOF )
		g_string_append_c(s, c);

	r = s->str;
	g_string_free(s, 0);

	return r;
}

/*
 * Output GByteArray into a file, if free is one, free the GByteArray
 *
 * @param file			File name
 * @param b				GByteArray
 * @param free			Free flag
 * @return				None
 */

void
spit_file( char* file, GByteArray* b, int free )
{
	FILE* f;

	f = fopen_write_or_die(file);
	fwrite(b->data, 1, b->len, f);
	fclose(f);

	if( free )
		g_byte_array_free(b, 1);
}

/*!
 * Read serialized KP-ABE ciphertext from input file and put it into a buffer
 * Read AES encrypted data from input file and put it into a buffer
 *
 * @param file			File name
 * @param cph_buf		Pointer to Ciphertext buffer
 * @param file_len		Read file len
 * @param aes_buf		Pointer to AES buffer
 * @return				None
 */

void read_kpabe_file( char* file,    GByteArray** cph_buf,
											int* file_len, GByteArray** aes_buf )
{
	FILE* f;
	int i;
	int len;

	*cph_buf = g_byte_array_new();
	*aes_buf = g_byte_array_new();

	f = fopen_read_or_die(file);

	/* read real file len as 32-bit big endian int */
	*file_len = 0;
	for( i = 3; i >= 0; i-- )
		*file_len |= fgetc(f)<<(i*8);

	/* read aes buf */
	len = 0;
	for( i = 3; i >= 0; i-- )
		len |= fgetc(f)<<(i*8);
	g_byte_array_set_size(*aes_buf, len);
	fread((*aes_buf)->data, 1, len, f);

	/* read cph buf */
	len = 0;
	for( i = 3; i >= 0; i-- )
		len |= fgetc(f)<<(i*8);
	g_byte_array_set_size(*cph_buf, len);
	fread((*cph_buf)->data, 1, len, f);

	fclose(f);
}

/*!
 * write serialized KP-ABE ciphertext from buffer to file
 * Read AES encrypted data from buffer to file
 *
 * @param file			File name
 * @param cph_buf		Pointer to Ciphertext buffer
 * @param file_len		Write file len
 * @param aes_buf		Pointer to AES buffer
 * @return				None
 */


void
write_kpabe_file( char* file,   GByteArray* cph_buf,
									int file_len, GByteArray* aes_buf )
{
	FILE* f;
	int i;

	f = fopen_write_or_die(file);

	/* write real file len as 32-bit big endian int */
	for( i = 3; i >= 0; i-- )
		fputc((file_len & 0xff<<(i*8))>>(i*8), f);

	/* write aes_buf */
	for( i = 3; i >= 0; i-- )
		fputc((aes_buf->len & 0xff<<(i*8))>>(i*8), f);
	fwrite(aes_buf->data, 1, aes_buf->len, f);

	/* write cph_buf */
	for( i = 3; i >= 0; i-- )
		fputc((cph_buf->len & 0xff<<(i*8))>>(i*8), f);
	fwrite(cph_buf->data, 1, cph_buf->len, f);

	fclose(f);
}

/*!
 * Terminate program with error message
 *
 * @param fmt			Error message
 * @return				None
 */

void
die(char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(1);
}
