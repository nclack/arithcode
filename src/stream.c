#include "stream.h"

#include <stdint.h> // for fixed width integer types
#include <stdlib.h> // for size_t
#include <stdio.h>  // for exception handling
#include <string.h> // for memset

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef  int8_t   i8;
typedef  int16_t  i16;
typedef  int32_t  i32;
typedef  int64_t  i64;

#define ENDL "\n"
#define TRY(e) \
  do{ if(!(e)) {\
    printf("%s(%d):"ENDL "\t%s"ENDL "\tExpression evaluated as false."ENDL, \
        __FILE__,__LINE__,#e); \
    goto Error; \
  }} while(0)

#define SAFE_FREE(e) if(e) { free(e); (e)=NULL; }

static void maybe_init(stream_t *self)
{ if(!self->d)
  { memset(self,0,sizeof(*self));
    TRY(self->d=malloc(self->nbytes=4096));
    self->own=1;
  }
  return;
Error:
  abort();
}

void attach(stream_t *self, void* d, size_t n)
{ if(self->own) SAFE_FREE(self->d);
  self->d = (u8*)d;
  self->own = 0;
  self->nbytes = n;
  maybe_init(self);
}

void detach(stream_t *self, void **d, size_t *n)
{ if(d) *d = self->d;
  if(n) *n = self->ibyte;
  memset(self,0,sizeof(*self));
}

static void maybe_resize(stream_t *s)
{ if(s->ibyte>=s->nbytes)
    TRY(s->d = realloc(s->d,s->nbytes=(1.2*s->ibyte+50)));
  return;
Error:
  abort();
}

//printf("PUSH: [%llu] %llu %llu"ENDL,(u64)self->ibyte,(u64)v,(u64)(*(T*)(self->d+self->ibyte))); 

#define DEFN_PUSH(T) \
  void push_##T(stream_t *self, T v) \
  { *(T*)(self->d+self->ibyte) = v;  \
    self->ibyte+=sizeof(T);          \
    maybe_resize(self);              \
  }
DEFN_PUSH(u8);
DEFN_PUSH(u16);
DEFN_PUSH(u32);
DEFN_PUSH(u64);
DEFN_PUSH(i8);
DEFN_PUSH(i16);
DEFN_PUSH(i32);
DEFN_PUSH(i64);

void push_u1(stream_t *self, u8 v)
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
    maybe_resize(self);
    self->d[self->ibyte]=0;         // clear out the byte    
  } else 
  { self->ibit++;
  }
}
void push_u4(stream_t *self, u8 v)
{ 
  self->mask = (self->ibit==0)?0xf0:0x0f ; // index from the high bit so we can use integer addition to do carries
  //set
  u8 *w = self->d+self->ibyte,
      m = self->mask;
  *w ^= (*w^ (v<<(4-self->ibit)) )&m;
  //inc
  if(self->ibit==4)
  { self->ibit=0;
    self->ibyte++;
    maybe_resize(self);
    self->d[self->ibyte]=0;         // clear out the byte    
  } else 
  { self->ibit=4;
  }
}

#define DEFN_POP(T) \
  T pop_##T(stream_t *self) \
  { T v; \
    if(self->ibyte>=self->nbytes) return 0; \
    v = *(T*)(self->d+self->ibyte); \
    self->ibyte+=sizeof(T); \
    return v; \
  }
DEFN_POP(u8);
DEFN_POP(u16);
DEFN_POP(u32);
DEFN_POP(u64);
DEFN_POP(i8);
DEFN_POP(i16);
DEFN_POP(i32);
DEFN_POP(i64);

u8 pop_u1(stream_t *self)
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
u8 pop_u4(stream_t *self)
{ u8 v,m;
  m = (self->ibit==0)?0xf0:0x0f; 
  v = 0;
  if(self->ibyte<self->nbytes)
    v = (self->d[self->ibyte] & m)>>(4-self->ibit);
  //inc
  if(self->ibit=4)
  { self->ibit=0;
    self->ibyte++;
  } else
    self->ibit=4;
  return v;
}

#define MAX_TYPE(T) static const T max_##T = (T)(-1)
MAX_TYPE(u8);
MAX_TYPE(u16);
MAX_TYPE(u32);
MAX_TYPE(u64);

#define DEFN_CARRY(T) \
void carry_##T(stream_t *self)       \
{ size_t n = self->ibyte-1;          \
  while( *(T*)(self->d+n)==max_##T ) \
    *(T*)(self->d+n--)=0;            \
  (*(T*)(self->d+n))++;              \
}
DEFN_CARRY(u8);
DEFN_CARRY(u16);
DEFN_CARRY(u32);
DEFN_CARRY(u64);

void carry_u1(stream_t *self)
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
void carry_u4(stream_t *self)
{ size_t ibyte=self->ibyte;      // ibit = 0 is the high bit
  u8 *d = self->d;
  u8 s,c;
  u8 m = (self->ibit==0)?0x01:0x10; //if 0, last write was 0x0f, otherwise at 0xf0
  ibyte -= (ibyte>0)&&(self->mask==0x0f); // if ibyte has wrapped over, subtract one
  c=d[ibyte] + m;                // carry
  s=c<d[ibyte];                  // overflow test
  d[ibyte]=c;
  if(s)                          // need to keep carrying
  { while(d[--ibyte]==255)
      d[ibyte]=0;                // compliment all bits
    d[ibyte] += 1;               // finish carrying
  }
}
