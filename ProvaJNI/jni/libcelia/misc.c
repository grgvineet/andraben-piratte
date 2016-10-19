/*!	\file celia.h
 *
 *	\brief Miscellaneous Utility routines related to GPSW07 scheme
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <glib/glib.h>
#include <include/pbc.h>
#include "celia.h"

/********************************************************************************
 * Utility functions
 ********************************************************************************/

/*!
 * Serialize a 32 bit unsign integer to a GByteArray.
 *
 * @param b					GByteArray.
 * @param k					Unsign integer
 * @return					None
 */

void
serialize_uint32( GByteArray* b, uint32_t k )
{
	int i;
	guint8 byte;

	for( i = 3; i >= 0; i-- )
	{
		byte = (k & 0xff<<(i*8))>>(i*8);
		g_byte_array_append(b, &byte, 1);
	}
}

/*!
 * Unserialize a 32 bit unsign integer from a GByteArray.
 *
 * @param b					GByteArray.
 * @param offset			offset of the integer
 * @return					Unsign integer
 */

uint32_t
unserialize_uint32( GByteArray* b, int* offset )
{
	int i;
	uint32_t r;

	r = 0;
	for( i = 3; i >= 0; i-- )
		r |= (b->data[(*offset)++])<<(i*8);

	return r;
}

/*!
 * Serialize a PBC element_t to a GByteArray.
 *
 * @param b					GByteArray.
 * @param e					element_t data type
 * @return					None
 */

void
serialize_element( GByteArray* b, element_t e )
{
	uint32_t len;
	unsigned char* buf;

	len = element_length_in_bytes(e);
	serialize_uint32(b, len);

	buf = (unsigned char*) malloc(len);
	element_to_bytes(buf, e);
	g_byte_array_append(b, buf, len);
	free(buf);
}

/*!
 * Unserialize a 32 PBC element_t from a GByteArray.
 *
 * @param b					GByteArray.
 * @param offset			offset of element_t within GByteArray
 * @param e					element_t
 * @return					None
 */

void
unserialize_element( GByteArray* b, int* offset, element_t e )
{
	uint32_t len;
	unsigned char* buf;

	len = unserialize_uint32(b, offset);

	buf = (unsigned char*) malloc(len);
	memcpy(buf, b->data + *offset, len);
	*offset += len;

	element_from_bytes(e, buf);
	free(buf);
}

/*!
 * Serialize a String to a GByteArray.
 *
 * @param b					GByteArray.
 * @param s					String
 * @return					None
 */

void
serialize_string( GByteArray* b, char* s )
{
	g_byte_array_append(b, (unsigned char*) s, strlen(s) + 1);
}

/*!
 * Unserialize a String from a GByteArray.
 *
 * @param b					GByteArray.
 * @param offset			offset of string within GByteArray
 * @return					String
 */

char*
unserialize_string( GByteArray* b, int* offset )
{
	GString* s;
	char* r;
	char c;

	s = g_string_sized_new(32);
	while( 1 )
	{
		c = b->data[(*offset)++];
		if( c && c != EOF )
			g_string_append_c(s, c);
		else
			break;
	}

	r = s->str;
	g_string_free(s, 0);

	return r;
}

/*!
 * serialize a policy data structure to a GByteArray.
 *
 * @param b					GByteArray.
 * @param p					Policy data structure
 * @return					None
 */

void
serialize_policy( GByteArray* b, kpabe_policy_t* p )
{
	int i;

	serialize_uint32(b, (uint32_t) p->k);

	serialize_uint32(b, (uint32_t) p->children->len);
	if( p->children->len == 0 )
	{
		serialize_string( b, p->attr);
		serialize_element(b, p->D);
	}
	else
		for( i = 0; i < p->children->len; i++ )
			serialize_policy(b, g_ptr_array_index(p->children, i));
}

/*!
 * Unserialize a policy data structure from a GByteArray using the paring parameter
 * from the public data structure
 *
 * @param pub				Public data structure
 * @param b					GByteArray.
 * @param offset			offset of policy data structure within GByteArray
 * @return					Policy data structure
 */

kpabe_policy_t*
unserialize_policy( kpabe_pub_t* pub, GByteArray* b, int* offset )
{
	int i;
	int n;
	kpabe_policy_t* p;

	p = (kpabe_policy_t*) malloc(sizeof(kpabe_policy_t));

	p->k = (int) unserialize_uint32(b, offset);
	p->attr = 0;
	p->children = g_ptr_array_new();

	n = unserialize_uint32(b, offset);
	if( n == 0 )
	{
		p->attr = unserialize_string(b, offset);
		element_init_G1(p->D,  pub->p);
		unserialize_element(b, offset, p->D);
	}
	else
		for( i = 0; i < n; i++ )
			g_ptr_array_add(p->children, unserialize_policy(pub, b, offset));

	return p;
}

