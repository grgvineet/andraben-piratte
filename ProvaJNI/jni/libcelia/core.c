/*!	\file core.c
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <openssl/sha.h>
#include <glib/glib.h>
#include <include/pbc.h>
#include "celia.h"
#include <android/log.h>

/********************************************************************************
 * Goyal-Pandey-Sahai-Waters Implementation
 ********************************************************************************/

#ifndef KPABE_DEBUG
#define NDEBUG
#endif

# define TYPE_A_PARAMS_r256_q1536 \
"type a\n" \
"q 791496878104402409321503747192521536280279872109362818307586" \
"928851256178053778187961366862691930632668217434105031571387723" \
"995541238510051637259158749629280453776820907283495151868382908" \
"698788067915606084165254958663854687180679173826988783265791944" \
"637961523327778129765002965789747308397608394690344563277112043" \
"661514947690801767064885615109833829241449365493344368109115561" \
"122491514652352115826236756749871432173210772557773824616972548" \
"293139613239847592408499\n" \
"h 1367100090028132144119129367294505322327360646941951434723684" \
"612915246624587693969940840922653303883530084192058506506630821" \
"717027130067649528239780058288678997990922625491747411116663455" \
"674673023452423234156282032815873594467986296755544443870280422" \
"883301179322501377283602098176595515702094822349105144101127606" \
"025156062349890632842356530486115981916154020706374944205075353" \
"3140519500\n" \
"r 5789604461865809771178549250434395392663499233289951018224305" \
"6341550108770303\n" \
"exp2 255\n" \
"exp1 96\n" \
"sign1 1\n" \
"sign0 -1\n"



#define TYPE_A_PARAMS_r224_q1024 \
"type a\n" \
"q 84384357784150562441149277795697790496559762625762977242" \
"51169654675114711095984428010207258080171534944611148008381" \
"93458738887318693248877742893529512937344701721761106262329" \
"65036531226995197992695517654000912574839411767930544235152" \
"47308744293799269234258528295024166195879716527777212345709" \
"6864630705177683\n" \
"h 625997957829558501789466470126799015378474404839796445854" \
"06462880464605171947553289071000663878346455291962503714527" \
"76259372454760186242653595095947734193487259243778023210836" \
"14399645653699707813360785314557908768067845855535633269491" \
"5302484\n" \
"r 134799733335753198973335075435098153368185722112702862405" \
"51805132801\n" \
"exp2 223\n" \
"exp1 13\n" \
"sign1 1\n" \
"sign0 1\n"


#define TYPE_A_PARAMS_r160_q512 \
"type a\n" \
"q 87807107996633125224377819847540498158068831994142082" \
"1102865339926647563088022295707862517942266222142315585" \
"8769582317459277713367317481324925129998224791\n" \
"h 12016012264891146079388821366740534204802954401251311" \
"822919615131047207289359704531102844802183906537786776\n" \
"r 730750818665451621361119245571504901405976559617\n" \
"exp2 159\n" \
"exp1 107\n" \
"sign1 1\n" \
"sign0 1\n"


/*!
 * Last error call back for display
 *
 * @return				last_error.
 */

char last_error[256];
char*
kpabe_error()
{
	return last_error;
}

/*!
 * Handle error while using library routine
 *
 * @param fmt			Error string
 * @return				none.
 */

void
raise_error(char* fmt, ...)
{
	va_list args;

#ifdef KPABE_DEBUG
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(1);
#else
	va_start(args, fmt);
	vsnprintf(last_error, 256, fmt, args);
	va_end(args);
#endif
}

/*!
 * Generate public and master key with the provided attributes list.
 *
 * @param pub			Pointer to the public key data structure
 * @param msk			Pointer to the master key data structure
 * @param attributes	Attributes list
 * @return				none.
 */

