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
        for (int i = 0; i < s.length(); ++i) {
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
        for (int i = 0; i < s.length(); ++i) {
          auto &c = s[i];

          if (c == ':') {
            if (++colonCount > 7) {
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

            // each chunk can at most have 4 chars 
            if (++charCountInEachGroup > 4) {
              return false;
            }
          }

          lastChar = c;
        }

        return colonCount >= 2;
      }
  };

} /* end of namspace: nul */

#endif /* end of include guard: NUL_UTIL_H_ */
