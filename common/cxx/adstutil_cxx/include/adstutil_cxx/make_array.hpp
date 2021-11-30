#pragma once

#include <array>

namespace adst::common {

template <typename... T>
constexpr auto make_array(T&&... values)
    -> std::array<typename std::decay<typename std::common_type<T...>::type>::type, sizeof...(T)>
{
    return std::array<typename std::decay<typename std::common_type<T...>::type>::type, sizeof...(T)>{
        std::forward<T>(values)...};
}
} // namespace adst::common
