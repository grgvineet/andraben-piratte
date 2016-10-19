#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <glib/glib.h>
#include <include/pbc.h>

#include "bswabe.h"
#include "private.h"

int leaf_count = 0;
int msk_size = 0;
void
serialize_uint32( GByteArray* b, uint32_t k )
{
	int i;
	guint8 byte;

	//printf("4\n");
	for( i = 3; i >= 0; i-- )
	{
		byte = (k & 0xff<<(i*8))>>(i*8);
		g_byte_array_append(b, &byte, 1);
	}
}

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

void
serialize_element( GByteArray* b, element_t e )
{
	uint32_t len;
	unsigned char* buf;

	len = element_length_in_bytes(e);
	//printf("*%u ", sizeof(len));

	serialize_uint32(b, len);

	buf = (unsigned char*) malloc(len);
	element_to_bytes(buf, e);
	g_byte_array_append(b, buf, len);
	free(buf);
}

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

void
serialize_string( GByteArray* b, char* s )
{
	//printf("pub_str: %u\n", (strlen(s) + 1));
	g_byte_array_append(b, (unsigned char*) s, strlen(s) + 1);
}

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

void
serialize_polynomial(GByteArray* b, bswabe_polynomial_t* p)
{
	int i;

	serialize_uint32(b, (uint32_t) p->deg);
	msk_size += sizeof(p->deg);
	//printf("*p->deg = %u\n", sizeof(p->deg));

	for(i=0; i<=p->deg; i++)
	{
		serialize_element(b, p->coef[i]);
		msk_size += element_length_in_bytes(p->coef[i]);
		//printf("*p->coef = %u\n", element_length_in_bytes(p->coef[i]));
	}
}
bswabe_polynomial_t*
unserialize_polynomial(GByteArray* b, int* offset, bswabe_pub_t* pub)
{
	int i;
	bswabe_polynomial_t* p;

	p = (bswabe_polynomial_t*) malloc(sizeof(bswabe_polynomial_t));

	p->deg = (int) unserialize_uint32(b, offset);

	p->coef = (element_t*) malloc(sizeof(element_t) * ((p->deg) + 1));
	for(i=0; i <= p->deg; i++)
	{
		element_init_Zr(p->coef[i], pub->p);
		unserialize_element(b, offset, p->coef[i]);
	}
	return p;
}

GByteArray*
bswabe_point_serialize( bswabe_point* point)
{
	GByteArray* b;
	int i;

	b = g_byte_array_new();

	serialize_uint32(b, point->count);

	for(i=0; i< (point->count); i++)
	{
		serialize_element(b, point->x[i]);
		serialize_element(b, point->p_x[i]);
	}

	return b;
}

bswabe_point*
bswabe_point_unserialize( bswabe_pub_t* pub, GByteArray* b, int free )
{
	bswabe_point* point;
	int offset;
	int i;

	point = (bswabe_point*) malloc(sizeof(bswabe_point));
	offset = 0;

	point->count = (int) unserialize_uint32(b, &offset);
	point->x = (element_t*) malloc(sizeof(element_t) * (point->count));
	point->p_x = (element_t*) malloc(sizeof(element_t) * (point->count));

	for(i=0; i<point->count; i++)
	{
		element_init_Zr(point->x[i], pub->p);
		element_init_Zr(point->p_x[i], pub->p);

		unserialize_element(b, &offset, point->x[i]);
		unserialize_element(b, &offset, point->p_x[i]);
	}

	if( free )
		g_byte_array_free(b, 1);

	return point;
}

GByteArray*
bswabe_pub_serialize( bswabe_pub_t* pub )
{
	GByteArray* b;

	b = g_byte_array_new();
	serialize_string(b,  pub->pairing_desc);

	serialize_element(b, pub->g);
	//printf("pub-g= %u\n", element_length_in_bytes(pub->g));

	serialize_element(b, pub->h);
	//printf("pub-h= %u\n", element_length_in_bytes(pub->h));

	serialize_element(b, pub->gp);
	//printf("pub-gp= %u\n", element_length_in_bytes(pub->gp));

	serialize_element(b, pub->g_hat_alpha);
	//printf("pub-egg= %u\n", element_length_in_bytes(pub->g_hat_alpha));

	return b;
}

