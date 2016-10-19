#include <string.h>
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <android/log.h>
#include <unistd.h>
#include "libbswabe/bswabe.h"
#include "policy_lang.h"

#include <unistd.h>



double
to_milliseconds(struct timespec* time) 
{
	return (double)time->tv_sec*1000 + ((double)time->tv_nsec/1000000);
}

void
diff(struct timespec* res,struct timespec *start,struct timespec *end)
{
	if (res == NULL) return;
	
	res -> tv_sec = end -> tv_sec - start -> tv_sec;
	res -> tv_nsec = end -> tv_nsec - start -> tv_nsec;
	if (res -> tv_nsec < 0) 
	{
		res -> tv_nsec = res -> tv_nsec + 1000000000;
		res -> tv_sec --;
	}
}


/******************************************************************************************************
 * 
 * UTILITY FUNCTIONS FOR MANAGING BYTE ARRAYS
 * 
 */

GByteArray* as_gbyte_array(JNIEnv *env, jbyteArray array) {
    // buf should be deleted manually
    int len = (*env)->GetArrayLength (env, array);
    unsigned char* buf = (unsigned char*) malloc(len * sizeof(unsigned char));
    (*env)->GetByteArrayRegion (env, array, 0, len, (jbyte*)(buf));
    GByteArray* res = (GByteArray*)g_byte_array_new();
    res = (GByteArray*)g_byte_array_append(res, buf, len);
    free(buf);
    return res;
}

jbyteArray as_jbyte_array(JNIEnv *env, unsigned char* buf, int len) {
    jbyteArray array = (*env)->NewByteArray (env, len);
    (*env)->SetByteArrayRegion (env, array, 0, len, (jbyte*)(buf));
    return array;
}

void read_cph_array(JNIEnv *env, jbyteArray array, GByteArray** cph_buf) {
    int i;
    int len;
    int j=0;
    
    int arrayLen = (*env)->GetArrayLength (env, array);
    unsigned char* buf = (unsigned char*) malloc(arrayLen * sizeof(unsigned char));
    (*env)->GetByteArrayRegion (env, array, 0, arrayLen, (jbyte*)(buf));

    *cph_buf = g_byte_array_new();

    /* read cph buf */
    len = 0;
    for( i = 3; i >= 0; i-- )
            len |= buf[j++]<<(i*8);
    //g_byte_array_set_size(*cph_buf, len);
    g_byte_array_append(*cph_buf, &buf[j], len);
//	printf("[common.c][read_cpabe_file][cph_buf->len] %u\n", cph_buf->len);

//        printf("cph_buf->len = %u\n", len);
//        printf("cph_buf->data = %s\n", (*cph_buf)->data);
    
    free(buf);
}

unsigned char* write_cph_array(JNIEnv *env, GByteArray* cph_buf, int* totalLen) {
    int i;
    int len = 4 + cph_buf->len; // Adding 4 to write file size
    *totalLen = len;
    unsigned char* buf = (unsigned char*) malloc(len * sizeof(unsigned char));
    int j=0;
    
    /* write cph_buf */
    for( i = 3; i >= 0; i-- ) {
        buf[j++] = (unsigned char) ((cph_buf->len & 0xff<<(i*8))>>(i*8));
        //fputc((cph_buf->len & 0xff<<(i*8))>>(i*8), f);
    }
    for (i=0; i<cph_buf->len ; i++) {
        buf[j++] = cph_buf->data[i];
    }
    //fwrite(cph_buf->data, 1, cph_buf->len, f);
    
//     printf("cph_buf->len = %u\n", cph_buf->len);
//     printf("cph_buf->data = %s\n", cph_buf->data);
    __android_log_print(ANDROID_LOG_VERBOSE, "ABE", "cph_buf->len = %u\n",cph_buf->len);
    __android_log_print(ANDROID_LOG_VERBOSE, "ABE", "cph_buf->data = %s\n",cph_buf->data);
//     jbyteArray array = (*env)->NewByteArray (env, len);
//     (*env)->SetByteArrayRegion (env, array, 0, len, (jbyte*)(buf));
//     
//     free(buf);
//     return array;
    return buf;
}

