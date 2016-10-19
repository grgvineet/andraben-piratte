/*!	\file celia.h
 *
 *	\brief Routines for the Goyal-Pandey-Sahai-Waters ABE scheme.
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

#if defined (__cplusplus)
extern "C" {
#endif

/*
  one attribute structure in public key
*/
typedef struct
{
	char* attr;
	element_t T;		/* G_1 */
}
kpabe_pub_comp_t;

/*
  A public key.
*/
typedef struct
{
	char* pairing_desc;
	pairing_t p;
	element_t g;           /* G_1 */
	element_t Y; 		   /* G_T */
	GArray* comps;		   /* kpabe_pub_comp_t */
}
kpabe_pub_t;

/*
  one attribute structure in master key
*/
typedef struct
{
	char* attr;
	element_t t;		/* Z_p */
}
kpabe_msk_comp_t;

/*
  A master secret key.
*/
typedef struct
{
	element_t y;    	/* Z_p */
	GArray* comps;		/* kpabe_msk_comp_t */
}
kpabe_msk_t;

typedef struct
{
	int deg;
	/* coefficients from [0] x^0 to [deg] x^deg */
	element_t* coef; /* Z_p (of length deg + 1) */
}
kpabe_polynomial_t;

typedef struct
{
	/* serialized */
	int k;            /* one if leaf, otherwise threshold */
	char* attr;       /* attribute string if leaf, otherwise null */
	element_t D;      /* G_1, only for leaves */
	GPtrArray* children; /* pointers to kpabe_policy_t's, len == 0 for leaves */

	/* only used during encryption */
	kpabe_polynomial_t* q;

	/* only used during decryption */
	int satisfiable;
	int min_leaves;
	int attri;
	GArray* satl;
}
kpabe_policy_t;

/*
  A private key.
*/
typedef struct
{
	kpabe_policy_t* p;	/* kpabe_policy_t */
}
kpabe_prv_t;

/*
  one attribute structure in ciphertext key
*/
typedef struct
{
	char* attr;
	element_t E;  		/* G_1 */
}
kpabe_cph_comp_t;

/*
  A ciphertext.
*/
typedef struct
{
	element_t Ep; 		/* G_T */
	GArray* comps;		/* kpabe_cph_comp_t */
}
kpabe_cph_t;

/*
  core function
*/
void kpabe_setup( kpabe_pub_t** pub, kpabe_msk_t** msk, char** attributes, size_t num_attr, int parameters_type);
kpabe_prv_t* kpabe_keygen( kpabe_pub_t* pub,kpabe_msk_t* msk,char* policy );
kpabe_cph_t* kpabe_enc( kpabe_pub_t* pub, element_t m, char** attributes, size_t num_attr);
int kpabe_dec( kpabe_pub_t* pub, kpabe_prv_t* prv, kpabe_cph_t* cph, element_t m );

/*
  Exactly what it seems.
*/
GByteArray* kpabe_pub_serialize( kpabe_pub_t* pub );
GByteArray* kpabe_msk_serialize( kpabe_msk_t* msk );
GByteArray* kpabe_prv_serialize( kpabe_prv_t* prv );
GByteArray* kpabe_cph_serialize( kpabe_cph_t* cph );

/*
  Also exactly what it seems. If free is true, the GByteArray passed
  in will be free'd after it is read.
*/
kpabe_pub_t* kpabe_pub_unserialize( GByteArray* b, int free );
kpabe_msk_t* kpabe_msk_unserialize( kpabe_pub_t* pub, GByteArray* b, int free );
kpabe_prv_t* kpabe_prv_unserialize( kpabe_pub_t* pub, GByteArray* b, int free );
kpabe_cph_t* kpabe_cph_unserialize( kpabe_pub_t* pub, GByteArray* b, int free );

/*
  Again, exactly what it seems.
*/
void kpabe_pub_free( kpabe_pub_t* pub );
void kpabe_msk_free( kpabe_msk_t* msk );
void kpabe_prv_free( kpabe_prv_t* prv );
void kpabe_cph_free( kpabe_cph_t* cph );

/*
  Return a description of the last error that occured. Call this after
  kpabe_enc or kpabe_dec returns 0. The returned string does not
  need to be free'd.
*/
char* kpabe_error();

#if defined (__cplusplus)
} // extern "C"
#endif
