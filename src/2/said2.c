/*
 * Introduction
 * ------------
 *
 * Implementation after Amir Said's Algorithm 22-29[1].
 *
 * The idea here is to get a more precise idea of what it takes to implement an
 * arithmetic coder, and to get a reference implementation.
 *
 * My first attempt used floating point arithmetic, but here I'll try to use 
 * integer arithmetic only.  I'll also use a wider bit-depth for the output
 * symbols.
 *
 * Following Amir's notation, 
 *
 *  D is the number of symbols in the output alphabet.
 *  P is the number of output symbols in the active block (see Eq 2.3 in [1]).
 *    Need 2P for multiplication.
 *
 * So, have 2P*bitsof(D) = 64 (widest integer type I'll have access to)
 *
 * The smallest codable probabililty is p = D/D^P = D^(1-P).  Want to minimize
 * p wrt constraint. Enumerating the possibilities:
 *
 *   bitsof(D)  P                D^(1-P)
 *   --------   --               -------
 *   1          32                2^-32
 *   8          4    (2^8)^(-3) = 2^-24
 *   16         2                 2^-16
 *   32         1                 1
 *
 * One can see there's a efficiency verses precision trade off here (higher D
 * means less renormalizing and so better efficiency).
 *
 * I'll choose bitsof(D)=8 and P=4.
 *
 * Notes
 * -----
 *
 * An upshot of bigger symbols is that we can work on byte streams instead of
 * bit streams.  This requires a change in interface, but should be simpler.
 *
 * Search for HERE to see where I left off.
 * [ ] implement state_t's init()
 * [ ] how to do the CDF?
 * [ ] may be a good opertunity to make encoder stepwise.
 *
 * References
 * ----------
 * [1]:  Said, A. “Introduction to Arithmetic Coding - Theory and Practice.” 
 *       Hewlett Packard Laboratories Report: 2004–2076.
 *       www.hpl.hp.com/techreports/2004/HPL-2004-76.pdf
 *
 */


#include <stdint.h> // for fixed width integer types
#include <stdlib.h> // for size_t
#include <stdio.h>  // for exception handling
#include <string.h> // for memset

typedef uint8_t   u8;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef float     real;

