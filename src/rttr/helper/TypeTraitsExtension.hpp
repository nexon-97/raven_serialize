#pragma once
#include <vector>
#include <array>
#include <string>

namespace rttr
{

template <typename>
struct is_string : std::false_type {};

template <>
struct is_string<std::string> : std::true_type {};

template <>
struct is_string<const char*> : std::true_type {};

template <typename>
struct is_std_vector : std::false_type {};

template <typename T, typename A>
struct is_std_vector<std::vector<T, A>> : std::true_type {};

template <typename T>
struct std_vector_type
{
	typedef nullptr_t type;
};

template <typename T, typename A>
struct std_vector_type<std::vector<T, A>>
{
	typedef T type;
};

template <typename>
struct is_std_array : std::false_type {};

template <typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

}