void read_aes_array(JNIEnv *env, jbyteArray array, int* file_len, GByteArray** aes_buf) {
    int i;
    int len;
    int j=0;

    int arrayLen = (*env)->GetArrayLength (env, array);
    unsigned char* buf = (unsigned char*) malloc(arrayLen * sizeof(unsigned char));
    (*env)->GetByteArrayRegion (env, array, 0, arrayLen, (jbyte*)(buf));

    *aes_buf = g_byte_array_new();

    //printf("*** [common][read_cpabe_file] ***\n");

    /* read real file len as 32-bit big endian int */
    *file_len = 0;
    for( i = 3; i >= 0; i-- )
            *file_len |= buf[j++]<<(i*8);

    //printf("file_len = %d\n", *file_len);

    /* read aes buf */
    len = 0;
    for( i = 3; i >= 0; i-- )
            len |= buf[j++]<<(i*8);
    //g_byte_array_set_size(*aes_buf, len);
    g_byte_array_append(*aes_buf, &buf[j], len);

    //printf("aes_buf->len = %u\n", len);
    //printf("aes_buf->data = %s\n", (*aes_buf)->data);
    
    free(buf);
}

unsigned char* write_aes_array(JNIEnv* env, int file_len, GByteArray* aes_buf, int* totalLen) {
    int i;
    int len = 8 + aes_buf->len;
    *totalLen = len;
    unsigned char* buf = (unsigned char*) malloc(len * sizeof(unsigned char));
    int j=0;

    //printf("*** [common][write_cpabe_file] ***\n");
    
    /* write real file len as 32-bit big endian int */
    for( i = 3; i >= 0; i-- ) {
        buf[j++] = (unsigned char)((file_len & 0xff<<(i*8))>>(i*8));
        //fputc((file_len & 0xff<<(i*8))>>(i*8), f);
    }

    //printf("file_len = %d\n", file_len);

    /* write aes_buf */
    for( i = 3; i >= 0; i-- ) {
        buf[j++] = (unsigned char) ((aes_buf->len & 0xff<<(i*8))>>(i*8));
        //fputc((aes_buf->len & 0xff<<(i*8))>>(i*8), f);
    }
    for (i=0 ; i<aes_buf->len ; i++) {
        buf[j++] = aes_buf->data[i];
    }
    //fwrite(aes_buf->data, 1, aes_buf->len, f);

//     printf("aes_buf->len = %u\n", aes_buf->len);
//     printf("aes_buf->data = %s\n", aes_buf->data);
    __android_log_print(ANDROID_LOG_VERBOSE, "ABE", "aes_buf->len = %u\n",aes_buf->len);
    __android_log_print(ANDROID_LOG_VERBOSE, "ABE", "aes_buf->data = %s\n",aes_buf->data);
    
//     jbyteArray array = (*env)->NewByteArray (env, len);
//     (*env)->SetByteArrayRegion (env, array, 0, len, (jbyte*)(buf));
//     
//     free(buf);
//     return array;
    return buf;
}

/*
 * 
 * 
 *******************************************************************************************************/

JNIEXPORT jfloat
JNICALL Java_edu_dce_nfc_cardemulator_abe_NativeCPABE_getTick(JNIEnv *env,jobject thisObj)
{
	return sysconf(_SC_CLK_TCK);
}

