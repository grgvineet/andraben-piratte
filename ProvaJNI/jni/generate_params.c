#include <pbc.h>


int main() {
	pbc_param_t par;
	pbc_param_init_a_gen(par,224,1024);
	
	FILE* pf;
	pf = fopen ( "parameters_224_1024.txt" , "w" ) ;

	pbc_param_t par1;
	pbc_param_init_a_gen(par1,256,1536);
	
	FILE* pf1;
	pf1 = fopen ( "parameters_256_1536.txt" , "w" ) ;

	pbc_param_out_str(pf,par);
	pbc_param_out_str(pf1,par1);
	
}
