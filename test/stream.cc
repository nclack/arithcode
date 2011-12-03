#include <gtest/gtest.h>
#include "stream.h"

///// PREP

// templated calls to the stream api
// - won't work for u1 and u4 types, but these usually require
//   special tests anyway
template<class T> void push(stream_t *s, T v);
#define DEFN_PUSH(a,b) \
  template<> void push<a>(stream_t *s, a v) { push_##b(s,v); }
DEFN_PUSH(uint8_t ,u8);
DEFN_PUSH(uint16_t,u16);
DEFN_PUSH(uint32_t,u32);
DEFN_PUSH(uint64_t,u64);
DEFN_PUSH( int8_t ,i8);
DEFN_PUSH( int16_t,i16);
DEFN_PUSH( int32_t,i32);
DEFN_PUSH( int64_t,i64);

template<class T> T pop(stream_t *s);
#define DEFN_POP(a,b) \
  template<> a pop(stream_t *s) { return pop_##b(s); }
DEFN_POP(uint8_t ,u8);
DEFN_POP(uint16_t,u16);
DEFN_POP(uint32_t,u32);
DEFN_POP(uint64_t,u64);
DEFN_POP( int8_t ,i8);
DEFN_POP( int16_t,i16);
DEFN_POP( int32_t,i32);
DEFN_POP( int64_t,i64);

template<class T> void carry(stream_t *s);
#define DEFN_CARRY(a,b) \
  template<> void carry<a> (stream_t *s) { carry_##b(s); }
DEFN_CARRY(uint8_t ,u8);
DEFN_CARRY(uint16_t,u16);
DEFN_CARRY(uint32_t,u32);

///// Attach and Detach

TEST(Attach,NullBuffer)
{ stream_t d={0};
  attach(&d,NULL,0);
  EXPECT_TRUE(d.d!=NULL);
  EXPECT_GT(d.nbytes,0);
  EXPECT_EQ(0,d.ibyte);
  EXPECT_EQ(0,d.ibit);
  //clean up
  { ASSERT_TRUE(d.d!=NULL);
    void *buf;
    size_t n;
    detach(&d,&buf,&n);
    EXPECT_EQ(0,n); // nothing pushed so zero length
    ASSERT_TRUE(buf!=NULL);
    free(buf);
  }
}

TEST(Attach,ExistingBuffer)
{ stream_t d={0};
  float buf[1024];
  size_t n = sizeof(buf)/sizeof(*buf);
  attach(&d,buf,n);
  EXPECT_EQ((void*)buf,(void*)d.d);
  EXPECT_EQ(n,d.nbytes);
  EXPECT_EQ(0,d.ibyte);
  EXPECT_EQ(0,d.ibit);
}

///// Basic Framework used below

class StreamSanityTest : public ::testing::Test 
{ public:
    inline stream_t *s() {return &stream_;}
  protected:
    virtual void SetUp()
    { memset(&stream_,0,sizeof(stream_)); // important to zero things out
      attach(&stream_,NULL,0);
      memset(stream_.d,0,stream_.nbytes); // attach doesn't zero (nor should it) 
    }
    virtual void TearDown()
    { void *buf;
      size_t n;
      detach(&stream_,&buf,&n);
      if(buf) free(buf);
    }
  stream_t stream_;
};

///// Push's and stream counters

TEST_F(StreamSanityTest,PushU1)
{ 
  push_u1(s(),1);
  push_u1(s(),0);
  push_u1(s(),0);
  push_u1(s(),1);
  EXPECT_EQ(0,s()->ibyte);
  EXPECT_EQ(4,s()->ibit);
  EXPECT_EQ(0x90,s()->d[0]);

  push_u1(s(),0);
  push_u1(s(),1);
  push_u1(s(),0);
  push_u1(s(),1);
  EXPECT_EQ(1,s()->ibyte);
  EXPECT_EQ(0,s()->ibit);
  EXPECT_EQ(0x95,s()->d[0]);
}
TEST_F(StreamSanityTest,PushU4)
{ 
  push_u4(s(),9);
  EXPECT_EQ(0,s()->ibyte);
  EXPECT_EQ(4,s()->ibit);
  EXPECT_EQ(0x90,s()->d[0]);

  push_u4(s(),5);
  EXPECT_EQ(1,s()->ibyte);
  EXPECT_EQ(0,s()->ibit);
  EXPECT_EQ(0x95,s()->d[0]);
}
TEST_F(StreamSanityTest,PushU8)
{ 
  push_u8(s(),0x95);
  EXPECT_EQ(1,s()->ibyte);
  EXPECT_EQ(0,s()->ibit);
  push_u8(s(),0x59);
  EXPECT_EQ(2,s()->ibyte);
  EXPECT_EQ(0,s()->ibit);
  EXPECT_EQ(0x95,s()->d[0]);
  EXPECT_EQ(0x59,s()->d[1]);
}

///// Basic Push and Pop sequences (FIFO)

typedef testing::Types<
  uint8_t,
  uint16_t,
  uint32_t,
  uint64_t,
  int8_t,
  int16_t,
  int32_t,
  int64_t
  > RegularTypes;

template<class T> class TypedStreamSanityTest : public StreamSanityTest {};

TYPED_TEST_CASE(TypedStreamSanityTest,RegularTypes);

TYPED_TEST(TypedStreamSanityTest,PushPop)
{ stream_t *t = StreamSanityTest::s();
  push<TypeParam>(t,1);
  push<TypeParam>(t,2);
  push<TypeParam>(t,3);
  push<TypeParam>(t,4);
  t->ibyte = 0;
  EXPECT_EQ(1,pop<TypeParam>(t));
  EXPECT_EQ(2,pop<TypeParam>(t));
  EXPECT_EQ(3,pop<TypeParam>(t));
  EXPECT_EQ(4,pop<TypeParam>(t));
  EXPECT_EQ(0,pop<TypeParam>(t)); // should return 0's on overflow
  EXPECT_EQ(0,pop<TypeParam>(t));
}
TEST_F(StreamSanityTest,PushPopU1)
{ stream_t *t = s();
  push_u1(t,1);
  push_u1(t,0);
  push_u1(t,1);
  push_u1(t,1);
  t->ibyte = 0;
  t->ibit = 0;
  EXPECT_EQ(1,pop_u1(t));
  EXPECT_EQ(0,pop_u1(t));
  EXPECT_EQ(1,pop_u1(t));
  EXPECT_EQ(1,pop_u1(t));
  EXPECT_EQ(0,pop_u1(t)); // should return 0's on overflow
  EXPECT_EQ(0,pop_u1(t));
}
TEST_F(StreamSanityTest,PushPopU4)
{ stream_t *t = s();
  push_u4(t,1);
  push_u4(t,2);
  push_u4(t,3);
  push_u4(t,4);
  EXPECT_EQ(2,t->ibyte);
  EXPECT_EQ(0,t->ibit);
  t->ibyte = 0;
  t->ibit = 0;
  EXPECT_EQ(1,pop_u4(t));
  EXPECT_EQ(2,pop_u4(t));
  EXPECT_EQ(3,pop_u4(t));
  EXPECT_EQ(4,pop_u4(t));
  EXPECT_EQ(0,pop_u4(t)); // should return 0's on overflow
  EXPECT_EQ(0,pop_u4(t));
}

///// The Carry Op

typedef testing::Types<
  uint8_t,
  uint16_t,
  uint32_t
  > CarryTypes;

template<class T> class TypedStreamCarryTest : public StreamSanityTest {};
TYPED_TEST_CASE(TypedStreamCarryTest,CarryTypes);
TYPED_TEST(TypedStreamCarryTest,Carry)
{ stream_t *t = StreamSanityTest::s();
  push<TypeParam>(t,2);
  push<TypeParam>(t,(TypeParam)-1);
  push<TypeParam>(t,(TypeParam)-1);
  push<TypeParam>(t,(TypeParam)-1);
  push<TypeParam>(t,(TypeParam)-1);
  carry<TypeParam>(t);
  push<TypeParam>(t,2);
  t->ibyte = 0;
  EXPECT_EQ(3,pop<TypeParam>(t));
  EXPECT_EQ(0,pop<TypeParam>(t));
  EXPECT_EQ(0,pop<TypeParam>(t));
  EXPECT_EQ(0,pop<TypeParam>(t));
  EXPECT_EQ(0,pop<TypeParam>(t)); 
  EXPECT_EQ(2,pop<TypeParam>(t)); 
  EXPECT_EQ(0,pop<TypeParam>(t)); // should return 0's on overflow
}
 
TEST_F(StreamSanityTest,CarryU1)
{ stream_t *t = s();
  push_u1(t,0);
  push_u1(t,1);
  push_u1(t,1);
  push_u1(t,1);
  push_u1(t,1);
  carry_u1(t);
  push_u1(t,1);
  t->ibyte = 0;
  t->ibit  = 0;
  EXPECT_EQ(1,pop_u1(t));
  EXPECT_EQ(0,pop_u1(t));
  EXPECT_EQ(0,pop_u1(t));
  EXPECT_EQ(0,pop_u1(t));
  EXPECT_EQ(0,pop_u1(t)); 
  EXPECT_EQ(1,pop_u1(t)); 
  EXPECT_EQ(0,pop_u1(t)); // should return 0's on overflow
}
 
TEST_F(StreamSanityTest,CarryU4)
{ stream_t *t = s();
  push_u4(t,2);
  push_u4(t,(uint8_t)-1);
  push_u4(t,(uint8_t)-1);
  push_u4(t,(uint8_t)-1);
  push_u4(t,(uint8_t)-1);
  carry_u4(t);
  push_u4(t,2);
  t->ibyte = 0;
  t->ibit  = 0;
  EXPECT_EQ(3,pop_u4(t));
  EXPECT_EQ(0,pop_u4(t));
  EXPECT_EQ(0,pop_u4(t));
  EXPECT_EQ(0,pop_u4(t));
  EXPECT_EQ(0,pop_u4(t)); 
  EXPECT_EQ(2,pop_u4(t)); 
  EXPECT_EQ(0,pop_u4(t)); // should return 0's on overflow
}