JNIEXPORT jdouble 
JNICALL Java_edu_dce_nfc_cardemulator_abe_NativeCPABE_setup (JNIEnv *env, jobject thisObj,
		jstring pubFile,
		jstring mskFile,
		jint parameters_type)
{
	struct timespec start,d,end;
	clock_gettime(CLOCK_REALTIME,&start);
   
	bswabe_pub_t* pub;
	bswabe_msk_t* msk;

	bswabe_setup(&pub,&msk,parameters_type);


	const char *pub_file = (*env) -> GetStringUTFChars(env, pubFile, 0);
	const char *msk_file = (*env) -> GetStringUTFChars(env, mskFile, 0);

	spit_file(pub_file, bswabe_pub_serialize(pub), 1);
	spit_file(msk_file, bswabe_msk_serialize(msk), 1);

		// Release Strings
	(*env) -> ReleaseStringUTFChars(env,pubFile,pub_file);
	(*env) -> ReleaseStringUTFChars(env,mskFile,msk_file);
	
	// Return the total execution time
	clock_gettime(CLOCK_REALTIME,&end);
	diff(&d,&start,&end);	
	return to_milliseconds(&d);

}


JNIEXPORT jdouble 
JNICALL Java_edu_dce_nfc_cardemulator_abe_NativeCPABE_keygen (JNIEnv *env, jobject thisObj, 
	jstring pubFile, 
	jstring mskFile, 
	jstring prvFile,
	jstring jattributes,
	jint jnum_attr)

{
        element_t u_k;
	// Retrive the number of attributes
	int num_attr = (int) jnum_attr;//(*env) -> GetArrayLength(env,attributes);
	// Initialize the array of attributes
	char **c_attrs = 0;
	// Allocate
	c_attrs = malloc((num_attr + 1) * sizeof(char*));
	
/*	short int j = 0;*/
/*	for (;j < num_attr;j++) */
/*	{*/
/*		jobject el = (*env) -> GetObjectArrayElement(env,attributes, j);*/
/*		char* attribute = (char*) (*env) -> GetStringUTFChars(env, (jstring) el,0);*/
/*		c_attrs[j] = attribute;*/
/*	}*/
	
	const char *attributes = (*env) -> GetStringUTFChars(env, jattributes, 0);
	
	char* attributes_split = strdup(attributes);
	char* token = strsep(&attributes_split, " ");
	short int j = 0;
	while (token != NULL) {
		c_attrs[j] = token;
		token = strsep(&attributes_split, " ");
		j++;
	}
	
	
	bswabe_pub_t* pub;
	bswabe_msk_t* msk;
	bswabe_prv_t* prv;

	// String conversion: jstring --> const char*
	const char *pub_file = (*env) -> GetStringUTFChars(env, pubFile, 0);
	const char *msk_file = (*env) -> GetStringUTFChars(env, mskFile, 0);
	const char *prv_file = (*env) -> GetStringUTFChars(env, prvFile, 0);
        
        const char *id_file_name = g_strdup_printf("%s.id", prv_file);
	
	// Retrive public key and master key from file
	GByteArray* pb = (GByteArray*) suck_file(pub_file);
	GByteArray* ms = (GByteArray*) suck_file(msk_file);

	pub = bswabe_pub_unserialize(pb, 1);
	msk = bswabe_msk_unserialize(pub, ms, 1);

	struct timespec start,d,end;
	clock_gettime(CLOCK_REALTIME,&start);

	// Generate the private key corresponding to the given attributes
	prv = bswabe_keygen(pub, msk, c_attrs,num_attr, u_k);
	
	// Return the total execution time
	clock_gettime(CLOCK_REALTIME,&end);
	diff(&d,&start,&end);	
	
	// Serialize the private key into a file
	spit_file(prv_file, bswabe_prv_serialize(prv), 1);
        
        /*file containing friend CPABE ID*/
        FILE* fp_users;
        fp_users = fopen(id_file_name, "w");
        if(fp_users)
        {
            element_fprintf(fp_users, "%B\n", u_k);
            fclose(fp_users);
        }
	
	// Clean everything:
/*	bswabe_prv_free(prv);*/
	free(c_attrs);
	bswabe_pub_free(pub);
	
	// Release Strings
	(*env) -> ReleaseStringUTFChars(env,pubFile,pub_file);
	(*env) -> ReleaseStringUTFChars(env,mskFile,msk_file);
	(*env) -> ReleaseStringUTFChars(env,prvFile,prv_file);
	(*env) -> ReleaseStringUTFChars(env,jattributes,attributes);
	
	return to_milliseconds(&d);
	
}


