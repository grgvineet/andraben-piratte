#include <string.h>
#include <jni.h>
#include <stdio.h>
#include <time.h>
#include <android/log.h>
#include <unistd.h>
#include <glib/glib.h>
#include <include/pbc.h>
#include "kpabe/common.h"
#include "libcelia/celia.h"
#include "policy_lang.h"



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


JNIEXPORT jfloat
JNICALL Java_com_example_kpabe_NativeCPABE_getTick(JNIEnv *env,jobject thisObj)
{
	return sysconf(_SC_CLK_TCK);
}

 
JNIEXPORT jdouble 
JNICALL Java_com_example_kpabe_NativeKPABE_setup (JNIEnv *env, jobject thisObj, 
	jstring pubFile, 
	jstring mskFile,
	jint type,
	jstring juniverse,
	jint jnum_attr) 
{
   
	FILE* file = fopen("/sdcard/kp_hello.txt","w+");

    if (file != NULL)
    {
        fputs("HELLO WORLD!\n", file);
        fflush(file);
        fclose(file);
    }
    
    struct timespec start,d,end;
	clock_gettime(CLOCK_REALTIME,&start);
    
    // Retrive the number of attributes in the universe
	int num_attr = (int) jnum_attr;//(*env) -> GetArrayLength(env,universe);
	// Initialize the array
	char **c_univ = 0;
	// Allocate
	c_univ = malloc((num_attr + 1) * sizeof(char*));
    
/*    short int j = 0;*/
/*	for (;j < num_attr;j++) */
/*	{*/
/*		jobject el = (*env) -> GetObjectArrayElement(env,universe,j);*/
/*		char* attribute = (char*) (*env) -> GetStringUTFChars(env, (jstring) el,0);*/
/*		c_univ[j] = attribute;*/
/*	}*/
    
    const char *universe = (*env) -> GetStringUTFChars(env, juniverse, 0);
	
	char* universe_split = strdup(universe);
	char* token = strsep(&universe_split, " ");
	short int j = 0;
	while (token != NULL) {
		c_univ[j] = token;
		token = strsep(&universe_split, " ");
		j++;
	}
    
	kpabe_pub_t* pub;
	kpabe_msk_t* msk;

	kpabe_setup(&pub, &msk, c_univ, num_attr,type);
	const char *pub_file = (*env) -> GetStringUTFChars(env, pubFile, 0);
	const char *msk_file = (*env) -> GetStringUTFChars(env, mskFile, 0);
	
	spit_file(pub_file, kpabe_pub_serialize(pub), 1);
	spit_file(msk_file, kpabe_msk_serialize(msk), 1);
	
	(*env) -> ReleaseStringUTFChars(env,juniverse,universe);
	
	// Return the time needed to setup
	clock_gettime(CLOCK_REALTIME,&end);
	diff(&d,&start,&end);	
	return to_milliseconds(&d);

}


JNIEXPORT jdouble 
JNICALL Java_com_example_kpabe_NativeKPABE_keygen (JNIEnv *env, jobject thisObj, 
	jstring pubFile, 
	jstring mskFile, 
	jstring prvFile,
	jstring jpolicy)

{

	struct timespec start,d,end;

	kpabe_pub_t* pub;
	kpabe_msk_t* msk;
	kpabe_prv_t* prv;


	const char *pub_file 	= (*env) -> GetStringUTFChars(env, pubFile, 0);
	const char *msk_file 	= (*env) -> GetStringUTFChars(env, mskFile, 0);
	const char *prv_file 	= (*env) -> GetStringUTFChars(env, prvFile, 0);
	const char *policy 		= (*env) -> GetStringUTFChars(env, jpolicy, 0);
	pub = kpabe_pub_unserialize(suck_file(pub_file), 1);
	msk = kpabe_msk_unserialize(pub, suck_file(msk_file), 1);

	clock_gettime(CLOCK_REALTIME,&start);
	
	// Parse the policy in order to convert it in a threshold-like lang
	policy = parse_policy_lang((char*)policy);

  	if( !(prv = kpabe_keygen(pub, msk, policy)) ) {
  		__android_log_print(ANDROID_LOG_VERBOSE, "ABE", "%s",kpabe_error());
 		return 0.0;
  	}
	
	clock_gettime(CLOCK_REALTIME,&end);
	free(policy);
	
	spit_file(prv_file, kpabe_prv_serialize(prv), 1);
	
	diff(&d,&start,&end);	
	return to_milliseconds(&d);
	
}