void
kpabe_setup( kpabe_pub_t** pub, kpabe_msk_t** msk, char** attributes, size_t num_attr, int parameters_type)
{
/*	__android_log_print(ANDROID_LOG_INFO, "KPABE", "Setup ");*/

	element_t tmp;	/* G_1 */

	/* initialize */

	*pub = malloc(sizeof(kpabe_pub_t));
	*msk = malloc(sizeof(kpabe_msk_t));

	
	switch(parameters_type)
	{
		case 1:
			(*pub)->pairing_desc = strdup(TYPE_A_PARAMS_r160_q512);
			break;
		case 2:
			(*pub)->pairing_desc = strdup(TYPE_A_PARAMS_r224_q1024);
			break;
		case 3:
			(*pub)->pairing_desc = strdup(TYPE_A_PARAMS_r256_q1536);
			break;
		default:
			(*pub)->pairing_desc = strdup(TYPE_A_PARAMS_r160_q512);
	}
	
	
	pairing_init_set_buf((*pub)->p, (*pub)->pairing_desc, strlen((*pub)->pairing_desc));

	element_init_G1((*pub)->g,           (*pub)->p);
	element_init_G1(tmp,         		 (*pub)->p);
	element_init_GT((*pub)->Y,			 (*pub)->p);
	element_init_Zr((*msk)->y,			 (*pub)->p);

	(*pub)->comps = g_array_new(0, 1, sizeof(kpabe_pub_comp_t));
	(*msk)->comps = g_array_new(0, 1, sizeof(kpabe_msk_comp_t));

	/* compute */

 	element_random((*msk)->y);
	element_random((*pub)->g);

	element_pow_zn(tmp, (*pub)->g, (*msk)->y);
	pairing_apply((*pub)->Y, (*pub)->g, tmp, (*pub)->p);

	int i = 0;
/*	while( *attributes )*/
	for (;i < num_attr; i++)
	{
		kpabe_pub_comp_t TA;
		kpabe_msk_comp_t ta;
	
	
		TA.attr = *(attributes++);
		ta.attr = TA.attr;

		element_init_Zr(ta.t,(*pub)->p);
		element_init_G1(TA.T,(*pub)->p);

 		element_random(ta.t);
		element_pow_zn(TA.T, (*pub)->g, ta.t);

		g_array_append_val((*pub)->comps, TA);
		g_array_append_val((*msk)->comps, ta);
	}

}

/*!
 * Encrypt a secret message with the provided attributes list, return a ciphertext.
 *
 * @param pub			Public key structure
 * @param m				Secret Message
 * @param attributes	Attributes list
 * @return				Ciphertext structure
 */

kpabe_cph_t*
kpabe_enc( kpabe_pub_t* pub, element_t m, char** attributes, size_t num_attr)
{
	kpabe_cph_t* cph;
 	element_t s;
 	int i;

	/* initialize */

	cph = malloc(sizeof(kpabe_cph_t));

	element_init_Zr(s, pub->p);
	element_init_GT(m, pub->p);
	element_init_GT(cph->Ep, pub->p);

	/* compute */

 	element_random(m);
 	element_random(s);
	element_pow_zn(cph->Ep, pub->Y, s);
	element_mul(cph->Ep, cph->Ep, m);

	cph->comps = g_array_new(0, 1, sizeof(kpabe_cph_comp_t));

	int j;
	for (j = 0; j < num_attr; j++)
/*	while( *attributes )*/
	{
		kpabe_cph_comp_t c;

		c.attr = *(attributes++);

		element_init_G1(c.E, pub->p);

		for( i = 0; i < pub->comps->len; i++ )
		{
/*			__android_log_print(ANDROID_LOG_INFO, "KPABE", "Att: %s | %s",g_array_index(pub->comps, kpabe_pub_comp_t, i).attr,c.attr);*/
			if( !strcmp(g_array_index(pub->comps, kpabe_pub_comp_t, i).attr, c.attr) )
			{
				element_pow_zn(c.E, g_array_index(pub->comps, kpabe_pub_comp_t, i).T, s);
				break;
			}
			else
			{
				if(i == (pub->comps->len - 1))
				{
					raise_error("ENC - Check your attribute universe,\nCertain attribute not include!\n");
					return 0;
				}
			}
		}

		g_array_append_val(cph->comps, c);
	}

	return cph;
}

