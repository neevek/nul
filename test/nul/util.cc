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

TEST(NetUtil, ipv4ToBinary) {
  uint8_t ipv4[4] = {0};
  ASSERT_TRUE(NetUtil::ipv4ToBinary("123.123.123.244", ipv4));
  ASSERT_TRUE(ipv4[0] == 123 && ipv4[1] == 123 && ipv4[2] == 123 && ipv4[3] == 244);
  ASSERT_TRUE(NetUtil::ipv4ToBinary("1.2.3.4", ipv4));
  ASSERT_TRUE(ipv4[0] == 1 && ipv4[1] == 2 && ipv4[2] == 3 && ipv4[3] == 4);
  ASSERT_TRUE(NetUtil::ipv4ToBinary("1.22.33.255", ipv4));
  ASSERT_TRUE(ipv4[0] == 1 && ipv4[1] == 22 && ipv4[2] == 33 && ipv4[3] == 255);
}

TEST(NetUtil, ipv6ToBinary) {
  uint8_t ipv6[16] = {0};
  ASSERT_TRUE(NetUtil::ipv6ToBinary("::", ipv6));
  ASSERT_TRUE(ipv6[0] == 0 && ipv6[1] == 0 && ipv6[2] == 0 && ipv6[3] == 0 &&
              ipv6[4] == 0 && ipv6[5] == 0 && ipv6[6] == 0 && ipv6[7] == 0 &&
              ipv6[8] == 0 && ipv6[9] == 0 && ipv6[10] == 0 && ipv6[11] == 0 &&
              ipv6[12] == 0 && ipv6[13] == 0 && ipv6[14] == 0 && ipv6[15] == 0);
  ASSERT_TRUE(NetUtil::ipv6ToBinary("::1", ipv6));
  ASSERT_TRUE(ipv6[0] == 0 && ipv6[1] == 0 && ipv6[2] == 0 && ipv6[3] == 0 &&
              ipv6[4] == 0 && ipv6[5] == 0 && ipv6[6] == 0 && ipv6[7] == 0 &&
              ipv6[8] == 0 && ipv6[9] == 0 && ipv6[10] == 0 && ipv6[11] == 0 &&
              ipv6[12] == 0 && ipv6[13] == 0 && ipv6[14] == 0 && ipv6[15] == 1);
  ASSERT_TRUE(NetUtil::ipv6ToBinary("1::", ipv6));
  ASSERT_TRUE(ipv6[0] == 0 && ipv6[1] == 1 && ipv6[2] == 0 && ipv6[3] == 0 &&
              ipv6[4] == 0 && ipv6[5] == 0 && ipv6[6] == 0 && ipv6[7] == 0 &&
              ipv6[8] == 0 && ipv6[9] == 0 && ipv6[10] == 0 && ipv6[11] == 0 &&
              ipv6[12] == 0 && ipv6[13] == 0 && ipv6[14] == 0 && ipv6[15] == 0);
  ASSERT_TRUE(NetUtil::ipv6ToBinary("1:feff::", ipv6));
  ASSERT_TRUE(ipv6[0] == 0 && ipv6[1] == 1 && ipv6[2] == 0xfe && ipv6[3] == 0xff &&
              ipv6[4] == 0 && ipv6[5] == 0 && ipv6[6] == 0 && ipv6[7] == 0 &&
              ipv6[8] == 0 && ipv6[9] == 0 && ipv6[10] == 0 && ipv6[11] == 0 &&
              ipv6[12] == 0 && ipv6[13] == 0 && ipv6[14] == 0 && ipv6[15] == 0);
  ASSERT_TRUE(NetUtil::ipv6ToBinary("1:feff::1234", ipv6));
  ASSERT_TRUE(ipv6[0] == 0 && ipv6[1] == 1 && ipv6[2] == 0xfe && ipv6[3] == 0xff &&
              ipv6[4] == 0 && ipv6[5] == 0 && ipv6[6] == 0 && ipv6[7] == 0 &&
              ipv6[8] == 0 && ipv6[9] == 0 && ipv6[10] == 0 && ipv6[11] == 0 &&
              ipv6[12] == 0 && ipv6[13] == 0 && ipv6[14] == 0x12 && ipv6[15] == 0x34);
  ASSERT_TRUE(NetUtil::ipv6ToBinary("1:feff:2:45:53:66:ae:1234", ipv6));
  ASSERT_TRUE(ipv6[0] == 0 && ipv6[1] == 1 && ipv6[2] == 0xfe && ipv6[3] == 0xff &&
              ipv6[4] == 0 && ipv6[5] == 2 && ipv6[6] == 0 && ipv6[7] == 0x45 &&
              ipv6[8] == 0 && ipv6[9] == 0x53 && ipv6[10] == 0 && ipv6[11] == 0x66 &&
              ipv6[12] == 0 && ipv6[13] == 0xae && ipv6[14] == 0x12 && ipv6[15] == 0x34);
  ASSERT_TRUE(NetUtil::ipv6ToBinary("1113:feff:acce:3399:aacc:33ff:1100:8899", ipv6));
  ASSERT_TRUE(ipv6[0] == 0x11 && ipv6[1] == 0x13 && ipv6[2] == 0xfe && ipv6[3] == 0xff &&
              ipv6[4] == 0xac && ipv6[5] == 0xce && ipv6[6] == 0x33 && ipv6[7] == 0x99 &&
              ipv6[8] == 0xaa && ipv6[9] == 0xcc && ipv6[10] == 0x33 && ipv6[11] == 0xff &&
              ipv6[12] == 0x11 && ipv6[13] == 0 && ipv6[14] == 0x88 && ipv6[15] == 0x99);
}

