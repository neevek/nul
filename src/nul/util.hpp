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

} /* end of namspace: nul */

#endif /* end of include guard: NUL_UTIL_H_ */