#define ENDL "\n"
#define TRY(e) \
  do{ if(!(e)) {\
    printf("%s(%d):"ENDL "%s"ENDL "Expression evaluated as false."ENDL, \
        __FILE__,__LINE__,#e); \
    goto Error; \
  }} while(0)

//
// Byte Stream
// - push appends to end
// - pop  removes from front
// - models a FIFO stream
// - push/pop are not meant to work together.  That is, can't simultaneously
//   encode and decode on the same stream; Can't interleave push/pop.
//
typedef struct _stream_t
{ size_t nbtyes; //capacity
  size_t ibyte;  //current byte
  u8*    d;      //data
} stream_t;

static
void maybe_init(stream_t *self)
{ if(!self->d)
  { memset(self,0,sizeof(*self));
    TRY(self->d=malloc(self->nbytes=4096));
  }
  return;
Error:
  abort();
}

static
void maybe_resize(stream_t *s)
{ if(s->ibyte>=s->nbytes)
    TRY(*d = realloc(*d,s->nbytes=(1.2*s->ibyte+50)));
  return;
Error:
  abort();
}

static
void push(stream_t *self, u8 v)
{ 
  self->d[self->ibyte] = v;
  self->ibyte++;
  maybe_resize(self);
}

static u8 pop(stream_t *self)
{ if(self->ibyte>=self->nbytes)
    return 0;
  return self->d[self->ibyte++];
}

//
// Encoder/Decoder state
//

struct _state_t
{ u64      b,l,v;
  stream_t d;
  size_t   nsym;
  u64     *cdf;
} state_t;


//
// Build CDF
// 
static
u32 maximum(u32 *s, size_t n)
{ u32 max=0;
  while(n--)
    max = (s[n]>max)?s[n]:max;
  return max;
}

void cdf_build(real **cdf, size_t *M, u32 *s, size_t N)
{ size_t i,nbytes;
  *M = maximum(s,N)+1; 
  TRY( *cdf=realloc(*cdf,nbytes=sizeof(real)*(M[0]+1)) ); // cdf has M+1 elements
  memset(*cdf,0,nbytes);
  for(i=0;i<     N;++i) cdf[0][s[i]]++;                   // histogram
  for(i=0;i<M[0]  ;++i) cdf[0][s[i]]/=(real)N;            // norm
  for(i=1;i<M[0]  ;++i) cdf[0][i]   += cdf[0][i-1];       // cumsum
  for(i=M[0]+1;i>0;--i) cdf[0][i]    = cdf[0][i-1];       // move
  cdf[0][0] = 0.0;
    
#ifdef DEBUG
  TRY( fabs(cdf[M[0]+1]-1.0) < 1e-6 );
#endif
  return;
Error:
  abort();
}

//
// Encoder
//

static
void update(u32 s,real *b,real *l,size_t M, real *C)
{ real y;
   y = b[0]+l[0]*C[s+1]; // end
  *b = b[0]+l[0]*C[s];   // beg
  *l = y-b[0];           // length
}

static
void carry(stream_t *self)
{ size_t ibyte=self->ibyte;       // ibit = 0 is the high bit
  u8 *d = self->d;
  u8 s,c;
  ibyte -= (ibyte>0)&&(self->mask&1); // if ibyte has wrapped over, subtract one
  c=d[ibyte] + self->mask;       // carry the ibits
  s=c<d[ibyte];                  // overflow test
  d[ibyte]=c;
  if(s)                          // need to keep carrying
  { while(d[--ibyte]==255)
      d[ibyte]=0;                // compliment all bits
    d[ibyte] += 1;               // finish carrying
  }
}

static
void erenorm(real *b, real *l, stream_t *out)
{ while(*l<0.5)
  { *l*=2.0;
    if(*b>=0.5)
    { push(out,1);
      *b = 2.0*(*b-0.5);
    } else
    { push(out,0);
      *b *= 2.0;
    }
  }
}
                                
static 
void eselect(stream_t *out, real *b)
{ if(*b<=0.5)
    push(out,1);
  else
  { carry(out);
    push(out,0);
  }
}

void encode(u8 **out, size_t *nout, u32 *in, size_t nin, real *cdf, size_t nsym)
{ real   b=0.0, // beginning of the interval
         l=1.0; // length of the interval
  size_t i;
  state_t state;
  init(&state,out,nout,cdf,nsym);   // HERE HERE HERE HERE
  maybe_init(out);
  for(i=0;i<N;++i)
  { update(s[i],&b,&l,M,C);
    if(b>=1.0)
    { b-=1.0;
      carry(out);
    }
    if(l<=0.5)
      erenorm(&b,&l,out);
  }
  eselect(out,&b);
}

static
u32 dselect(real v, real *b, real *l, real *c, size_t nsym)
{ size_t s = nsym-1; // start from last symbol
  real x = *b + *l * c[s],
       y = *b + *l;
  while(x>v)
  { s--;
    y = x;
    x = *b + *l*c[s];
  }
  *b = x;
  *l = y-*b;
  return s;
}

static
void drenorm(real *v,real *b, real *l, stream_t *in)
{ while(*l<0.5)
  { if(*b>=0.5)
    { *b -=0.5;
      *v -=0.5;
    }
    *b *= 2.0;
    *v *= 2.0;
    *v += pop(in)/256.0;
    *l *= 2.0;
  }
  return;
}

void decode(u32 *out, size_t nout, u8 *in, size_t nin, real *c, size_t nsym)
{ real b=0.0,
       l=1.0,
       v;
  size_t i;
  stream_t bits = {0};
  bits.d = in;
  bits.nbytes = nin;
  for(i=0;i<8;++i)                    // TODO: could do this from table lookup
    v += pop(&bits)/(real)(1<<(i+1)); // read in first byte
  for(i=0;i<nout;++i)
  { out[i] = dselect(v,&b,&l,c,nsym);
    if(b>=1.0)
    { b-=1.0;
      v-=1.0;
    }
    if(l<=0.5)
      drenorm(&v,&b,&l,&bits);
  }
}
