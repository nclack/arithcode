#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

typedef uint8_t   u8;
typedef uint32_t  u32;
typedef float     real;

/*
 * encode
 * ------
 *  <out>   <*out> may be NULL or point to a preallocated buffer with <*nout> 
 *          elements.  The buffer may be reallocated if it's not big   enough,
 *          in which case the new size and adress are output via <*out> and
 *          <*nout>.
 *  <nsym>  The number of symbols.
 *  <cdf>   The cumulative distribution of the symbols 0..<nsym>-1.
 *          It should be specified as a sequence of <nsym>+1 real numbers.
 *          cdf[0] shold be 0.0, and cdf[nsym] should be 1.0.  The sequence
 *          should be monotonically increasing.  No symbol should have
 *          probability less than the resolution of the coder, which depends on
 *          the output bit depth. See the source for details.  As a rule of
 *          thumb, probabilities should be greater than 1/2^(32-bitsof(output
 *          symbol)).
 * 
 *          Note that this doesn't have to be the _actual_ probability of the
 *          symbol.  Differences will result in a slightly less efficient
 *          compression.
 *                                                                      
 * decode                                                               
 * ------                                                               
 *  <out>   output buffer, should be pre-allocated                     
 *  <nout>  must be less than or equal to the actual message size      
 *  <in>    the bit string returned in encode's <out.d>                
 *  <c>     the CDF over output symbols                                
 *  <nsym>  the number of output symbols                               
 */

void cdf_build(real **cdf, size_t *nsym, u32 *s, size_t ns);

// encode_<Tout>_<Tin>
// - Tout: u1,u4,u8,u16
// - Tin : u8,u16,u32,u64
void encode_u1_u8  (void **out, size_t *nout, uint8_t *in, size_t nin, real *cdf, size_t nsym);
void encode_u4_u8  (void **out, size_t *nout, uint8_t *in, size_t nin, real *cdf, size_t nsym);
void encode_u8_u8  (void **out, size_t *nout, uint8_t *in, size_t nin, real *cdf, size_t nsym);

void encode_u1_u16  (void **out, size_t *nout, uint16_t  *in, size_t nin, real *cdf, size_t nsym);
void encode_u4_u16  (void **out, size_t *nout, uint16_t  *in, size_t nin, real *cdf, size_t nsym);
void encode_u8_u16  (void **out, size_t *nout, uint16_t  *in, size_t nin, real *cdf, size_t nsym);

void encode_u1_u32  (void **out, size_t *nout, uint32_t *in, size_t nin, real *cdf, size_t nsym);
void encode_u4_u32  (void **out, size_t *nout, uint32_t *in, size_t nin, real *cdf, size_t nsym);
void encode_u8_u32  (void **out, size_t *nout, uint32_t *in, size_t nin, real *cdf, size_t nsym);

void encode_u1_u64  (void **out, size_t *nout, uint64_t *in, size_t nin, real *cdf, size_t nsym);
void encode_u4_u64  (void **out, size_t *nout, uint64_t *in, size_t nin, real *cdf, size_t nsym);
void encode_u8_u64  (void **out, size_t *nout, uint64_t *in, size_t nin, real *cdf, size_t nsym);

// Only use u16 outputs if u64's are supported on your system.
void encode_u16_u8 (void **out, size_t *nout, uint8_t *in, size_t nin, real *cdf, size_t nsym);
void encode_u16_u16 (void **out, size_t *nout, uint16_t  *in, size_t nin, real *cdf, size_t nsym);
void encode_u16_u32 (void **out, size_t *nout, uint32_t *in, size_t nin, real *cdf, size_t nsym);
void encode_u16_u64 (void **out, size_t *nout, uint64_t *in, size_t nin, real *cdf, size_t nsym);

// decode_<Tout>_<Tin>
// - Tout: u8,u16,u32,u64
// - Tin : u1,u4,u8,u16
void decode_u8_u1 (uint8_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);
void decode_u8_u4 (uint8_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);
void decode_u8_u8 (uint8_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);

void decode_u16_u1 (uint16_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);
void decode_u16_u4 (uint16_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);
void decode_u16_u8 (uint16_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);

void decode_u32_u1 (uint32_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);
void decode_u32_u4 (uint32_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);
void decode_u32_u8 (uint32_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);

void decode_u64_u1 (uint64_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);
void decode_u64_u4 (uint64_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);
void decode_u64_u8 (uint64_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);

// Only use u16 inputs if u64's are supported on your system.
void decode_u8_u16(uint8_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);
void decode_u16_u16(uint16_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);
void decode_u32_u16(uint32_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);
void decode_u64_u16(uint64_t **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);

#ifdef __cplusplus
}
#endif