JNIEXPORT jdouble 
JNICALL Java_com_example_kpabe_NativeKPABE_enc (JNIEnv *env, jobject thisObj, 
	jstring pubFile, 
	jstring inFile,
	jstring jattributes,
	jint jnum_attr) 
{

	int keep = 1;

	struct timespec start,d,end;

	kpabe_pub_t* pub;
	kpabe_cph_t* cph;
	int file_len;
	GByteArray* plt;
	GByteArray* cph_buf;
	GByteArray* aes_buf;
	element_t m;


	// Retrive the number of attributes to enforce on the ciphertext
	int num_attr = (int) jnum_attr;
	// Initialize the array
	char **c_attrs = 0;
	// Allocate
	c_attrs = malloc((num_attr + 1) * sizeof(char*));

	const char *attributes = (*env) -> GetStringUTFChars(env, jattributes, 0);
	
	char* attributes_split = strdup(attributes);
	char* token = strsep(&attributes_split, " ");
	short int j = 0;
	while (token != NULL) {
		c_attrs[j] = token;
		token = strsep(&attributes_split, " ");
		j++;
	}


	const char *pub_file 	= (*env) -> GetStringUTFChars(env, pubFile, 0);
	const char *in_file 	= (*env) -> GetStringUTFChars(env, inFile, 0);
	char* out_file 			= g_strdup_printf("%s.kpabe", in_file);

	pub = kpabe_pub_unserialize(suck_file(pub_file), 1);

	clock_gettime(CLOCK_REALTIME,&start);

  	if( !(cph = kpabe_enc(pub, m, c_attrs, num_attr)) )
  	{
		__android_log_print(ANDROID_LOG_INFO, "ABE", "ERROR: %s", kpabe_error());
		return 0;
	}

	clock_gettime(CLOCK_REALTIME,&end);

	free(c_attrs);

	cph_buf = kpabe_cph_serialize(cph);
	kpabe_cph_free(cph);
/*	kpabe_pub_free(pub);*/

	plt = suck_file(in_file);
	file_len = plt->len;
	aes_buf = aes_128_cbc_encrypt(plt, m);
	g_byte_array_free(plt, 1);
	element_clear(m);

	write_kpabe_file(out_file, cph_buf, file_len, aes_buf);

	g_byte_array_free(cph_buf, 1);
	g_byte_array_free(aes_buf, 1);

	
	(*env) -> ReleaseStringUTFChars(env,pubFile,pub_file);
	(*env) -> ReleaseStringUTFChars(env,inFile,in_file);
	(*env) -> ReleaseStringUTFChars(env,jattributes,attributes);

	// Return the time needed to encrypt
	diff(&d,&start,&end);	
	return to_milliseconds(&d);
}


JNIEXPORT jdouble 
JNICALL Java_com_example_kpabe_NativeKPABE_dec (JNIEnv *env, jobject thisObj, 
	jstring pubFile, 
	jstring prvFile, 
	jstring inFile,
	jstring outFile) 
{

	struct timespec start,d,end;

	int   keep       = 1;

	kpabe_pub_t* pub;
	kpabe_prv_t* prv;
	int file_len;
	GByteArray* aes_buf;
	GByteArray* plt;
	GByteArray* cph_buf;
	kpabe_cph_t* cph;
	element_t m;
	
	const char *pub_file 	= (*env) -> GetStringUTFChars(env, pubFile, 0);
	const char *prv_file 	= (*env) -> GetStringUTFChars(env, prvFile, 0);
	const char *in_file 	= (*env) -> GetStringUTFChars(env, inFile, 0);
	char* out_file 			= (*env) -> GetStringUTFChars(env, outFile, 0);

	pub = kpabe_pub_unserialize(suck_file(pub_file), 1);
	prv = kpabe_prv_unserialize(pub, suck_file(prv_file), 1);

	read_kpabe_file(in_file, &cph_buf, &file_len, &aes_buf);

	clock_gettime(CLOCK_REALTIME,&start);

	cph = kpabe_cph_unserialize(pub, cph_buf, 1);
	if( !kpabe_dec(pub, prv, cph, m) ){
		__android_log_print(ANDROID_LOG_INFO, "ABE", "ERROR: %s", kpabe_error());
		return 0;
	}
	clock_gettime(CLOCK_REALTIME,&end);
	kpabe_cph_free(cph);
	kpabe_prv_free(prv);

	g_byte_array_free(aes_buf, 1);


	element_clear(m);

	(*env) -> ReleaseStringUTFChars(env,pubFile,pub_file);
	(*env) -> ReleaseStringUTFChars(env,prvFile,prv_file);
	(*env) -> ReleaseStringUTFChars(env,inFile,in_file);
	(*env) -> ReleaseStringUTFChars(env,outFile,out_file);
	
	// Return the time needed to decrypt
	diff(&d,&start,&end);	
	return to_milliseconds(&d);
}