/*!
 * Subroutine to fill out a single KP-ABE Policy node structure
 *
 * @param k				Threshold of this node
 * @param s				Attribute of this node (if it is the leaf node)
 * @return				Policy node data structure
 */

kpabe_policy_t*
base_node( int k, char* s )
{
	kpabe_policy_t* p;

	p = (kpabe_policy_t*) malloc(sizeof(kpabe_policy_t));
	p->k = k;
	p->attr = s ? strdup(s) : 0;
	p->children = g_ptr_array_new();
	p->q = 0;

	return p;
}

/*!
 * Generate a Policy tree from the input policy string.
 *
 * @param s				Policy string
 * @return				Policy root node data structure
 */

/*
	TODO convert this to use a GScanner and handle quotes and / or
	escapes to allow attributes with whitespace or = signs in them
*/

kpabe_policy_t*
parse_policy_postfix( char* s )
{
	char** toks;
	char** cur_toks;
	char*  tok;
	GPtrArray* stack; /* pointers to kpabe_policy_t's */
	kpabe_policy_t* root;

	toks     = g_strsplit(s, " ", 0);
	cur_toks = toks;
	stack    = g_ptr_array_new();
	
	while( *cur_toks )
	{
	
		int i, k, n;

		tok = *(cur_toks++);

		if( !*tok )
			continue;

		if( sscanf(tok, "%dof%d", &k, &n) != 2 )
			/* push leaf token */
			g_ptr_array_add(stack, base_node(1, tok));
		else
		{
			kpabe_policy_t* node;

			/* parse "kofn" operator */

			if( k < 1 )
			{
				raise_error("error parsing \"%s\": trivially satisfied operator \"%s\"\n", s, tok);
				return 0;
			}
			else if( k > n )
			{
				raise_error("error parsing \"%s\": unsatisfiable operator \"%s\"\n", s, tok);
				return 0;
			}
			else if( n == 1 )
			{
				raise_error("error parsing \"%s\": identity operator \"%s\"\n", s, tok);
				return 0;
			}
			else if( n > stack->len )
			{
				raise_error("error parsing \"%s\": stack underflow at \"%s\"\n", s, tok);
				return 0;
			}

			/* pop n things and fill in children */
			node = base_node(k, 0);
			g_ptr_array_set_size(node->children, n);
			for( i = n - 1; i >= 0; i-- )
				node->children->pdata[i] = g_ptr_array_remove_index(stack, stack->len - 1);

			/* push result */
			g_ptr_array_add(stack, node);
		}
	}

	if( stack->len > 1 )
	{
		raise_error("error parsing \"%s\": extra tokens left on stack\n", s);
		return 0;
	}
	else if( stack->len < 1 )
	{
		raise_error("error parsing \"%s\": empty policy\n", s);
		return 0;
	}

	root = g_ptr_array_index(stack, 0);

 	g_strfreev(toks);
 	g_ptr_array_free(stack, 0);

	return root;
}

/*!
 * Randomly generate the Lagrange basis polynomial base on provided constant value
 *
 * @param deg			Degree of the lagrange basis polynomial
 * @param zero_val		Constant value of the lagrange basis polynomial
 * @return				Lagrange basis polynomial data structure
 */

kpabe_polynomial_t*
rand_poly( int deg, element_t zero_val )
{
	int i;
	kpabe_polynomial_t* q;

	q = (kpabe_polynomial_t*) malloc(sizeof(kpabe_polynomial_t));
	q->deg = deg;
	q->coef = (element_t*) malloc(sizeof(element_t) * (deg + 1));

	for( i = 0; i < q->deg + 1; i++ )
		element_init_same_as(q->coef[i], zero_val);

	element_set(q->coef[0], zero_val);

	for( i = 1; i < q->deg + 1; i++ )
 		element_random(q->coef[i]);


	return q;
}

