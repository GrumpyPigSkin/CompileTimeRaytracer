#ifndef OVERLOADED_H_
#define OVERLOADED_H_

#include <type_traits>
#include <utility>

namespace detail {
template <class... Ts> struct Overloaded;

template <> struct Overloaded<> {};

template <class T> struct Overloaded<T> : T {
  using T::operator();

  template <class U> constexpr Overloaded(U &&u) : T{std::forward<U>(u)} {}
};

template <class T, class... Ts>
struct Overloaded<T, Ts...> : T, Overloaded<Ts...> {
  using T::operator();
  using Overloaded<Ts...>::operator();

  template <class U, class... Us>
  constexpr Overloaded(U &&u, Us &&...us)
      : T{std::forward<U>(u)}, Overloaded<Ts...>{std::forward<Us>(us)...} {}
};

} // namespace detail

template <class... Ts> constexpr auto Overload(Ts &&...ts) {
  return detail::Overloaded<typename std::decay<Ts>::type...>{
      std::forward<Ts>(ts)...};
}

#endif
