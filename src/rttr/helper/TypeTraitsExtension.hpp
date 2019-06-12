#pragma once
#include <vector>
#include <array>

namespace rttr
{

template <typename>
struct is_std_vector : std::false_type {};

template <typename T, typename A>
struct is_std_vector<std::vector<T, A>> : std::true_type {};

template <typename>
struct is_std_array : std::false_type {};

template <typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

/*template <typename T>
struct is_array_ext : std::enable_if<std::disjunction<is_std_vector<T>::value, is_std_array<T>::value, std::is_array<T>::value>::value>, std::true_type> {};*/

}