JNIEXPORT jbyteArray 
JNICALL Java_edu_dce_nfc_cardemulator_abe_NativeCPABE_enc (JNIEnv *env, jobject thisObj, 
	jbyteArray pubKey, 
	jstring jpolicy,
	jbyteArray input) 
{
	int keep = 1;
	
	bswabe_pub_t* pub;
	bswabe_cph_t* cph;
	int file_len;
	GByteArray* plt;
	GByteArray* cph_buf;
	GByteArray* aes_buf;
	element_t m;

	const char *policy 		= (*env) -> GetStringUTFChars(env, jpolicy, 0);
        
	policy = parse_policy_lang((char*)policy);

	pub = bswabe_pub_unserialize((GByteArray*) as_gbyte_array(env, pubKey), 1);

	struct timespec start,d,end;
	clock_gettime(CLOCK_REALTIME,&start);
	
 	if( !(cph = bswabe_enc(pub, m, (char*)policy)) )
 	{
 		__android_log_print(ANDROID_LOG_VERBOSE, "ABE", "%s",bswabe_error());
 		return (*env)->NewByteArray (env, 0);
 	}
 	
 	// Return the time needed to encrypt
	clock_gettime(CLOCK_REALTIME,&end);
	diff(&d,&start,&end);	
 	
	cph_buf = bswabe_cph_serialize(cph, 0);
	plt = (GByteArray*) as_gbyte_array(env, input);
	file_len = plt->len;
	
	__android_log_print(ANDROID_LOG_VERBOSE,"ABE", "File length = %d",file_len);
	
	aes_buf = (GByteArray*) aes_128_cbc_encrypt(plt, m);
	g_byte_array_free(plt, 1);
	element_clear(m);

        // Concatenating cpabe and cpaes buffer to return from this method
        int outCpaesBufLen, outCpabeBufLen;
        unsigned char* outCpaesBuf = write_aes_array(env, file_len, aes_buf, &outCpaesBufLen); // Creates file equivalend array of aes buffer
        unsigned char* outCpabeBuf = write_cph_array(env, cph_buf, &outCpabeBufLen); // Created file equivalent array of cipher buffer
        jbyteArray result = (*env)->NewByteArray (env, outCpabeBufLen +  outCpaesBufLen);
        (*env)->SetByteArrayRegion (env, result, 0, outCpabeBufLen, (jbyte*)(outCpabeBuf));
        (*env)->SetByteArrayRegion (env, result, outCpabeBufLen, outCpaesBufLen, (jbyte*)(outCpaesBuf));
        free(outCpabeBuf); free(outCpaesBuf);
        
	g_byte_array_free(cph_buf, 1);
	g_byte_array_free(aes_buf, 1);
        
	// Release Strings
	(*env) -> ReleaseStringUTFChars(env,jpolicy,policy);
        
	bswabe_cph_free(cph, 0);
	bswabe_pub_free(pub);	
        
	//return to_milliseconds(&d);
        return result;
}


