/*******************************************************************************
**          File: cpp11_compat.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2019-09-18 Wed 08:20 PM
**   Description: c++11 compatable functions
*******************************************************************************/
#ifndef CPP11_COMPAT_H_
#define CPP11_COMPAT_H_

#if __cplusplus == 201103L || (defined(_MSC_VER) && _MSC_VER == 1900)
namespace std {
  template<class T> struct _Unique_if {
    typedef unique_ptr<T> _Single_object;
  };

  template<class T> struct _Unique_if<T[]> {
    typedef unique_ptr<T[]> _Unknown_bound;
  };

  template<class T, size_t N> struct _Unique_if<T[N]> {
    typedef void _Known_bound;
  };

  template<class T, class... Args>
    typename _Unique_if<T>::_Single_object
    make_unique(Args&&... args) {
      return unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

  template<class T>
    typename _Unique_if<T>::_Unknown_bound
    make_unique(size_t n) {
      typedef typename remove_extent<T>::type U;
      return unique_ptr<T>(new U[n]());
    }

  template<class T, class... Args>
    typename _Unique_if<T>::_Known_bound
    make_unique(Args&&...) = delete;
}
#endif

#endif /* end of include guard: CPP11_COMPAT_H_ */