bswabe_pub_t*
bswabe_pub_unserialize( GByteArray* b, int free )
{
	bswabe_pub_t* pub;
	int offset;

	pub = (bswabe_pub_t*) malloc(sizeof(bswabe_pub_t));
	offset = 0;

	pub->pairing_desc = unserialize_string(b, &offset);
	pairing_init_set_buf(pub->p, pub->pairing_desc, strlen(pub->pairing_desc));

	element_init_G1(pub->g,           pub->p);
	element_init_G1(pub->h,           pub->p);
	element_init_G2(pub->gp,          pub->p);
	element_init_GT(pub->g_hat_alpha, pub->p);

	unserialize_element(b, &offset, pub->g);
	unserialize_element(b, &offset, pub->h);
	unserialize_element(b, &offset, pub->gp);
	unserialize_element(b, &offset, pub->g_hat_alpha);

	if( free )
		g_byte_array_free(b, 1);

	return pub;
}

GByteArray*
bswabe_msk_serialize( bswabe_msk_t* msk )
{
	GByteArray* b;

	b = g_byte_array_new();

	serialize_element(b, msk->beta);
	//printf("*msk->beta = %u\n", element_length_in_bytes(msk->beta));

	serialize_element(b, msk->g_alpha);
	//printf("*msk->g_alpha = %u\n", element_length_in_bytes(msk->g_alpha));

	msk_size += element_length_in_bytes(msk->beta);
	msk_size += element_length_in_bytes(msk->g_alpha);

	serialize_polynomial(b, msk->rpt);

	//printf("msk-size = %u\n", msk_size);

	return b;
}

bswabe_msk_t*
bswabe_msk_unserialize( bswabe_pub_t* pub, GByteArray* b, int free )
{
	bswabe_msk_t* msk;
	int offset;

	msk = (bswabe_msk_t*) malloc(sizeof(bswabe_msk_t));
	offset = 0;

	element_init_Zr(msk->beta, pub->p);
	element_init_G2(msk->g_alpha, pub->p);

	unserialize_element(b, &offset, msk->beta);
	unserialize_element(b, &offset, msk->g_alpha);

	msk->rpt = unserialize_polynomial(b, &offset, pub);

	if( free )
		g_byte_array_free(b, 1);

	return msk;
}

GByteArray*
bswabe_prv_serialize( bswabe_prv_t* prv )
{
	GByteArray* b;
	int i;

	b = g_byte_array_new();

	serialize_element(b, prv->d);
	//printf("prv-d = %u\n", element_length_in_bytes(prv->d));
	serialize_uint32( b, prv->comps->len);
	//printf("prv-comp-len = %u\n", sizeof(prv->comps->len));

	for( i = 0; i < prv->comps->len; i++ )
	{
		serialize_string( b, g_array_index(prv->comps, bswabe_prv_comp_t, i).attr);
		serialize_element(b, g_array_index(prv->comps, bswabe_prv_comp_t, i).d);
		//printf("prv-di = %u\n", element_length_in_bytes(g_array_index(prv->comps, bswabe_prv_comp_t, i).d));

		serialize_element(b, g_array_index(prv->comps, bswabe_prv_comp_t, i).dp);
		//printf("prv-di' = %u\n", element_length_in_bytes(g_array_index(prv->comps, bswabe_prv_comp_t, i).dp));

		serialize_element(b, g_array_index(prv->comps, bswabe_prv_comp_t, i).dpp);
		//printf("prv-di'' = %u\n", element_length_in_bytes(g_array_index(prv->comps, bswabe_prv_comp_t, i).dpp));
	}

	return b;
}

bswabe_prv_t*
bswabe_prv_unserialize( bswabe_pub_t* pub, GByteArray* b, int free )
{
	bswabe_prv_t* prv;
	int i;
	int len;
	int offset;

	prv = (bswabe_prv_t*) malloc(sizeof(bswabe_prv_t));
	offset = 0;

	element_init_G2(prv->d, pub->p);
	unserialize_element(b, &offset, prv->d);

	prv->comps = g_array_new(0, 1, sizeof(bswabe_prv_comp_t));
	len = unserialize_uint32(b, &offset);

	for( i = 0; i < len; i++ )
	{
		bswabe_prv_comp_t c;

		c.attr = unserialize_string(b, &offset);

		element_init_G2(c.d,  pub->p);
		element_init_G1(c.dp, pub->p);
		element_init_G1(c.dpp, pub->p);

		unserialize_element(b, &offset, c.d);
		unserialize_element(b, &offset, c.dp);
		unserialize_element(b, &offset, c.dpp);

		g_array_append_val(prv->comps, c);
	}

	if( free )
		g_byte_array_free(b, 1);

	return prv;
}

