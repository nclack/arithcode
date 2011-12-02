#include <gtest/gtest.h>
#include "stream.h"

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
