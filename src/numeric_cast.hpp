//
// Created by Austin on 10/10/2015.
//

#ifndef TASK_NUMERIC_CAST_HPP_H
#define TASK_NUMERIC_CAST_HPP_H
#include <type_traits>
#undef max
#undef min

// Based off boost.numeric_cast
template <typename Target, typename Source>
Target numeric_cast(Source arg) {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4018)
#endif
  if ((arg < 0 && !std::is_signed<Target>::value)
       || (std::is_signed<Source>::value && arg < std::numeric_limits<Target>::min())
       || (arg > std::numeric_limits<Target>::max()))
  {
    throw std::bad_cast();
  }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

  return static_cast<Target>(arg);
}

#endif //TASK_NUMERIC_CAST_HPP_H
