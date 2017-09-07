#ifndef _AMF_UTIL_H_
#define _AMF_UTIL_H_
#include "flv.h"

inline char* amf_put_byte(char *output, uint8_t nVal) {
	output[0] = nVal;
	return output+1;
}

inline char* amf_put_be16(char *output, uint16_t nVal) {   
	output[0] = char(nVal >> 8);
	output[1] = char(nVal & 0xff);
	return output+2;
}

inline char* amf_put_be24(char *output,uint32_t nVal) {
	output[0] = char(nVal >> 16);
	output[1] = char(nVal >> 8);
	output[2] = char(nVal & 0xff);
	return output+3;
}

inline char* amf_put_be32(char *output, uint32_t nVal) {
	output[0] = char(nVal >> 24); 
	output[1] = char(nVal >> 16);
	output[2] = char(nVal >> 8);
	output[3] = char(nVal & 0xff);
	return output+4;
}

inline char* amf_put_be64(char *output, uint64_t nVal) {    
	output = amf_put_be32(output, uint32_t(nVal>>32));
	output = amf_put_be32(output, uint32_t(nVal));
	return output;
}

inline char* amf_put_mem(char *output, const char *str, uint16_t len) {
	output = amf_put_be16(output, len);
	memcpy(output, str, len);
	return output+len;
}

inline char* amf_put_string(char *output, const char *str, uint16_t len) {
	*output++ = AMF_DATA_TYPE_STRING;
	return amf_put_mem(output, str, len);
}

inline char* amf_put_number(char *output, double d) {
	char *ci, *co;
	*output++ = AMF_DATA_TYPE_NUMBER;  /* type: Number */    

	ci = (char *)&d;
	co = (char *)output;
	co[0] = ci[7];
	co[1] = ci[6];
	co[2] = ci[5];
	co[3] = ci[4];
	co[4] = ci[3];
	co[5] = ci[2];
	co[6] = ci[1];
	co[7] = ci[0];
	return output+8;
}

inline char* amf_put_bool(char *output, int b) {
	output[0] = AMF_DATA_TYPE_BOOL;
	output[1] = !!b;
	return output+2;
}

#define amf_put_sz(output, sz)    amf_put_string(output, sz, sizeof(sz)-1)
#define amf_put_str(output, str)  amf_put_mem(output, str, uint16_t(strlen(str)))
#define amf_put_kv(output, key, value) \
	/*output = */amf_put_str(output, key); \
	output = amf_put_number(output, value);

#endif
