/*
 * Introduction
 * ------------
 *
 * Implementation after Amir Said's Algorithm 22-29[1].
 *
 * The idea here is to get a more precise idea of what it takes to implement an
 * arithmetic coder, and to get a reference implementation.
 *
 * Following Amir's notation, 
 *
 *  D is the number of symbols in the output alphabet.  For simplicity, I'll
 *    start with D=2 (binary).
 *  P is the number of output symbols in the active block (see Eq 2.3 in [1]).
 *
 * Notes
 * -----
 *
 * - how to replace reals with integers?
 *   - consider sum(2^-n * bit(n)), but need values to be able to exceed [0,1]
 *     interval
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
typedef float     real;

#define ENDL "\n"
#define TRY(e) \
  do{ if(!(e)) {\
    printf("%s(%d):"ENDL "%s"ENDL "Expression evaluated as false."ENDL, \
        __FILE__,__LINE__,#e); \
    goto Error; \
  }} while(0)

typedef struct _bits_t
{ size_t ibyte;  //current byte
  size_t ibit;   //current bit - addresses the next write - FIXME?: don't actually need this, just use mask instead
  size_t nbytes; //capacity
  u8     mask;   //bit is set in the position of the last write
  u8 *d;
} bits_t;

static
void maybe_init(bits_t *self)
{ if(!self->d)
  { memset(self,0,sizeof(*self));
    TRY(self->d=malloc(self->nbytes=4096));
  }
  return;
Error:
  abort();
}

static
void maybe_resize(u8 **d,size_t req,size_t *cap)
{ if(req>*cap)
    TRY(*d = realloc(*d,*cap=(1.2*req+50)));
  return;
Error:
  abort();
}

static
void push(bits_t *self, u8 v)
{ 
  self->mask = 1<<(7-self->ibit); // index from the high bit so we can use integer addition to do carries
  //set
  u8 *w = self->d+self->ibyte,
      m = self->mask;
  *w = (*w & ~m) | (-(v) & m);
  //inc
  if(self->ibit==7)
  { self->ibit=0;
    self->ibyte++;
    maybe_resize(&self->d,self->ibyte+1,&self->nbytes);
    self->d[self->ibyte]=0;         // clear out the byte    
  } else 
  { self->ibit++;
  }
}

static u8 pop(bits_t *self)
{ u8 v,m;
  m = 1<<(7-self->ibit); 
  v = 0;
  if(self->ibyte<self->nbytes)
    v = (self->d[self->ibyte] & m)==m;
  //inc
  self->ibit++;
  if(self->ibit>7)
  { self->ibit=0;
    self->ibyte++;
  }
  return v;
}

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

/*
 * d: output    \____ These are contained in bits_t
 * t: bit index /
 * N: length of input
 * S: input
 * M: number of symbols
 * C: adjusted cumulative symbol probabilities
 *    c(s) = cumsum(p(s))
 *    C should be length M+1 where c[M]=1
 *
 * cap is the allocated capacity of d in bytes.
 *
 * Returns length of d in symbols.
 */


static
void update(u32 s,real *b,real *l,size_t M, real *C)
{ real y;
   y = b[0]+l[0]*C[s+1]; // end
  *b = b[0]+l[0]*C[s];   // beg
  *l = y-b[0];           // length
}

static
void carry(bits_t *self)
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
void erenorm(real *b, real *l, bits_t *out)
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
void eselect(bits_t *out, real *b)
{ if(*b<=0.5)
    push(out,1);
  else
  { carry(out);
    push(out,0);
  }
}

void encode(bits_t *out, u32 *s, size_t N,real *C, size_t M)
{ real   b=0.0, // beginning of the interval
         l=1.0; // length of the interval
  size_t i;
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
void drenorm(real *v,real *b, real *l, bits_t *in)
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
  bits_t bits = {0};
  bits.d = in;
  bits.nbytes = nin;
  for(i=0;i<8;++i)
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