JNIEXPORT jbyteArray 
JNICALL Java_edu_dce_nfc_cardemulator_abe_NativeCPABE_dec (JNIEnv *env, jobject thisObj, 
	jbyteArray pubKey, 
	jbyteArray prvKey,
        jbyteArray lambda,
	jbyteArray inCpabe,
        jbyteArray inCpaes) 
{
	bswabe_pub_t* pub;
	bswabe_prv_t* prv;
        int file_len;
	GByteArray* aes_buf;
	GByteArray* plt;
	GByteArray* cph_buf;
	bswabe_cph_t* cph;
	element_t m;
        element_t lambda_k;
        int offset = 0;

	int   keep       = 1;

	struct timespec start,d,end;
        
	pub = bswabe_pub_unserialize((GByteArray*) as_gbyte_array(env, pubKey), 1);
	prv = bswabe_prv_unserialize(pub, (GByteArray*) as_gbyte_array(env, prvKey), 1);
        
	bswabe_element_init_Zr(lambda_k, pub);
	unserialize_element((GByteArray*)as_gbyte_array(env, lambda), &offset, lambda_k);
        
        read_cph_array(env, inCpabe, &cph_buf);
        read_aes_array(env, inCpaes, &file_len, &aes_buf);
        
	cph = bswabe_cph_unserialize(pub, cph_buf, 1, 1); //last 1 for proxy
        
	
	clock_gettime(CLOCK_REALTIME,&start);

	if( !bswabe_dec(pub, prv, cph, m, lambda_k) )
	{
		__android_log_print(ANDROID_LOG_VERBOSE, "ABE", "%s",bswabe_error());
 		return (*env)->NewByteArray (env, 0);
	}
	
	
	// Return the time needed to decrypt
	clock_gettime(CLOCK_REALTIME,&end);
	
		
	plt = (GByteArray*) aes_128_cbc_decrypt(aes_buf, m);
	g_byte_array_set_size(plt, file_len);
        jbyteArray output = as_jbyte_array(env, plt->data, plt->len);
	
        
	element_clear(m);
        element_clear(lambda_k);
        
	
	g_byte_array_free(aes_buf, 1);
	g_byte_array_free(plt, 1);
// 	g_byte_array_free(cph_buf, 1);

	bswabe_cph_free(cph, 1);
	bswabe_pub_free(pub);
	
	diff(&d,&start,&end);	
        return output;
// 	return to_milliseconds(&d);
}

JNIEXPORT jdouble 
JNICALL Java_edu_dce_nfc_cardemulator_abe_NativeCPABE_delegate (JNIEnv *env, jobject thisObj, 
	jstring pubFile, 
	jstring mskFile,
        jstring prvFile,
	jstring u_kFile,
        jstring outFile,
	jstring jattributes,
        jint jnum_attr) 
{
    
        // Retrive the number of attributes
	int num_attr = (int) jnum_attr;//(*env) -> GetArrayLength(env,attributes);
	// Initialize the array of attributes
	char **c_attrs = 0;
	// Allocate
	c_attrs = malloc((num_attr + 1) * sizeof(char*));
	
/*	short int j = 0;*/
/*	for (;j < num_attr;j++) */
/*	{*/
/*		jobject el = (*env) -> GetObjectArrayElement(env,attributes, j);*/
/*		char* attribute = (char*) (*env) -> GetStringUTFChars(env, (jstring) el,0);*/
/*		c_attrs[j] = attribute;*/
/*	}*/
	
	const char *attributes = (*env) -> GetStringUTFChars(env, jattributes, 0);
	
	char* attributes_split = strdup(attributes);
	char* token = strsep(&attributes_split, " ");
	short int j = 0;
	while (token != NULL) {
		c_attrs[j] = token;
		token = strsep(&attributes_split, " ");
		j++;
	}
	
        bswabe_pub_t* pub;
	bswabe_msk_t* msk;
	bswabe_prv_t* prv;
	bswabe_del_prv_t* del_prv;
	FILE* fp_u_k;
        char u_k_str[100];
        
        struct timespec start,d,end;

	const char *pub_file 	= (*env) -> GetStringUTFChars(env, pubFile, 0);
	const char *msk_file 	= (*env) -> GetStringUTFChars(env, mskFile, 0);
        const char *prv_file = (*env) -> GetStringUTFChars(env, prvFile, 0);
	const char* u_k_file  	= (*env) -> GetStringUTFChars(env, u_kFile, 0);
        const char* out_file  	= (*env) -> GetStringUTFChars(env, outFile, 0);
                
        pub = bswabe_pub_unserialize((GByteArray*)suck_file(pub_file), 1);
	msk = bswabe_msk_unserialize(pub, (GByteArray*)suck_file(msk_file), 1);
	prv = bswabe_prv_unserialize(pub, (GByteArray*)suck_file(prv_file), 1);
        
        fp_u_k = fopen( u_k_file, "r");
	fscanf(fp_u_k, "%s", u_k_str);
        
        clock_gettime(CLOCK_REALTIME,&start);
        
        del_prv = bswabe_delegate(pub, msk, prv, u_k_str, c_attrs, num_attr);
        
        clock_gettime(CLOCK_REALTIME,&end);
        
        spit_file(out_file, bswabe_del_prv_serialize(del_prv), 1);

	if(fp_u_k)
		fclose(fp_u_k);
        
        (*env) -> ReleaseStringUTFChars(env,pubFile,pub_file);
	(*env) -> ReleaseStringUTFChars(env,prvFile,prv_file);
	(*env) -> ReleaseStringUTFChars(env,mskFile,msk_file);
	(*env) -> ReleaseStringUTFChars(env,u_kFile,u_k_file);
        (*env) -> ReleaseStringUTFChars(env,jattributes,attributes);
        
        diff(&d,&start,&end);	
	return to_milliseconds(&d);
}

