#include <gtest/gtest.h>
#include "nul/xbuffer.hpp"

using namespace nul;

static void assertBuffer(
  std::unique_ptr<Buffer> buf, const char *data, std::size_t len) {
  ASSERT_TRUE(!!buf);
  ASSERT_EQ(buf->getLength(), len);
  ASSERT_EQ(memcmp(buf->getData(), data, len), 0);
}

TEST(XBuffer, XBuffer) {
  XBuffer<2> xbuf;
  xbuf.offer("\x0\x5hello\x0\x0\x0\x5world", 16);
  ASSERT_EQ(xbuf.getBufferCount(), 3);

  assertBuffer(xbuf.take(), "hello", 5);
  assertBuffer(xbuf.take(), "", 0);
  assertBuffer(xbuf.take(), "world", 5);
  ASSERT_EQ(xbuf.getBufferCount(), 0);

  xbuf.offer("\x0", 1);
  ASSERT_EQ(xbuf.getBufferCount(), 0);
  xbuf.offer("\x5", 1);
  ASSERT_EQ(xbuf.getBufferCount(), 0);
  xbuf.offer("hell", 4);
  ASSERT_EQ(xbuf.getBufferCount(), 0);
  xbuf.offer("o\x0\x5world\x0\x0", 10);
  ASSERT_EQ(xbuf.getBufferCount(), 3);

  assertBuffer(xbuf.take(), "hello", 5);
  assertBuffer(xbuf.take(), "world", 5);
  assertBuffer(xbuf.take(), "", 0);
  ASSERT_EQ(xbuf.getBufferCount(), 0);

  constexpr auto sLen = 500;
  char s1[sLen] = {0};
  char s2[sLen] = {0};
  for (int i = 0; i < sLen; ++i) {
    s1[i] = i;
    s2[i] = i;
  }

  xbuf.offer("\x1\xf4", 2);
  xbuf.offer(s1, sLen);
  xbuf.offer("\x0\x0", 2);
  ASSERT_EQ(xbuf.getBufferCount(), 2);
  assertBuffer(xbuf.take(), s2, sLen);
  assertBuffer(xbuf.take(), "", 0);
  ASSERT_EQ(xbuf.getBufferCount(), 0);

}
