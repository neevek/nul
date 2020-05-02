/*******************************************************************************
**          File: util.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-02 Thu 09:52 AM
**   Description: utilities
*******************************************************************************/
#ifndef NUL_UTIL_H_
#define NUL_UTIL_H_
#include <memory>
#include <string>
#include <functional>
#include <algorithm>
#include <vector>
#include <cinttypes>
#include "log.h"

namespace nul {

  auto ByteArrayDeleter = [](char *p) { delete [] p; };
  using ByteArray = std::unique_ptr<char, decltype(ByteArrayDeleter)>;

  class StringUtil {
    public:
      static std::string tolower(std::string &s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){
          return std::tolower(c);
        });
        return s;
      }

      static std::string trim(const std::string &s) {
        if (s.empty()) {
          return s;
        }
        std::string::size_type start;
        for (start = 0; start < s.length(); ++start) {
          if (!std::isspace(s[start])) {
            break;
          }
        }
        if (start == s.length()) {
          return "";
        }

        std::string::size_type end;
        for (end = s.length(); end > 0; --end) {
          if (!std::isspace(s[end - 1])) {
            break;
          }
        }
        if (start == 0 && end == s.length()) {
          return s;
        }
        return s.substr(start, end - start);
      }

      static bool split(
        const std::string &s,
        const std::string &seprator,
        std::function<bool(std::string::size_type index,
                           const std::string &part)> visitor) {
        std::string::size_type pos0 = 0;
        std::string::size_type pos1 = 0;
        std::string::size_type index = 0;
        while ((pos1 = s.find(seprator, pos0)) != std::string::npos) {
          if (pos1 > pos0) {
            if (!visitor(index++, s.substr(pos0, pos1 - pos0))) {
              return false;
            }
          }
          pos0 = pos1 + seprator.length();
          pos1 = pos0;
        }

        if (pos0 < s.length()) {
          return visitor(index, s.substr(pos0));
        }
        return true;
      }

      static std::vector<std::string> split(
        const std::string &s, const std::string &seprator) {
        auto v = std::vector<std::string>{};
        split(s, seprator, [&v](auto _, const auto &part) {
          v.push_back(part);
          return true;
        });
        return v;
      }
  };

  class NetUtil {
    public:
      // e.g. 192.168.22.31, 192.168.22.0/24
      static bool isIPv4(const std::string &s, bool hasMask = false) {
        if (s.empty() || s.length() > 18) {
          return false;
        }

        auto sum = 0;
        auto dotCount = 0;
        auto lastChar = '\0';
        auto eatingMask = false;
        auto maskValue = 0;
        for (std::size_t i = 0; i < s.length(); ++i) {
          auto &c = s[i];
          if (eatingMask) {
            if (c < '0' && c > '9') {
              LOG_E("invalid mask");
              return false;
            }
            maskValue = maskValue * 10 + (c - '0');
            if (maskValue > 32) {
              return false;
            }

          } else if (c == '.') {
            if (lastChar == '\0' || lastChar == '.' || ++dotCount > 3) {
              return false;
            }
            sum = 0;

          } else {
            if (hasMask && c == '/' && dotCount == 3 && lastChar != '.') {
              eatingMask = true;

            } else {
              if (c < '0' || c > '9') {
                return false;
              }

              sum = sum * 10 + (c - '0');
              if (sum > 255) {
                return false;
              }
            }
          }

          lastChar = c;
        }
        return dotCount == 3 && lastChar != '.';
      }

      static bool isIPv6(const std::string &s, bool hasMask = false) {
        if (s.empty() || s.length() > 43) {
          return false;
        }

        auto charCountInEachGroup = 0;
        auto colonCount = 0;
        auto hasConsecutiveGroup = false;
        auto lastChar = '\0';
        auto eatingMask = false;
        auto maskValue = 0;
        for (std::size_t i = 0; i < s.length(); ++i) {
          auto &c = s[i];
          if (eatingMask) {
            if (c < '0' || c > '9') {
              LOG_E("invalid mask");
              return false;
            }
            maskValue = maskValue * 10 + (c - '0');
            if (maskValue > 128) {
              return false;
            }

          } else if (c == ':') {
            if (++colonCount > 7 &&
                (s[0] != ':' && s[s.length() - 1] != ':')) {
              return false;
            }
            if (lastChar == ':') {
              if (hasConsecutiveGroup) {
                // each IPv6 address can only have one consecutive group
                return false;
              }
              hasConsecutiveGroup = true;
            }
            charCountInEachGroup = 0;

          } else {

            if (hasMask && c == '/' && (colonCount >= 7 || hasConsecutiveGroup)) {
              eatingMask = true;

            } else {
              auto isValidIPv6Char =
                (c >= '0' && c <= '9') ||
                (c >= 'a' && c <= 'f') ||
                (c >= 'A' && c <= 'F');
              if (!isValidIPv6Char) {
                return false;
              }

              // each group can at most have 4 chars
              if (++charCountInEachGroup > 4) {
                return false;
              }
            }
          }

          lastChar = c;
        }

        return colonCount >= 2;
      }

      static std::string ipToHex(const std::string &ip) {
        return ip;
      }

      /**
       * return original string if it is not an IPv6 address
       */
      static std::string expandIPv6(const std::string &ip, bool *ret = nullptr) {
        auto slashPos = ip.rfind("/");
        if (!isIPv6(ip, slashPos != std::string::npos)) {
          if (ret) {
            *ret = false;
          }
          return ip;
        }

        auto result = std::string(4 * 8 + 7, '\0');
        result.clear();

        auto colons = 0;
        for (auto &c : ip) {
          if (c == ':') {
            ++colons;
          }
        }

        auto groupStart = 0;

        auto slen = slashPos != std::string::npos ? slashPos : ip.length();
        for (std::size_t i = 0; i < slen; ++i) {
          auto nextIndex = i + 1;

          if (ip[i] == ':' || nextIndex == slen) {
            auto glen = i - groupStart;
            // if we reach the end, and the current char is not ':', then
            // the last char SHOULDN'T be ':', so increment 'glen' by 1 to
            // account for the last char
            if (ip[i] != ':' && nextIndex == slen) {
              ++glen;
            }

            if (glen > 0) {
              if (groupStart > 0) {
                result.append(1, ':');
              }
              if (glen < 4) {
                result.append(4 - glen, '0');
              }
              result.append(ip, groupStart, glen);

              groupStart = nextIndex;

            } else {
              ++groupStart;
            }

            if (nextIndex == slen && ip[i] == ':') {
              result.append(":0000", 5);

            } else if (nextIndex < slen && ip[nextIndex] == ':') {
              // glen=0 when consecutive groups is at the leading of the string
              if (glen == 0) {
                --colons;
              }

              auto groupsToAppend = 8 - colons;
              for (auto j = 0; j < groupsToAppend; ++j) {
                if (result.length() > 0) {
                  result.append(1, ':');
                }
                result.append("0000", 4);
              }

            }
          }
        }

        if (ret) {
          *ret = true;
        }

        if (slashPos != std::string::npos) {
          result.append(ip.substr(slashPos));
        }
        return result;
      }

      static bool ipv4ToBinary(const std::string &ip, uint8_t ipv4Bin[4]) {
        if (!isIPv4(ip)) {
          return false;
        }

        std::sscanf(
          ip.c_str(),
          "%03" SCNu8 ".%03" SCNu8 ".%03" SCNu8 ".%03" SCNu8,
          &ipv4Bin[0], &ipv4Bin[1], &ipv4Bin[2], &ipv4Bin[3]);
        return true;
      }

      static bool ipv4ToBinary(
        const std::string &ipWithMask, uint8_t ipv4Bin[4], /*out*/ int &mask) {
        if (!isIPv4(ipWithMask, true)) {
          return false;
        }

        std::sscanf(
          ipWithMask.c_str(),
          "%03" SCNu8 ".%03" SCNu8 ".%03" SCNu8 ".%03" SCNu8  "/%2d",
          &ipv4Bin[0], &ipv4Bin[1], &ipv4Bin[2], &ipv4Bin[3], &mask);
        return true;
      }

      static bool ipv6ToBinary(const std::string &ip, uint8_t ipv6Bin[16]) {
        bool ret = false;
        auto expandedIp = expandIPv6(ip, &ret);
        if (!ret) {
          return false;
        }

        std::sscanf(
          expandedIp.c_str(),
          "%2" SCNx8 "%2" SCNx8 ":%2" SCNx8 "%2" SCNx8 ":"
          "%2" SCNx8 "%2" SCNx8 ":%2" SCNx8 "%2" SCNx8 ":"
          "%2" SCNx8 "%2" SCNx8 ":%2" SCNx8 "%2" SCNx8 ":"
          "%2" SCNx8 "%2" SCNx8 ":%2" SCNx8 "%2" SCNx8,
          &ipv6Bin[0], &ipv6Bin[1], &ipv6Bin[2], &ipv6Bin[3],
          &ipv6Bin[4], &ipv6Bin[5], &ipv6Bin[6], &ipv6Bin[7],
          &ipv6Bin[8], &ipv6Bin[9], &ipv6Bin[10], &ipv6Bin[11],
          &ipv6Bin[12], &ipv6Bin[13], &ipv6Bin[14], &ipv6Bin[15]);
        return true;
      }

      static bool ipv6ToBinary(
        const std::string &ipWithMask, uint8_t ipv6Bin[16], /*out*/int &mask) {
        bool ret = false;
        auto expandedIp = expandIPv6(ipWithMask, &ret);
        if (!ret) {
          return false;
        }

        std::sscanf(
          expandedIp.c_str(),
          "%2" SCNx8 "%2" SCNx8 ":%2" SCNx8 "%2" SCNx8 ":"
          "%2" SCNx8 "%2" SCNx8 ":%2" SCNx8 "%2" SCNx8 ":"
          "%2" SCNx8 "%2" SCNx8 ":%2" SCNx8 "%2" SCNx8 ":"
          "%2" SCNx8 "%2" SCNx8 ":%2" SCNx8 "%2" SCNx8 "/%2d",
          &ipv6Bin[0], &ipv6Bin[1], &ipv6Bin[2], &ipv6Bin[3],
          &ipv6Bin[4], &ipv6Bin[5], &ipv6Bin[6], &ipv6Bin[7],
          &ipv6Bin[8], &ipv6Bin[9], &ipv6Bin[10], &ipv6Bin[11],
          &ipv6Bin[12], &ipv6Bin[13], &ipv6Bin[14], &ipv6Bin[15], &mask);
        return true;
      }

      // e.g. ip = 192.168.23.44, ipWithMask = 192.168.23.0/24
      static bool ipv4IsInSubnet(
        const std::string &ip, const std::string &ipWithMask) {
        uint8_t ipv4Bin[4];
        if (!ipv4ToBinary(ip, ipv4Bin)) {
          return false;
        }

        uint8_t subnet[4];
        int mask = 32;
        if (!ipv4ToBinary(ipWithMask, subnet, mask)) {
          return false;
        }

        return ipv4IsInSubnet(ipv4Bin, subnet, mask);
      }

      // e.g. ip = ff::1, ipWithMask = ff::1/64
      static bool ipv6IsInSubnet(
        const std::string &ip, const std::string &ipWithMask) {
        uint8_t ipv6Bin[16];
        if (!ipv6ToBinary(ip, ipv6Bin)) {
          return false;
        }

        uint8_t subnet[16];
        int mask = 128;
        if (!ipv6ToBinary(ipWithMask, subnet, mask)) {
          return false;
        }

        return ipv6IsInSubnet(ipv6Bin, subnet, mask);
      }

      // 4-byte IPv4 address
      static bool ipv4IsInSubnet(
        const uint8_t *ip, const uint8_t *subnet, int prefix) {
        return ipIsInSubnet(ip, subnet, 4, prefix);
      }

      // 16-byte IPv6 address
      static bool ipv6IsInSubnet(
        const uint8_t *ip, const uint8_t *subnet, int prefix) {
        return ipIsInSubnet(ip, subnet, 16, prefix);
      }

    private:
      static bool ipIsInSubnet(
        const uint8_t *ip, const uint8_t *subnet, int len, int prefix) {
        for (int i = 0; i < len && prefix > 0; ++i) {
          auto shift = 8 - std::min(8, prefix);
          if ((ip[i] >> shift) != (subnet[i] >> shift)) {
            return false;
          }
          prefix -= 8;
        }
        return true;
      }
  };

} /* end of namspace: nul */

#endif /* end of include guard: NUL_UTIL_H_ */
