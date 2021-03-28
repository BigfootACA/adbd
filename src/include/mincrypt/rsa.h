#ifndef _EMBEDDED_RSA_H_
#define _EMBEDDED_RSA_H_
#include<inttypes.h>
#define RSANUMBYTES 256
#define RSANUMWORDS (RSANUMBYTES/sizeof(uint32_t))
typedef struct RSAPublicKey {
	int len;
	uint32_t n0inv,n[RSANUMWORDS],rr[RSANUMWORDS];
	int exponent;
}RSAPublicKey;
extern int RSA_verify(const RSAPublicKey*key,const uint8_t*signature,const int len,const uint8_t* sha);
#endif
