#ifndef UTIL_MOVE_H
#define UTIL_MOVE_H

namespace util {
  template<typename _T>
  struct remove_reference {
    typedef _T type;
  };

  template<typename _T>
  struct remove_reference<_T&> {
    typedef _T type;
  };

  template<typename _T>
  struct remove_reference<_T&&> {
    typedef _T type;
  };

  template<typename _T>
  constexpr typename remove_reference<_T>::type&& move(_T&& a) noexcept
  {
    return static_cast<typename remove_reference<_T>::type&&>(a);
  }

  template<typename _T>
  inline void swap(_T& a, _T& b)
  {
    _T tmp(util::move(a));
    a = util::move(b);
    b = util::move(tmp);
  }
}

#endif // UTIL_MOVE_H