/*!
 * Compute the constant value of the child node's Lagrange basis polynomial,
 *
 * @param r				Constant value of this child node's Lagrange basis polynomial
 * @param q				Pointer to the lagrange basis polynomial of parent node
 * @param x				index of this child node in its parent node
 * @return				None
 */

void
eval_poly( element_t r, kpabe_polynomial_t* q, element_t x )
{
	int i;
	element_t s, t;

	element_init_same_as(s, r);
	element_init_same_as(t, r);

	element_set0(r);
	element_set1(t);

	for( i = 0; i < q->deg + 1; i++ )
	{
		/* r += q->coef[i] * t */
		element_mul(s, q->coef[i], t);
		element_add(r, r, s);

		/* t *= x */
		element_mul(t, t, x);
	}

	element_clear(s);
	element_clear(t);
}

/*!
 * Routine to fill out the Policy tree
 *
 * @param P				Pointer to Root node policy data structure
 * @param pub			Public key
 * @param msk			Master key
 * @param e				Root secret
 * @return				None
 */

int
fill_policy( kpabe_policy_t* p, kpabe_pub_t* pub, kpabe_msk_t* msk, element_t e )
{
	int i;
	element_t r;
	element_t t;
	element_t a;
	
	element_init_Zr(r, pub->p);
	element_init_Zr(t, pub->p);
	element_init_Zr(a, pub->p);

	p->q = rand_poly(p->k - 1, e);
	
	if( p->children->len == 0 )
	{
		element_init_G1(p->D,  pub->p);
/*		__android_log_print(ANDROID_LOG_INFO, "ABE", "Len = %d",msk->comps->len);*/
		for( i = 0; i < msk->comps->len; i++ )
		{
			if( !strcmp(g_array_index(msk->comps, kpabe_msk_comp_t, i).attr, p->attr) )
			{
				element_div(a, p->q->coef[0], g_array_index(msk->comps, kpabe_msk_comp_t, i).t);
				element_pow_zn(p->D, pub->g, a);
				break;
			}
			else
			{
				if(i == (msk->comps->len - 1))
				{
					__android_log_print(ANDROID_LOG_ERROR, "ABE", "Check your attributes universe, certain attributes are not included!\n");
					return 0;
				}
			}
		}
	}
	else
		for( i = 0; i < p->children->len; i++ )
		{
			element_set_si(r, i + 1);
			eval_poly(t, p->q, r);
			if(!fill_policy(g_ptr_array_index(p->children, i), pub, msk, t))
				return 0;
		}

	element_clear(r);
	element_clear(t);
	element_clear(a);
	return 1;
}

/*!
 * Generate private key with the provided policy.
 *
 * @param pub			Public key data structure
 * @param msk			Master key data structure
 * @param policy		Policy tree string
 * @return				Private key data structure.
 */

kpabe_prv_t*
kpabe_keygen( kpabe_pub_t* pub, kpabe_msk_t* msk, char* policy )
{
	kpabe_prv_t* prv;
	/* initialize */
	prv = malloc(sizeof(kpabe_prv_t));
	prv->p = parse_policy_postfix(policy);

	/* compute */
	int res = fill_policy(prv->p, pub, msk, msk->y); 
	
	if(!res)
		return 0;
	
	return prv;
}

/*!
 * Check whether the attributes in the ciphertext data structure can
 * access the root secret in the policy data structure, and mark all
 * possible path
 *
 * @param p				Policy node data structure (root)
 * @param cph			Ciphertext data structure
 * @param oub			Public key data structure
 * @return				None
 */