/*for delegated private key*/
GByteArray*
bswabe_del_prv_serialize( bswabe_del_prv_t* prv )
{
	GByteArray* b;
	int i;

	b = g_byte_array_new();

	serialize_element(b, prv->d);
	//printf("prv-d = %u\n", element_length_in_bytes(prv->d));
	serialize_uint32( b, prv->comps->len);
	//printf("prv-comp-len = %u\n", sizeof(prv->comps->len));

	for( i = 0; i < prv->comps->len; i++ )
	{
		serialize_string( b, g_array_index(prv->comps, bswabe_del_prv_comp_t, i).attr);
		serialize_element(b, g_array_index(prv->comps, bswabe_del_prv_comp_t, i).d);
		//printf("prv-di = %u\n", element_length_in_bytes(g_array_index(prv->comps, bswabe_del_prv_comp_t, i).d));

		serialize_element(b, g_array_index(prv->comps, bswabe_del_prv_comp_t, i).dp);
		//printf("prv-di' = %u\n", element_length_in_bytes(g_array_index(prv->comps, bswabe_del_prv_comp_t, i).dp));

		serialize_element(b, g_array_index(prv->comps, bswabe_del_prv_comp_t, i).dpp);
		//printf("prv-di'' = %u\n", element_length_in_bytes(g_array_index(prv->comps, bswabe_del_prv_comp_t, i).dpp));

		serialize_element(b, g_array_index(prv->comps, bswabe_del_prv_comp_t, i).d3p);
		//printf("prv-di''' = %u\n", element_length_in_bytes(g_array_index(prv->comps, bswabe_del_prv_comp_t, i).d3p));
	}

	return b;
}

bswabe_del_prv_t*
bswabe_del_prv_unserialize( bswabe_pub_t* pub, GByteArray* b, int free )
{
	bswabe_del_prv_t* prv;
	int i;
	int len;
	int offset;

	prv = (bswabe_del_prv_t*) malloc(sizeof(bswabe_del_prv_t));
	offset = 0;

	element_init_G2(prv->d, pub->p);
	unserialize_element(b, &offset, prv->d);

	prv->comps = g_array_new(0, 1, sizeof(bswabe_del_prv_comp_t));
	len = unserialize_uint32(b, &offset);

	for( i = 0; i < len; i++ )
	{
		bswabe_del_prv_comp_t c;

		c.attr = unserialize_string(b, &offset);

		element_init_G2(c.d,  pub->p);
		element_init_G1(c.dp, pub->p);
		element_init_G1(c.dpp, pub->p);
		element_init_G1(c.d3p, pub->p);

		unserialize_element(b, &offset, c.d);
		unserialize_element(b, &offset, c.dp);
		unserialize_element(b, &offset, c.dpp);
		unserialize_element(b, &offset, c.d3p);

		g_array_append_val(prv->comps, c);
	}

	if( free )
		g_byte_array_free(b, 1);

	return prv;
}

/*if proxy ==1, srlz/unsrlz cpp*/
void
serialize_policy( GByteArray* b, bswabe_policy_t* p, int proxy )
{
	int i;

	//printf("k len = ");
	serialize_uint32(b, (uint32_t) p->k);

	//printf("children len = ");
	serialize_uint32(b, (uint32_t) p->children->len);
	if( p->children->len == 0 )
	{
		//printf("attr len = ");
		serialize_string( b, p->attr);
		//printf("c len = ");
		serialize_element(b, p->c);
		//printf("cp len = ");
		serialize_element(b, p->cp);
		if(proxy == 1)
		{
			//printf("cpp len = ");
			serialize_element(b, p->cpp);
		}
	}
	else
		for( i = 0; i < p->children->len; i++ )
			serialize_policy(b, g_ptr_array_index(p->children, i), proxy);
}

bswabe_policy_t*
unserialize_policy( bswabe_pub_t* pub, GByteArray* b, int* offset, int proxy)
{
	int i;
	int n;
	bswabe_policy_t* p;

	p = (bswabe_policy_t*) malloc(sizeof(bswabe_policy_t));

	p->k = (int) unserialize_uint32(b, offset);
	p->attr = 0;
	p->children = g_ptr_array_new();

	n = unserialize_uint32(b, offset);
	if( n == 0 )
	{
		p->attr = unserialize_string(b, offset);
		element_init_G1(p->c,  pub->p);
		element_init_G2(p->cp, pub->p);
		unserialize_element(b, offset, p->c);
		unserialize_element(b, offset, p->cp);

		if(proxy == 1)
		{
			element_init_G2(p->cpp, pub->p);
			unserialize_element(b, offset, p->cpp);
		}
		leaf_count++;
	}
	else
		for( i = 0; i < n; i++ )
			g_ptr_array_add(p->children, unserialize_policy(pub, b, offset, proxy));

	return p;
}

