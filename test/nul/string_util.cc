#include <gtest/gtest.h>
#include "nul/util.hpp"

using namespace nul;

TEST(StringUtil, split) {
  StringUtil::split("\r\n", "\r\n", [](auto index, const auto &part) -> bool {
    EXPECT_TRUE(false) << "not possible";
    return false;
  });

  StringUtil::split("", "\r\n", [](auto index, const auto &part){
    EXPECT_TRUE(false) << "not possible";
    return false;
  });

  StringUtil::split("hello", "\r\n", [](auto index, const auto &part){
    EXPECT_STREQ("hello", part.c_str());
    return true;
  });

  StringUtil::split("\r\nhello", "\r\n", [](auto index, const auto &part){
    if (index == 0) EXPECT_STREQ("hello", part.c_str());
    else EXPECT_TRUE(false) << "not possible";
    return true;
  });

  StringUtil::split("hello:string:util:", ":", [](auto index, const auto &part){
    if (index == 0) EXPECT_STREQ("hello", part.c_str());
    else if (index == 1) EXPECT_STREQ("string", part.c_str());
    else if (index == 2) EXPECT_STREQ("util", part.c_str());
    else EXPECT_TRUE(false) << "not possible";
    return true;
  });

  StringUtil::split("CONNECT 127.0.0.1:443 HTTP/1.1\r\n"
                    "Host: 127.0.0.1:443\r\n"
                    "User-Agent: curl/7.43.0\r\n"
                    "Proxy-Connection: Keep-Alive\r\n\r\n",
                    "\r\n", [](auto index, const auto &part){
    if (index == 0) EXPECT_STREQ("CONNECT 127.0.0.1:443 HTTP/1.1", part.c_str());
    else if (index == 1) EXPECT_STREQ("Host: 127.0.0.1:443", part.c_str());
    else if (index == 2) EXPECT_STREQ("User-Agent: curl/7.43.0", part.c_str());
    else if (index == 3) EXPECT_STREQ("Proxy-Connection: Keep-Alive", part.c_str());
    else EXPECT_TRUE(false) << "not possible";
    return true;
  });
}

TEST(StringUtil, tolwer) {
  auto s = std::string{"Hello World"};
  EXPECT_STREQ("hello world", StringUtil::tolower(s).c_str());
}

TEST(StringUtil, trim) {
  ASSERT_STREQ("hello world", StringUtil::trim("\n hello world").c_str());
  ASSERT_STREQ("hello world", StringUtil::trim(" \thello world").c_str());
  ASSERT_STREQ("hello world", StringUtil::trim("\t\t hello world").c_str());
  ASSERT_STREQ("hello world", StringUtil::trim("hello world \n").c_str());
  ASSERT_STREQ("hello world", StringUtil::trim("hello world\t ").c_str());
  ASSERT_STREQ("hello world", StringUtil::trim("hello world \t\t").c_str());
  ASSERT_STREQ("hello world", StringUtil::trim("\t\t hello world \t\t").c_str());
  ASSERT_STREQ("", StringUtil::trim("").c_str());
  ASSERT_STREQ("", StringUtil::trim("  \t").c_str());
  ASSERT_STREQ("", StringUtil::trim(" ").c_str());
  ASSERT_STREQ("", StringUtil::trim("\r\n \r\n").c_str());
}