int
check_sat( kpabe_policy_t* p, kpabe_cph_t* cph, kpabe_pub_t* pub )
{
	int i, l;

	p->satisfiable = 0;
	if( p->children->len == 0 )
	{
		for( i = 0; i < cph->comps->len; i++ )
			if( !strcmp(g_array_index(cph->comps, kpabe_cph_comp_t, i).attr,
									p->attr) )
			{
				p->satisfiable = 1;
				p->attri = i;
				break;
			}
		for( i = 0; i < pub->comps->len; i++ )
			if( !strcmp(g_array_index(pub->comps, kpabe_pub_comp_t, i).attr,
					                p->attr) )
			{
				break;
			}
			else
			{
				if(i == (pub->comps->len - 1))
				{
					raise_error("Check your attribute universe,\nCertain attribute not included!\n");
					return 0;
				}
			}
	}
	else
	{
		for( i = 0; i < p->children->len; i++ )
			if(!check_sat(g_ptr_array_index(p->children, i), cph, pub))
				return 0;

		l = 0;
		for( i = 0; i < p->children->len; i++ )
			if( ((kpabe_policy_t*) g_ptr_array_index(p->children, i))->satisfiable )
				l++;

		if( l >= p->k )
			p->satisfiable = 1;
	}
	return 1;
}

/*!
 * Function that compare the minimal leaves of two child policy node of the same parent node
 *
 * @param a				index of first child node in its parent node
 * @param b				index of second child node in its parent node
 * @return	k			compare result
 */

kpabe_policy_t* cur_comp_pol;
int
cmp_int( const void* a, const void* b )
{
	int k, l;

	k = ((kpabe_policy_t*) g_ptr_array_index(cur_comp_pol->children, *((int*)a)))->min_leaves;
	l = ((kpabe_policy_t*) g_ptr_array_index(cur_comp_pol->children, *((int*)b)))->min_leaves;

	return
		k <  l ? -1 :
		k == l ?  0 : 1;
}

/*!
 * Choose the path with minimal leaves node from all possible path which are marked as satisfiable
 * Mark the respective "min_leaves" element in the policy node data structure
 *
 * @param p				Policy node data structure (root)
 * @return				None
 */

void
pick_sat_min_leaves( kpabe_policy_t* p )
{
	int i, k, l;
	int* c;

	assert(p->satisfiable == 1);

	if( p->children->len == 0 )
		p->min_leaves = 1;
	else
	{
		for( i = 0; i < p->children->len; i++ )
			if( ((kpabe_policy_t*) g_ptr_array_index(p->children, i))->satisfiable )
				pick_sat_min_leaves(g_ptr_array_index(p->children, i));

		c = alloca(sizeof(int) * p->children->len);
		for( i = 0; i < p->children->len; i++ )
			c[i] = i;

		cur_comp_pol = p;
		qsort(c, p->children->len, sizeof(int), cmp_int);

		p->satl = g_array_new(0, 0, sizeof(int));
		p->min_leaves = 0;
		l = 0;

		for( i = 0; i < p->children->len && l < p->k; i++ )
			if( ((kpabe_policy_t*) g_ptr_array_index(p->children, c[i]))->satisfiable )
			{
				l++;
				p->min_leaves += ((kpabe_policy_t*) g_ptr_array_index(p->children, c[i]))->min_leaves;
				k = c[i] + 1;
				g_array_append_val(p->satl, k);
			}
		assert(l == p->k);
	}
}

/*!
 * Compute Lagrange coefficient
 *
 * @param r				Lagrange coefficient
 * @param s				satisfiable node set
 * @param i				index of this node in the satisfiable node set
 * @return				None
 */

void
lagrange_coef( element_t r, GArray* s, int i )
{
	int j, k;
	element_t t;

	element_init_same_as(t, r);

	element_set1(r);
	for( k = 0; k < s->len; k++ )
	{
		j = g_array_index(s, int, k);
		if( j == i )
			continue;
		element_set_si(t, - j);
		element_mul(r, r, t); /* num_muls++; */
		element_set_si(t, i - j);
		element_invert(t, t);
		element_mul(r, r, t); /* num_muls++; */
	}

	element_clear(t);
}

/*!
 * DecryptNode(E;D;x) algorithm for leaf node
 *
 * @param r				Pairing result
 * @param exp			Recursive exponent from DecryptNode(E;D;z) algorithm from non-leaf node above
 * @param p				Policy node dtat structure(leaf node x)
 * @param cph			Ciphertext data structure
 * @param pub			Public key data structure
 * @return				None
 */