TEST(NetUtil, ipv4IsSameSubnet) {
  uint8_t subnet[4] = {0};
  ASSERT_TRUE(NetUtil::ipv4ToBinary("192.168.1.100", subnet));

  uint8_t ipv4[4] = {0};
  ASSERT_TRUE(NetUtil::ipv4ToBinary("192.168.2.120", ipv4));
  ASSERT_TRUE(NetUtil::ipv4IsSameSubnet(ipv4, subnet, 16));
  ASSERT_FALSE(NetUtil::ipv4IsSameSubnet(ipv4, subnet, 24));
  ASSERT_TRUE(NetUtil::ipv4IsSameSubnet(ipv4, subnet, 22));

  ASSERT_TRUE(NetUtil::ipv4ToBinary("172.200.100.2", subnet));
  ASSERT_TRUE(NetUtil::ipv4ToBinary("172.100.100.1", ipv4));
  ASSERT_TRUE(NetUtil::ipv4IsSameSubnet(ipv4, subnet, 8));
}

TEST(NetUtil, ipv6IsSameSubnet) {
  uint8_t subnet[16] = {0};
  ASSERT_TRUE(NetUtil::ipv6ToBinary("fe7f:41::", subnet));

  uint8_t ipv6[16] = {0};
  ASSERT_TRUE(NetUtil::ipv6ToBinary("fe7f:41:3:55::", ipv6));
  ASSERT_TRUE(NetUtil::ipv6IsSameSubnet(ipv6, subnet, 16));

  ASSERT_TRUE(NetUtil::ipv6ToBinary("fe7f:42:3:55::", ipv6));
  ASSERT_TRUE(NetUtil::ipv6IsSameSubnet(ipv6, subnet, 16));
  ASSERT_TRUE(NetUtil::ipv6IsSameSubnet(ipv6, subnet, 20));
  ASSERT_TRUE(NetUtil::ipv6IsSameSubnet(ipv6, subnet, 24));
  ASSERT_TRUE(NetUtil::ipv6IsSameSubnet(ipv6, subnet, 30));
  ASSERT_FALSE(NetUtil::ipv6IsSameSubnet(ipv6, subnet, 31));
  ASSERT_FALSE(NetUtil::ipv6IsSameSubnet(ipv6, subnet, 32));
}