JNIEXPORT jbyteArray 
JNICALL Java_edu_dce_nfc_cardemulator_abe_NativeCPABE_convert (JNIEnv *env, jobject thisObj, 
	jbyteArray pubKey, 
	jbyteArray input,
        jbyteArray revoke,
	jstring u_k) 
{
        int i;
	bswabe_pub_t* pub;
	bswabe_cph_t* cph;
	bswabe_point* rvk;

	element_t lambda_k;
	GByteArray* cph_buf;
	GByteArray* l_k;

	int file_len;
        
        struct timespec start,d,end;
        
        const char* u_k_str  	= (*env) -> GetStringUTFChars(env, u_k, 0); 
        char* u_k_str_temp = u_k_str;
        
        pub = bswabe_pub_unserialize((GByteArray*)as_gbyte_array(env, pubKey), 1);
        read_cph_array(env, input, &cph_buf);
                
        cph = bswabe_cph_unserialize(pub, cph_buf, 1, 0); //

	rvk = bswabe_point_unserialize(pub, (GByteArray*)as_gbyte_array(env, revoke), 1);

	element_t* lambda_i_ps = bswabe_convert(pub, rvk);
        
        clock_gettime(CLOCK_REALTIME,&start);
        
        convert(pub, cph, rvk, u_k_str_temp, lambda_k, lambda_i_ps);
                
        clock_gettime(CLOCK_REALTIME,&end);
        
        int proxy_key_size = 0;

	for(i=0; i<REVOKE_T; i++)
		proxy_key_size += element_length_in_bytes(lambda_i_ps[i]);
        
	cph_buf = bswabe_cph_serialize(cph, 1); // 1 for proxy

        int len;
        unsigned char* outputBuf = write_cph_array(env, cph_buf, &len);
        
	l_k = g_byte_array_new();
        serialize_element(l_k, lambda_k);
                
        __android_log_print(ANDROID_LOG_VERBOSE, "ABE", "Point %d %d", len, l_k->len);
        jbyteArray result = (*env)->NewByteArray (env, len +  l_k->len);
        (*env)->SetByteArrayRegion (env, result, 0, len, (jbyte*)(outputBuf));
        (*env)->SetByteArrayRegion (env, result, len, l_k->len, (jbyte*)(l_k->data));
        

        free(outputBuf);

	g_byte_array_free(cph_buf, 1);
        g_byte_array_free(l_k, 1);
        
	element_clear(lambda_k);

	for(i=0; i<REVOKE_T; i++)
		element_clear(lambda_i_ps[i]);
        
//         if (!keep)
// 		unlink(in_file);
        (*env) -> ReleaseStringUTFChars(env,u_k,u_k_str);
        diff(&d,&start,&end);	
                
        return result;
// 	return to_milliseconds(&d);
}

