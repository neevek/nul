/*******************************************************************************
**          File: uri.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-21 Tue 04:39 PM
**   Description: URI
*******************************************************************************/
#ifndef NUL_URI_H_
#define NUL_URI_H_
#include <string>
#include "log.hpp"

namespace nul {
  class URI final {
    public:
      bool parse(const std::string &strUri) {
        if (strUri.empty()) {
          return false;
        }

        strUri_ = strUri;

        auto start = 0;
        auto fragmentStart = scan(strUri, '#', start, strUri.length());
        if (fragmentStart != -1) {
          fragment_ = std::string(strUri, fragmentStart + 1);

        } else {
          fragmentStart = strUri.length();
        }

        auto schemeEnd = scan(strUri, ":/?", start, fragmentStart);
        if (schemeEnd != -1 && strUri[schemeEnd] == ':') {

          bool isValidScheme = true;
          for (int i = start; i < schemeEnd; ++i) {
            if (!isValidSchemeChar(i, strUri[i])) {
              // invalid scheme, do not treat it as scheme
              isValidScheme = false;
              break;
            }
          }

          if (isValidScheme) {
            scheme_ = std::string(strUri, start, schemeEnd - start);
            start = schemeEnd + 1;
          }
        }

        if (regionMatches(strUri, "//", start)) {
          start += 2; // ignore "//"
        }

        auto authorityEnd = scan(strUri, "/?", start, fragmentStart);
        if (authorityEnd == -1) {
          authorityEnd = fragmentStart;
        }
        if (authorityEnd > start) {
          authority_ = std::string(strUri, start, authorityEnd - start);
          parseAuthority(strUri, start, authorityEnd);

          start = authorityEnd;
        }

        if (start < fragmentStart) {
          auto pathEnd = scan(strUri, '?', start, fragmentStart);
          if (pathEnd != -1) {
            path_ = std::string(strUri, start, pathEnd - start);
            start = pathEnd + 1;  // ignore '?'

            queryStr_ = std::string(strUri, start, fragmentStart - start);

          } else {
            path_ = std::string(strUri, start, fragmentStart - start);
          }
        }

        return true;
      }

      std::string getScheme() const {
        return scheme_;
      }

      std::string getAuthority() const {
        return authority_;
      }

      std::string getUserInfo() const {
        return userInfo_;
      }

      std::string getHost() const {
        return host_;
      }

      const uint16_t getPort() const {
        return port_;
      }

      std::string getPath() const {
        return path_;
      }

      std::string getQueryStr() const {
        return queryStr_;
      }

      std::string getFragment() const {
        return fragment_;
      }

      std::string getStrUri() const {
        return strUri_;
      }

    private:
      void parseAuthority(
        const std::string &strUri, std::size_t start, std::size_t end) {
        auto userInfoEnd = scan(strUri, '@', start, end);
        if (userInfoEnd != -1) {
          userInfo_ = std::string{strUri, start, userInfoEnd - start};
          start = userInfoEnd + 1;  // ignore '@'
        }

        // brackets are used for IPv6
        auto hasOpenBracket = start < end && strUri[start] == '[';
        if (hasOpenBracket) {
          ++start;
        }
        auto hostEnd = hasOpenBracket ?
          scan(strUri, ']', start, end) :
          scan(strUri, ':', start, end);
        if (hostEnd != -1) {
          host_ = std::string{strUri, start, hostEnd - start};
          start = hostEnd + 1;  // ignore ':'
          if (hasOpenBracket) {
            ++start;  // ignore ]
          }

          if (start < end) {
            for (std::size_t i = start; i < end; ++i) {
              if (!isdigit(strUri[i])) {
                return;
              }
            }

            port_ = std::stoi(strUri.substr(start, end - start));
          }

        } else {
          host_ = std::string{strUri, start, end - start};
        }
      }

      int scan(
        const std::string &strUri,
        const char *stopChars,
        std::size_t start,
        std::size_t end) {

        auto len = strlen(stopChars);
        while (start < end) {
          for (std::size_t i = 0; i < len; ++i) {
            if (strUri[start] == stopChars[i]) {
              return start;
            }
          }

          ++start;
        }

        return -1;
      }

      int scan(
        const std::string &strUri,
        char stopChar,
        int start,
        int end) {

        while (start < end) {
          if (strUri[start] == stopChar) {
            return start;
          }

          ++start;
        }

        return -1;
      }

      bool regionMatches(
        const std::string &s, const char *region, std::size_t start) {
        auto regionLen = strlen(region);
        auto strSize = s.size();
        if (regionLen > strSize) {
          return false;
        }

        while (*region != '\0' && start < strSize) {
          if (*region != s[start]) {
            return false;
          }

          ++region;
          ++start;
        }

        return true;
      }

      bool isValidSchemeChar(int index, char c) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
          return true;
        }

        // ref: https://stackoverflow.com/a/3641782/668963
        //
        // "+ - ." are valid chars for scheme, but I don't see
        // any existing schemes like that in practice, here I will
        // NOT treat these 3 chars as valid scheme chars to avoid
        // the case that for "www.google.com:443", "www.google.com" is
        // parsed as scheme
        return index > 0 && (c >= '0' && c <= '9');
      }

    private:
      std::string strUri_;
      std::string scheme_;
      std::string authority_;
      std::string userInfo_;
      std::string host_;
      uint16_t port_{0};
      std::string path_;
      std::string queryStr_;
      std::string fragment_;
  };
} /* end of namspace: nul */

#endif /* end of include guard: NUL_URI_H_ */