/*!
 * Serialize a public key data structure to a GByteArray.
 *
 * @param pub				Public key data structure
 * @return					GByteArray
 */

GByteArray*
kpabe_pub_serialize( kpabe_pub_t* pub )
{
	GByteArray* b;
	int i;

	b = g_byte_array_new();
	serialize_string(b,  pub->pairing_desc);
	serialize_element(b, pub->g);
	serialize_element(b, pub->Y);
	serialize_uint32( b, pub->comps->len);

	for( i = 0; i < pub->comps->len; i++ )
	{
		serialize_string( b, g_array_index(pub->comps, kpabe_pub_comp_t, i).attr);
		serialize_element(b, g_array_index(pub->comps, kpabe_pub_comp_t, i).T);
	}

	return b;
}

/*!
 * Unserialize a public key data structure from a GByteArray. if free is true,
 * free the GByteArray
 *
 * @param b					GByteArray
 * @param free				Free flag
 * @return					Public key data structure
 */

kpabe_pub_t*
kpabe_pub_unserialize( GByteArray* b, int free )
{
	kpabe_pub_t* pub;
	int offset;
	int len;
	int i;

	pub = (kpabe_pub_t*) malloc(sizeof(kpabe_pub_t));
	offset = 0;

	pub->pairing_desc = unserialize_string(b, &offset);
	pairing_init_set_buf(pub->p, pub->pairing_desc, strlen(pub->pairing_desc));

	element_init_G1(pub->g,           pub->p);
	element_init_GT(pub->Y, 		  pub->p);

	unserialize_element(b, &offset, pub->g);
	unserialize_element(b, &offset, pub->Y);

	pub->comps = g_array_new(0, 1, sizeof(kpabe_pub_comp_t));
	len = unserialize_uint32(b, &offset);

	for( i = 0; i < len; i++ )
	{
		kpabe_pub_comp_t c;

		c.attr = unserialize_string(b, &offset);

		element_init_G1(c.T,  pub->p);

		unserialize_element(b, &offset, c.T);

		g_array_append_val(pub->comps, c);
	}

	if( free )
		g_byte_array_free(b, 1);

	return pub;
}

/*!
 * Serialize a master key data structure to a GByteArray.
 *
 * @param msk				Master key data structure
 * @return					GByteArray
 */

GByteArray*
kpabe_msk_serialize( kpabe_msk_t* msk )
{
	GByteArray* b;
	int i;

	b = g_byte_array_new();
	serialize_element(b, msk->y);
	serialize_uint32( b, msk->comps->len);

	for( i = 0; i < msk->comps->len; i++ )
	{
		serialize_string( b, g_array_index(msk->comps, kpabe_msk_comp_t, i).attr);
		serialize_element(b, g_array_index(msk->comps, kpabe_msk_comp_t, i).t);
	}

	return b;
}

/*!
 * Unserialize a master key data structure from a GByteArray. if free is true,
 * free the GByteArray
 *
 * @param pub				Public key data structure
 * @param b					GByteArray
 * @param free				Free flag
 * @return					Master key data structure
 */

kpabe_msk_t*
kpabe_msk_unserialize( kpabe_pub_t* pub, GByteArray* b, int free )
{
	kpabe_msk_t* msk;
	int offset;
	int len;
	int i;

	msk = (kpabe_msk_t*) malloc(sizeof(kpabe_msk_t));
	offset = 0;

	element_init_Zr(msk->y, pub->p);
	unserialize_element(b, &offset, msk->y);

	msk->comps = g_array_new(0, 1, sizeof(kpabe_msk_comp_t));
	len = unserialize_uint32(b, &offset);

	for( i = 0; i < len; i++ )
	{
		kpabe_msk_comp_t c;

		c.attr = unserialize_string(b, &offset);

		element_init_Zr(c.t,  pub->p);

		unserialize_element(b, &offset, c.t);

		g_array_append_val(msk->comps, c);
	}

	if( free )
		g_byte_array_free(b, 1);

	return msk;
}

/*!
 * Serialize a ciphertext key data structure to a GByteArray.
 *
 * @param cph				Ciphertext data structure
 * @return					GByteArray
 */

GByteArray*
kpabe_cph_serialize( kpabe_cph_t* cph )
{
	GByteArray* b;
	int i;

	b = g_byte_array_new();
	serialize_element(b, cph->Ep);
	serialize_uint32( b, cph->comps->len);

	for( i = 0; i < cph->comps->len; i++ )
	{
		serialize_string( b, g_array_index(cph->comps, kpabe_cph_comp_t, i).attr);
		serialize_element(b, g_array_index(cph->comps, kpabe_cph_comp_t, i).E);
	}

	return b;
}