GByteArray*
bswabe_cph_serialize( bswabe_cph_t* cph, int proxy )
{
	GByteArray* b;

	b = g_byte_array_new();
	//printf("cs len = ");
	serialize_element(b, cph->cs);
	//printf("c len = ");
	serialize_element(b, cph->c);
	serialize_policy( b, cph->p, proxy);

	return b;
}

bswabe_cph_t*
bswabe_cph_unserialize( bswabe_pub_t* pub, GByteArray* b, int free, int proxy)
{
	bswabe_cph_t* cph;
	int offset;

	cph = (bswabe_cph_t*) malloc(sizeof(bswabe_cph_t));
	offset = 0;

	element_init_GT(cph->cs, pub->p);
	element_init_G1(cph->c,  pub->p);
	unserialize_element(b, &offset, cph->cs);
	unserialize_element(b, &offset, cph->c);

	cph->p = unserialize_policy(pub, b, &offset, proxy);

	if( free )
		g_byte_array_free(b, 1);

	//printf("%d ", leaf_count);

	return cph;
}

void
bswabe_pub_free( bswabe_pub_t* pub )
{
	element_clear(pub->g);
	element_clear(pub->h);
	element_clear(pub->gp);
	element_clear(pub->g_hat_alpha);
	pairing_clear(pub->p);
	free(pub->pairing_desc);
	free(pub);
}

void
bswabe_polynomial_free(bswabe_polynomial_t* pol)
{
	int i;
	for(i=0; i< pol->deg; i++)
	{
		element_clear(pol->coef[i]);
	}
}

void
bswabe_msk_free( bswabe_msk_t* msk )
{
	element_clear(msk->beta);
	element_clear(msk->g_alpha);
	bswabe_polynomial_free(msk->rpt);

	free(msk);
}

void
bswabe_prv_free( bswabe_prv_t* prv )
{
	int i;

	element_clear(prv->d);

	for( i = 0; i < prv->comps->len; i++ )
	{
		bswabe_prv_comp_t c;

		c = g_array_index(prv->comps, bswabe_prv_comp_t, i);
		free(c.attr);
		element_clear(c.d);
		element_clear(c.dp);
		element_clear(c.dpp);
	}

	g_array_free(prv->comps, 1);

	free(prv);
}

void
bswabe_del_prv_free( bswabe_del_prv_t* prv )
{
	int i;

	element_clear(prv->d);

	for( i = 0; i < prv->comps->len; i++ )
	{
		bswabe_del_prv_comp_t c;

		c = g_array_index(prv->comps, bswabe_del_prv_comp_t, i);
		free(c.attr);
		element_clear(c.d);
		element_clear(c.dp);
		element_clear(c.dpp);
		element_clear(c.d3p);
	}

	g_array_free(prv->comps, 1);

	free(prv);
}

void
bswabe_policy_free( bswabe_policy_t* p, int proxy )
{
	int i;

	if( p->attr )
	{
		free(p->attr);
		element_clear(p->c);
		element_clear(p->cp);
		if(proxy == 1)
		{
			element_clear(p->cpp);
		}
	}

	for( i = 0; i < p->children->len; i++ )
		bswabe_policy_free(g_ptr_array_index(p->children, i), proxy);

	g_ptr_array_free(p->children, 1);

	free(p);
}

void
bswabe_cph_free( bswabe_cph_t* cph, int proxy )
{
	element_clear(cph->cs);
	element_clear(cph->c);
	bswabe_policy_free(cph->p, proxy);
}

void
bswabe_element_init_Zr(element_t e, bswabe_pub_t* pub)
{
	element_init_Zr(e, pub->p);
}
void
bswabe_element_init_GT(element_t e, bswabe_pub_t* pub)
{
	element_init_GT(e, pub->p);
}

int
integer_from_element(element_t e)
{
	int integer;

	char s[100];

	element_snprint(s, sizeof(element_t), e);
	sscanf(s, "%d", &integer);

	return integer;
}
