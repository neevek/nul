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
#include "log.hpp"

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
        if (start == 0 && end == s.length() - 1) {
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
      static bool isIPv4(const std::string &s) {
        if (s.empty()) {
          return false;
        }

        auto sum = 0;
        auto dotCount = 0;
        auto lastChar = '\0';
        for (std::size_t i = 0; i < s.length(); ++i) {
          auto &c = s[i];
          if (c == '.') {
            if (lastChar == '\0' || lastChar == '.' || ++dotCount > 3) {
              return false;
            }
            sum = 0;

          } else {
            if (c < '0' || c > '9') {
              return false;
            }

            sum = sum * 10 + (c - '0');
            if (sum > 255) {
              return false;
            }
          }

          lastChar = c;
        }
        return dotCount == 3 && lastChar != '.';
      }

      static bool isIPv6(const std::string &s) {
        if (s.empty()) {
          return false;
        }

        auto charCountInEachGroup = 0;
        auto colonCount = 0;
        auto hasConsecutiveGroup = false;
        auto lastChar = '\0';
        for (std::size_t i = 0; i < s.length(); ++i) {
          auto &c = s[i];

          if (c == ':') {
            if (++colonCount > 7 &&
                (s[0] != ':' && s[s.length() - 1] != ':')) {
              return false;
            }
            if (lastChar == ':') {
              if (hasConsecutiveGroup) {
                // each IPv6 address can only one consecutive group
                return false;
              }
              hasConsecutiveGroup = true;
            }
            charCountInEachGroup = 0;

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
      static std::string expandIPv6(const std::string &s, bool *ret = nullptr) {
        if (!isIPv6(s)) {
          if (ret) {
            *ret = false;
          }
          return s;
        }

        auto result = std::string(4 * 8 + 7, '\0');
        result.clear();

        auto colons = 0;
        for (auto &c : s) {
          if (c == ':') {
            ++colons;
          }
        }

        auto groupStart = 0;

        auto slen = s.length();
        for (std::size_t i = 0; i < slen; ++i) {
          auto nextIndex = i + 1;

          if (s[i] == ':' || nextIndex == slen) {
            auto glen = i - groupStart;
            // if we reach the end, and the current char is not ':', then
            // the last char SHOULDN'T be ':', so increment 'glen' by 1 to
            // account for the last char
            if (s[i] != ':' && nextIndex == slen) {
              ++glen;
            }

            if (glen > 0) {
              if (groupStart > 0) {
                result.append(1, ':');
              }
              if (glen < 4) {
                result.append(4 - glen, '0');
              }
              result.append(s, groupStart, glen);

              groupStart = nextIndex;

            } else {
              ++groupStart;
            }

            if (nextIndex == slen && s[i] == ':') {
              result.append(":0000", 5);

            } else if (nextIndex < slen && s[nextIndex] == ':') {
              // glen=0 when consecutive groups is at the leading of the string
              if (glen == 0) {
                --colons;
              }

              auto groupsToAppend = 8 - colons;
              for (std::size_t j = 0; j < groupsToAppend; ++j) {
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
        return result;
      }

      static bool ipv4ToBinary(const std::string &ip, char ipv4Bin[4]) {
        if (!isIPv4(ip)) {
          return false;
        }

        std::sscanf(
          "%03d.%03d.%03d.%03d",
          &ipv4Bin[0], &ipv4Bin[1], &ipv4Bin[2], &ipv4Bin[3]);
        return true;
      }

      static bool ipv6ToBinary(const std::string &ip, char ipv6Bin[16]) {
        bool ret = false;
        auto expandedIp = expandIPv6(ip, &ret);
        if (!ret) {
          return false;
        }

        std::sscanf(
          "%02x%02x%02x%02x:"
          "%02x%02x%02x%02x:"
          "%02x%02x%02x%02x:"
          "%02x%02x%02x%02x:"
          "%02x%02x%02x%02x:"
          "%02x%02x%02x%02x:"
          "%02x%02x%02x%02x:"
          "%02x%02x%02x%02x",
          &ipv6Bin[0], &ipv6Bin[1], &ipv6Bin[2], &ipv6Bin[3],
          &ipv6Bin[4], &ipv6Bin[5], &ipv6Bin[6], &ipv6Bin[7],
          &ipv6Bin[8], &ipv6Bin[9], &ipv6Bin[10], &ipv6Bin[11],
          &ipv6Bin[12], &ipv6Bin[13], &ipv6Bin[14], &ipv6Bin[15]);
        return true;
      }

      static bool maskIPv4(const char *ip, const char *subnet) {
        return maskIP(ip, subnet, 4);
      }

      static bool maskIPv6(const char *ip, const char *subnet) {
        return maskIP(ip, subnet, 16);
      }

    private:
      static bool maskIP(const char *ip, const char *subnet, int len) {
        for (int i = 0; i < len; ++i) {
          char c = *(subnet + i);
          if ((c & ip[0]) != c) {
            return false;
          }
        }
        return true;
      }
  };

} /* end of namspace: nul */

#endif /* end of include guard: NUL_UTIL_H_ */
