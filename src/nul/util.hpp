/*******************************************************************************
**          File: util.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-02 Thu 09:52 AM
**   Description: utilities 
*******************************************************************************/
#ifndef NUL_UTIL_H_
#define NUL_UTIL_H_
#include <memory>

namespace nul {

  auto ByteArrayDeleter = [](char *p) { delete [] p; };
  using ByteArray = std::unique_ptr<char, decltype(ByteArrayDeleter)>;

} /* end of namspace: nul */

#endif /* end of include guard: NUL_UTIL_H_ */