JNIEXPORT jdouble 
JNICALL Java_edu_dce_nfc_cardemulator_abe_NativeCPABE_revoke (JNIEnv *env, jobject thisObj, 
	jstring pubFile, 
	jstring mskFile,
        jstring outFile,
        jobjectArray users) 
{
        bswabe_pub_t* pub;
	bswabe_msk_t* msk;
	bswabe_point* revoke_list;
	char** rl = 0;
	FILE* fp;
        
        int i = 0, j = 0;
        
        struct timespec start,d,end;

	const char *pub_file 	= (*env) -> GetStringUTFChars(env, pubFile, 0);
	const char *msk_file 	= (*env) -> GetStringUTFChars(env, mskFile, 0);
        const char *out_file 	= (*env) -> GetStringUTFChars(env, outFile, 0);
        
        int num_users = (*env)->GetArrayLength(env, users);
        __android_log_print(ANDROID_LOG_VERBOSE, "ABE", "%d", num_users);
                
        if(num_users > REVOKE_T)
	{
		die("Trying to revoke %d users. Can revoke at most %d users.\n", num_users, REVOKE_T);
	}
		
	rl = malloc((num_users + 1) * sizeof(char *));
	for(j=0; j<num_users; j++, i++)
	{
                jstring user_jstring = (*env) -> GetObjectArrayElement(env, users, j);
                const char* user = (*env) -> GetStringUTFChars(env, user_jstring, 0);
		fp = fopen(user, "r");
		rl[j] = malloc(50);
		fscanf(fp, "%s", rl[j]);
		fclose(fp);
                (*env) -> ReleaseStringUTFChars(env,user_jstring,user);
	}
	rl[j] = 0;
                
        pub = bswabe_pub_unserialize((GByteArray*)suck_file(pub_file), 1);
	msk = bswabe_msk_unserialize(pub, (GByteArray*)suck_file(msk_file), 1);
        
        clock_gettime(CLOCK_REALTIME,&start);
        
	revoke_list = bswabe_revoke(pub, msk, rl, (num_users));
        
        clock_gettime(CLOCK_REALTIME,&end);
        
	spit_file(out_file, bswabe_point_serialize(revoke_list), 1);
                
        (*env) -> ReleaseStringUTFChars(env,pubFile,pub_file);
	(*env) -> ReleaseStringUTFChars(env,mskFile,msk_file);
        (*env) -> ReleaseStringUTFChars(env,mskFile,out_file);
        
        diff(&d,&start,&end);	
        
	return to_milliseconds(&d);
}

