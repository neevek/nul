#include <gtest/gtest.h>
#include "nul/util.hpp"

using namespace nul;

TEST(StringUtil, split) {
  StringUtil::split("\r\n", "\r\n", [](auto index, const auto &part){
    FAIL() << "not possible";
  });

  StringUtil::split("", "\r\n", [](auto index, const auto &part){
    FAIL() << "not possible";
  });

  StringUtil::split("hello", "\r\n", [](auto index, const auto &part){
    ASSERT_STREQ("hello", part.c_str());
  });

  StringUtil::split("\r\nhello", "\r\n", [](auto index, const auto &part){
    if (index == 0) ASSERT_STREQ("hello", part.c_str());
    else FAIL() << "not possible";
  });

  StringUtil::split("hello:string:util:", ":", [](auto index, const auto &part){
    if (index == 0) ASSERT_STREQ("hello", part.c_str());
    else if (index == 1) ASSERT_STREQ("string", part.c_str());
    else if (index == 2) ASSERT_STREQ("util", part.c_str());
    else FAIL() << "not possible";
  });

  StringUtil::split("CONNECT 127.0.0.1:443 HTTP/1.1\r\n"
                    "Host: 127.0.0.1:443\r\n"
                    "User-Agent: curl/7.43.0\r\n"
                    "Proxy-Connection: Keep-Alive\r\n\r\n",
                    "\r\n", [](auto index, const auto &part){

    if (index == 0) ASSERT_STREQ("CONNECT 127.0.0.1:443 HTTP/1.1", part.c_str());
    else if (index == 1) ASSERT_STREQ("Host: 127.0.0.1:443", part.c_str());
    else if (index == 2) ASSERT_STREQ("User-Agent: curl/7.43.0", part.c_str());
    else if (index == 3) ASSERT_STREQ("Proxy-Connection: Keep-Alive", part.c_str());
    else FAIL() << "not possible";
  });
}

TEST(StringUtil, tolwer) {
  auto s = std::string{"Hello World"};
  ASSERT_STREQ("hello world", StringUtil::tolower(s).c_str());
}
