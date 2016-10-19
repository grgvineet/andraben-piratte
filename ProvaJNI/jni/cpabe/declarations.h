struct bswabe_pub_s
{
	char* pairing_desc;
	pairing_t p;
	element_t g;           /* G_1 */
	element_t h;           /* G_1 */
	element_t gp;          /* G_2 */
	element_t g_hat_alpha; /* G_T */
};

struct bswabe_msk_s
{
	element_t beta;    /* Z_r */
	element_t g_alpha; /* G_2 */
};


typedef struct
{
	/* these actually get serialized */
	char* attr;
	element_t d;  /* G_2 */
	element_t dp; /* G_2 */

	/* only used during dec (only by dec_merge) */
	int used;
	element_t z;  /* G_1 */
	element_t zp; /* G_1 */
}
bswabe_prv_comp_t;


typedef struct
{
	bswabe_prv_comp_t* data;
	int len;
	int size;
	int elem_size;
} GArray;



struct bswabe_prv_s
{
	element_t d;   /* G_2 */
	GArray* comps; /* bswabe_prv_comp_t's */
};

/*
  A public key.
*/
typedef struct bswabe_pub_s bswabe_pub_t;

/*
  A master secret key.
*/
typedef struct bswabe_msk_s bswabe_msk_t;

/*
  A private key.
*/
typedef struct bswabe_prv_s bswabe_prv_t;