/*!
 * Unserialize a ciphertext data structure from a GByteArray. if free is true,
 * free the GByteArray
 *
 * @param pub				Public key data structure
 * @param b					GByteArray
 * @param free				Free flag
 * @return					Ciphertext key data structure
 */

kpabe_cph_t*
kpabe_cph_unserialize( kpabe_pub_t* pub, GByteArray* b, int free )
{
	kpabe_cph_t* cph;
	int i;
	int len;
	int offset;

	cph = (kpabe_cph_t*) malloc(sizeof(kpabe_cph_t));
	offset = 0;

	element_init_GT(cph->Ep, pub->p);
	unserialize_element(b, &offset, cph->Ep);

	cph->comps = g_array_new(0, 1, sizeof(kpabe_cph_comp_t));
	len = unserialize_uint32(b, &offset);

	for( i = 0; i < len; i++ )
	{
		kpabe_cph_comp_t c;

		c.attr = unserialize_string(b, &offset);

		element_init_G1(c.E,  pub->p);

		unserialize_element(b, &offset, c.E);

		g_array_append_val(cph->comps, c);
	}

	return cph;
}

/*!
 * Serialize a private key data structure to a GByteArray.
 *
 * @param prv				Private key data structure
 * @return					GByteArray
 */

GByteArray*
kpabe_prv_serialize( kpabe_prv_t* prv )
{
	GByteArray* b;

	b = g_byte_array_new();
	serialize_policy( b, prv->p);

	return b;
}

/*!
 * Unserialize a ciphertext data structure from a GByteArray. if free is true,
 * free the GByteArray
 *
 * @param b					GByteArray
 * @param free				Free flag
 * @return					Private key data structure
 */

kpabe_prv_t*
kpabe_prv_unserialize( kpabe_pub_t* pub, GByteArray* b, int free )
{
	kpabe_prv_t* prv;
	int offset;

	prv = (kpabe_prv_t*) malloc(sizeof(kpabe_prv_t));
	offset = 0;

	prv->p = unserialize_policy(pub, b, &offset);

	if( free )
		g_byte_array_free(b, 1);

	return prv;
}

/*!
 * Free a policy date structure
 *
 * @param					Policy data structure
 * @return					None
 */

void
kpabe_policy_free( kpabe_policy_t* p )
{
	int i;

	if( p->attr )
	{
		free(p->attr);
		element_clear(p->D);
	}

	for( i = 0; i < p->children->len; i++ )
		kpabe_policy_free(g_ptr_array_index(p->children, i));

	g_ptr_array_free(p->children, 1);

	free(p);
}

/*!
 * Free a public key date structure
 *
 * @param					Public key data structure
 * @return					None
 */

void
kpabe_pub_free( kpabe_pub_t* pub )
{
	int i;

	element_clear(pub->g);
	element_clear(pub->Y);
	pairing_clear(pub->p);
	free(pub->pairing_desc);

	for( i = 0; i < pub->comps->len; i++ )
	{
		kpabe_pub_comp_t c;

		c = g_array_index(pub->comps, kpabe_pub_comp_t, i);
		c.attr = NULL;
		element_clear(c.T);
	}
	g_array_free(pub->comps, 1);

	free(pub);
}

/*!
 * Free a master key date structure
 *
 * @param					Master key data structure
 * @return					None
 */

void
kpabe_msk_free( kpabe_msk_t* msk )
{
	int i;

	element_clear(msk->y);

	for( i = 0; i < msk->comps->len; i++ )
	{
		kpabe_msk_comp_t c;

		c = g_array_index(msk->comps, kpabe_msk_comp_t, i);
		c.attr = NULL;
		element_clear(c.t);
	}
	g_array_free(msk->comps, 1);

	free(msk);
}

/*!
 * Free a private key date structure
 *
 * @param					Private key data structure
 * @return					None
 */

void
kpabe_prv_free( kpabe_prv_t* prv )
{
	kpabe_policy_free(prv->p);
}

/*!
 * Free a ciphrtext date structure
 *
 * @param					Ciphertext data structure
 * @return					None
 */

void
kpabe_cph_free( kpabe_cph_t* cph )
{
	int i;

	element_clear(cph->Ep);


	for( i = 0; i < cph->comps->len; i++ )
	{
		kpabe_cph_comp_t c;

		c = g_array_index(cph->comps, kpabe_cph_comp_t, i);
		c.attr = NULL;
		element_clear(c.E);
	}
	g_array_free(cph->comps, 1);

	free(cph);
}