JNIEXPORT jdouble 
JNICALL Java_edu_dce_nfc_cardemulator_abe_NativeCPABE_decDelegated (JNIEnv *env, jobject thisObj, 
	jstring pubFileA,
        jstring pubFileB,
        jstring delPrvFile,
        jstring lambdaABFile,
        jstring lambdaBCFile,
	jstring inFileAB,
        jstring inFileBC,
        jstring outFile) 
{
        bswabe_pub_t* pubA;
	bswabe_pub_t* pubB;
	bswabe_del_prv_t* del_prv;
	element_t lambda_AB;
	element_t lambda_BC;
	element_t m;

	int file_lenAB = 0;
	int file_lenBC = 0;

	int offset1 = 0;
	int offset2 = 0;

	GByteArray* aes_bufAB;
	GByteArray* pltAB;
	GByteArray* cph_bufAB;

	GByteArray* aes_bufBC;
	GByteArray* pltBC;
	GByteArray* cph_bufBC;

	bswabe_cph_t* cphAB;
	bswabe_cph_t* cphBC;
        
        struct timespec start,d,end;
        
        const char *pub_fileA 	= (*env) -> GetStringUTFChars(env, pubFileA, 0);
        const char *pub_fileB 	= (*env) -> GetStringUTFChars(env, pubFileB, 0);
        const char *del_prv_file= (*env) -> GetStringUTFChars(env, delPrvFile, 0);
        const char *lambdaAB_file= (*env) -> GetStringUTFChars(env, lambdaABFile, 0);
        const char *lambdaBC_file= (*env) -> GetStringUTFChars(env, lambdaBCFile, 0);
        const char *in_fileAB= (*env) -> GetStringUTFChars(env, inFileAB, 0);
        const char *in_fileBC= (*env) -> GetStringUTFChars(env, inFileBC, 0);
        const char *out_file= (*env) -> GetStringUTFChars(env, outFile, 0);
                
        pubA = bswabe_pub_unserialize((GByteArray*)suck_file(pub_fileA), 1);
	pubB = bswabe_pub_unserialize((GByteArray*)suck_file(pub_fileB), 1);
	del_prv = bswabe_del_prv_unserialize(pubB, (GByteArray*)suck_file(del_prv_file), 1);

	bswabe_element_init_Zr(lambda_AB, pubA); /*from proxy of A*/
	unserialize_element((GByteArray*)suck_file(lambdaAB_file), &offset1, lambda_AB);

	bswabe_element_init_Zr(lambda_BC, pubB); /*from proxy of B*/
	unserialize_element((GByteArray*)suck_file(lambdaBC_file), &offset2, lambda_BC);

	char* in_aes_file = g_strdup_printf("%s.cpaes", g_strndup(in_fileAB, strlen(in_fileAB) - 12));
	read_aes_file(in_aes_file, &file_lenAB, &aes_bufAB);

	read_cph_file(in_fileAB, &cph_bufAB);
	cphAB = bswabe_cph_unserialize(pubA, cph_bufAB, 1, 1);

	read_cph_file(in_fileBC, &cph_bufBC);
	cphBC = bswabe_cph_unserialize(pubB, cph_bufBC, 1, 1);

        clock_gettime(CLOCK_REALTIME,&start);
	if( !bswabe_dec_delegated( pubB, del_prv, cphAB, cphBC,  m,  lambda_AB,  lambda_BC) )
		die("%s", bswabe_error());
        clock_gettime(CLOCK_REALTIME,&end);

	bswabe_cph_free(cphAB, 1);
	bswabe_cph_free(cphBC, 1);

	pltAB = (GByteArray*)aes_128_cbc_decrypt(aes_bufAB, m);
	g_byte_array_set_size(pltAB, file_lenAB);
	g_byte_array_free(aes_bufAB, 1);

	spit_file(out_file, pltAB, 1);

	element_clear(m);
	element_clear(lambda_AB);
	element_clear(lambda_BC);

// 	if(!keep)
// 	{
// 		unlink(in_fileAB);
// 		unlink(in_fileBC);
// 	}
        
        (*env) -> ReleaseStringUTFChars(env,pubFileA,pub_fileA);
	(*env) -> ReleaseStringUTFChars(env,pubFileB,pub_fileB);
        (*env) -> ReleaseStringUTFChars(env,delPrvFile,del_prv_file);
	(*env) -> ReleaseStringUTFChars(env,lambdaABFile,lambdaAB_file);
        (*env) -> ReleaseStringUTFChars(env,lambdaBCFile,lambdaBC_file);
	(*env) -> ReleaseStringUTFChars(env,inFileAB,in_fileAB);
        (*env) -> ReleaseStringUTFChars(env,inFileBC,in_fileBC);
        (*env) -> ReleaseStringUTFChars(env,outFile,out_file);
        
        diff(&d,&start,&end);	
	return to_milliseconds(&d);
}