void
dec_leaf_flatten( element_t r, element_t exp,
									kpabe_policy_t* p, kpabe_cph_t* cph, kpabe_pub_t* pub )
{
	kpabe_cph_comp_t* c;
	element_t s;

	c = &(g_array_index(cph->comps, kpabe_cph_comp_t, p->attri));

	element_init_GT(s, pub->p);

	pairing_apply(s, p->D,  c->E,  pub->p); /* num_pairings++; */
	element_pow_zn(s, s, exp); /* num_exps++; */
	element_mul(r, r, s); /* num_muls++; */

	element_clear(s);
}

void dec_node_flatten( element_t r, element_t exp,
											 kpabe_policy_t* p, kpabe_cph_t* cph, kpabe_pub_t* pub );

/*!
 * DecryptNode(E;D;z) algorithm for non-leaf node
 *
 * @param r				Pairing result
 * @param exp			Recursive exponent from DecryptNode(E;D;z) algorithm from non-leaf node above
 * @param p				Policy node dtat structure(non-leaf node z)
 * @param cph			Ciphertext data structure
 * @param pub			Public key data structure
 * @return				None
 */

void
dec_internal_flatten( element_t r, element_t exp,
											kpabe_policy_t* p, kpabe_cph_t* cph, kpabe_pub_t* pub )
{
	int i;
	element_t t;
	element_t expnew;

	element_init_Zr(t, pub->p);
	element_init_Zr(expnew, pub->p);

	for( i = 0; i < p->satl->len; i++ )
	{
 		lagrange_coef(t, p->satl, g_array_index(p->satl, int, i));
		element_mul(expnew, exp, t); /* num_muls++; */
		dec_node_flatten(r, expnew, g_ptr_array_index
										 (p->children, g_array_index(p->satl, int, i) - 1), cph, pub);
	}

	element_clear(t);
	element_clear(expnew);
}

/*!
 * Choose DecryptNode algorithm for non-leaf node and leaf node
 *
 * @param r				Pairing result
 * @param exp			Recursive exponent from DecryptNode(E;D;z) algorithm from non-leaf node above
 * @param p				Policy node data structure
 * @param cph			Ciphertext data structure
 * @param pub			Public key data structure
 * @return				None
 */

void
dec_node_flatten( element_t r, element_t exp,
									kpabe_policy_t* p, kpabe_cph_t* cph, kpabe_pub_t* pub )
{
	assert(p->satisfiable);
	if( p->children->len == 0 )
		dec_leaf_flatten(r, exp, p, cph, pub);
	else
		dec_internal_flatten(r, exp, p, cph, pub);
}

/*!
 * DecryptNode algorithm for root secret
 *
 * @param r				Root secret
 * @param p				Policy node dtat structure(root)
 * @param cph			Ciphertext data structure
 * @param pub			Public key data structure
 * @return				None
 */

void
dec_flatten( element_t r, kpabe_policy_t* p, kpabe_cph_t* cph, kpabe_pub_t* pub )
{
	element_t one;

	element_init_Zr(one, pub->p);

	element_set1(one);
	element_set1(r);

	dec_node_flatten(r, one, p, cph, pub);

	element_clear(one);
}

/*!
 * Decrypt the secret message m
 *
 * @param pub				Public key data structure
 * @param prv				Private key data structure
 * @param cph				Ciphertext data structure
 * @param m					Secret message
 * @return int				Successfully decrypt or not
 */

int
kpabe_dec( kpabe_pub_t* pub, kpabe_prv_t* prv, kpabe_cph_t* cph, element_t m )
{
	element_t Ys;

	element_init_GT(m, pub->p);
	element_init_GT(Ys, pub->p);

	if(!check_sat(prv->p, cph, pub))
		return 0;
	if( !prv->p->satisfiable )
	{
		raise_error("cannot decrypt, attributes in ciphertext do not satisfy policy\n");
		return 0;
	}

	pick_sat_min_leaves(prv->p);
	dec_flatten(Ys, prv->p, cph, pub);
	element_div(m, cph->Ep, Ys);

	return 1;
}
