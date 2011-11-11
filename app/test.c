#include "said.h"
#include <stdio.h>

#define countof(e) (sizeof(e)/sizeof(*(e)))

void pcode(u8 *out, size_t nout)
{ size_t i,j;
  printf("Code:\n");
  for(i=0;i<nout;++i)
  { for(j=0;j<4 && i<nout;j++,i++)
      printf(" %x",out[i]);
    --i;
    printf("\n");
  }
  printf("--\n");
}

void pmsg(u32 *v,size_t n)
{ size_t i,j;
  printf("Message:\n");
  for(i=0;i<n;++i)
  { for(j=0;j<4 && i<n;j++,i++)
      printf(" %d",v[i]);
    --i;
    printf("\n");
  }
  printf("--\n");
}

int main(int argc, char* argv[])
{ u32  s[] = {2,1,0,0,1,3},
       t[1024];
  real c[] = {0.0, 0.2, 0.7, 0.9, 1.0};
  u8    *out = NULL;
  size_t nout = 0;

  pmsg(s,countof(s));
  encode(&out,&nout,s,countof(s),c,countof(c)-1);
  pcode(out,nout);
  decode(t,countof(s),out,nout,c,countof(c)-1);
  pmsg(t,countof(s));

  free(out);
  return 0;
}

#if 0
int main(int argc,char* argv[])
{ u32 s[] = { 0,1,0,0,0 },
      t[] = { 0,0,0,0,0 };
  real *c=NULL;
  size_t nc;
  bits_t out = {0};
  
  cdf_build(&c,&nc,s,countof(s));
  encode(&out,s,countof(s),c,nc);

  pmsg(s,countof(s));
  pcode(&out);

  decode(t,countof(t),out.d,out.ibyte+1,c,nc);
  pmsg(t,countof(t));

  free(out.d);
  free(c);
  return 0;
}
#endif
