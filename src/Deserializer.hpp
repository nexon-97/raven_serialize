#pragma once
#include <typeindex>

namespace raven
{

template <typename Reader>
class Deserializer
{
public:
	template <typename... Args>
	constexpr Deserializer(Args&& ... args)
		: m_reader{ std::forward<Args>(args)... }
	{}

	constexpr Deserializer(Deserializer&&) = default;
	constexpr Deserializer& operator=(Deserializer&&) = default;

	template <typename T>
	constexpr void Read(T& value)
	{
		m_reader.Read(value);
	}

	constexpr const Reader& GetReader() const { return m_reader; }
	constexpr Reader& GetReader() { return m_reader; }

private:
	Reader m_reader;

	Deserializer(const Deserializer&) = delete;
	Deserializer& operator=(const Deserializer&) = delete;
};

} // namespace raven
