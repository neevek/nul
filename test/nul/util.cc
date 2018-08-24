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

  ASSERT_EQ(0, StringUtil::split(":::", ":").size());
  ASSERT_EQ(1, StringUtil::split("123:::", ":").size());
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

TEST(NetUtil, isIPv4) {
  ASSERT_TRUE(NetUtil::isIPv4("128.1.0.1"));
  ASSERT_TRUE(NetUtil::isIPv4("0.0.0.0"));
  ASSERT_TRUE(NetUtil::isIPv4("10.0.0.1"));
  ASSERT_TRUE(NetUtil::isIPv4("0.120.0.1"));
  ASSERT_TRUE(NetUtil::isIPv4("0.250.00000.1"));
  ASSERT_TRUE(NetUtil::isIPv4("223.255.254.254"));
  ASSERT_FALSE(NetUtil::isIPv4("999.12345.0.0001"));
  ASSERT_FALSE(NetUtil::isIPv4("1.2.0.331"));
  ASSERT_FALSE(NetUtil::isIPv4("12.0.331"));
  ASSERT_FALSE(NetUtil::isIPv4("12.12.1."));
  ASSERT_FALSE(NetUtil::isIPv4(".12.12.1"));
}

TEST(NetUtil, isIPv6) {
  ASSERT_TRUE(NetUtil::isIPv6("1050:0:0:0:5:600:300c:326b"));
  ASSERT_FALSE(NetUtil::isIPv6("1050!0!0+0-5@600$300c#326b"));
  ASSERT_FALSE(NetUtil::isIPv6("1050:0:0:0:5:600:300c:326babcdef"));
  ASSERT_FALSE(NetUtil::isIPv6("1050:::600:5:1000::"));
  ASSERT_TRUE(NetUtil::isIPv6("fe80::202:b3ff:fe1e:8329"));
  ASSERT_FALSE(NetUtil::isIPv6("fe80::202:b3ff::fe1e:8329"));
  ASSERT_FALSE(NetUtil::isIPv6("fe80:0000:0000:0000:0202:b3ff:fe1e:8329:abcd"));
  ASSERT_TRUE(NetUtil::isIPv6("::1"));
  ASSERT_TRUE(NetUtil::isIPv6("1::"));
  ASSERT_TRUE(NetUtil::isIPv6("1:f3::"));
  ASSERT_TRUE(NetUtil::isIPv6("::1:f3"));
  ASSERT_TRUE(NetUtil::isIPv6("::"));
  ASSERT_FALSE(NetUtil::isIPv6(":"));
  ASSERT_TRUE(NetUtil::isIPv6("1:feee:0:0:0:0:0:1"));
  ASSERT_TRUE(NetUtil::isIPv6("1:feee::1"));
}

TEST(NetUtil, expandIPv6) {
  ASSERT_STREQ(
    "1050:0000:0000:0000:0005:0600:300c:326b",
    NetUtil::expandIPv6("1050:0:0:0:5:600:300c:326b").c_str());
  ASSERT_STREQ(
    "1050:0000:0000:0000:0005:0600:300c:326b",
    NetUtil::expandIPv6("1050:0000:0000:0000:0005:0600:300c:326b").c_str());
  ASSERT_STREQ(
    "fe80:0000:0000:0000:0202:b3ff:fe1e:8329",
    NetUtil::expandIPv6("fe80::202:b3ff:fe1e:8329").c_str());
  ASSERT_STREQ(
    "0000:0000:0000:0000:0000:0000:0000:0001",
    NetUtil::expandIPv6("::1").c_str());
  ASSERT_STREQ(
    "0001:0000:0000:0000:0000:0000:0000:0000",
    NetUtil::expandIPv6("1::").c_str());
  ASSERT_STREQ(
    "0001:00f3:0000:0000:0000:0000:0000:0000",
    NetUtil::expandIPv6("1:f3::").c_str());
  ASSERT_STREQ(
    "0000:0000:0000:0000:0000:0000:0001:00f3",
    NetUtil::expandIPv6("::1:f3").c_str());
  ASSERT_STREQ(
    "0000:0000:0000:0000:0000:0001:0001:00f3",
    NetUtil::expandIPv6("::1:1:f3").c_str());
  ASSERT_STREQ(
    "0000:0000:0000:0000:2345:0001:0001:00f3",
    NetUtil::expandIPv6("::2345:1:1:f3").c_str());
  ASSERT_STREQ(
    "0000:0000:0000:0333:2345:0001:0001:00f3",
    NetUtil::expandIPv6("::333:2345:1:1:f3").c_str());
  ASSERT_STREQ(
    "0000:0001:0000:0333:2345:0001:0001:00f3",
    NetUtil::expandIPv6("::1:0:333:2345:1:1:f3").c_str());
  ASSERT_STREQ(
    "1000:0001:0000:0333:2345:0001:0001:00f3",
    NetUtil::expandIPv6("1000:1:0:333:2345:1:1:f3").c_str());
  ASSERT_STREQ(
    "0000:0000:0000:0000:0000:0000:0000:0000",
    NetUtil::expandIPv6("::").c_str());
  ASSERT_STREQ(
    "0001:0000:0000:0000:0000:0000:0000:0001",
    NetUtil::expandIPv6("1::1").c_str());
  ASSERT_STREQ(
    "0001:feee:0000:0000:0000:0000:0000:0001",
    NetUtil::expandIPv6("1:feee:0:0:0:0:0:1").c_str());
  ASSERT_STREQ(
    "0001:feee:0000:0000:0000:0000:0000:0001",
    NetUtil::expandIPv6("1:feee::1").c_str());
}
