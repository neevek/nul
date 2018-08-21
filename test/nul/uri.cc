#include <gtest/gtest.h>
#include "nul/uri.hpp"

using namespace nul;

TEST(URI, Test) {
  auto uri = URI{};
  ASSERT_TRUE(uri.parse("http://google.com"));
  ASSERT_STREQ("http", uri.getScheme().c_str());
  ASSERT_STREQ("google.com", uri.getHost().c_str());
  ASSERT_EQ(0, uri.getPort());

  uri = URI{};
  ASSERT_TRUE(uri.parse("https://www.google.com:443/hello/world?key=value#hash"));
  ASSERT_STREQ("https", uri.getScheme().c_str());
  ASSERT_STREQ("www.google.com", uri.getHost().c_str());
  ASSERT_EQ(443, uri.getPort());
  ASSERT_STREQ("/hello/world", uri.getPath().c_str());
  ASSERT_STREQ("key=value", uri.getQueryStr().c_str());
  ASSERT_STREQ("hash", uri.getFragment().c_str());

  uri = URI{};
  ASSERT_TRUE(uri.parse("https://user@www.google.com:443/hello/world?key=value&k2=v2#hash"));
  ASSERT_STREQ("user", uri.getUserInfo().c_str());
  ASSERT_STREQ("https", uri.getScheme().c_str());
  ASSERT_STREQ("www.google.com", uri.getHost().c_str());
  ASSERT_EQ(443, uri.getPort());
  ASSERT_STREQ("/hello/world", uri.getPath().c_str());
  ASSERT_STREQ("key=value&k2=v2", uri.getQueryStr().c_str());
  ASSERT_STREQ("hash", uri.getFragment().c_str());
  ASSERT_STREQ("user@www.google.com:443", uri.getAuthority().c_str());

  uri = URI{};
  ASSERT_TRUE(uri.parse("user@www.google.com:443/hello/world?key=value&k2=v2#hash"));
  ASSERT_STREQ("user", uri.getUserInfo().c_str());
  ASSERT_STREQ("", uri.getScheme().c_str());
  ASSERT_STREQ("www.google.com", uri.getHost().c_str());
  ASSERT_EQ(443, uri.getPort());
  ASSERT_STREQ("/hello/world", uri.getPath().c_str());
  ASSERT_STREQ("key=value&k2=v2", uri.getQueryStr().c_str());
  ASSERT_STREQ("hash", uri.getFragment().c_str());
  ASSERT_STREQ("user@www.google.com:443", uri.getAuthority().c_str());

  uri = URI{};
  ASSERT_TRUE(uri.parse("www.google.com:443/hello/world?key=value&k2=v2#hash"));
  ASSERT_STREQ("", uri.getUserInfo().c_str());
  ASSERT_STREQ("", uri.getScheme().c_str());
  ASSERT_STREQ("www.google.com", uri.getHost().c_str());
  ASSERT_EQ(443, uri.getPort());
  ASSERT_STREQ("/hello/world", uri.getPath().c_str());
  ASSERT_STREQ("key=value&k2=v2", uri.getQueryStr().c_str());
  ASSERT_STREQ("hash", uri.getFragment().c_str());
  ASSERT_STREQ("www.google.com:443", uri.getAuthority().c_str());

  uri = URI{};
  ASSERT_TRUE(uri.parse("https://user@www.google.com:443/hello/world?key=value#hash"));
  ASSERT_STREQ("user", uri.getUserInfo().c_str());
  ASSERT_STREQ("https", uri.getScheme().c_str());
  ASSERT_STREQ("www.google.com", uri.getHost().c_str());
  ASSERT_EQ(443, uri.getPort());
  ASSERT_STREQ("/hello/world", uri.getPath().c_str());
  ASSERT_STREQ("key=value", uri.getQueryStr().c_str());
  ASSERT_STREQ("hash", uri.getFragment().c_str());

  uri = URI{};
  ASSERT_TRUE(uri.parse("file:///path/to/file//haha.txt?key=value#hash"));
  ASSERT_STREQ("file", uri.getScheme().c_str());
  ASSERT_STREQ("", uri.getHost().c_str());
  ASSERT_EQ(0, uri.getPort());
  ASSERT_STREQ("/path/to/file//haha.txt", uri.getPath().c_str());
  ASSERT_STREQ("key=value", uri.getQueryStr().c_str());
  ASSERT_STREQ("hash", uri.getFragment().c_str());
}
