#pragma once
#include <typeindex>

namespace raven
{

template <typename Writer>
class Serializer
{
public:
	template <typename... Args>
	constexpr Serializer(Args&& ... args)
		: m_writer{ std::forward<Args>(args)... }
	{}

	constexpr Serializer(Serializer&&) = default;
	constexpr Serializer& operator=(Serializer&&) = default;

	constexpr void Write(const std::type_index& valueTypeIndex, void* value)
	{
		m_writer.Write(valueTypeIndex, value);
	}

	template <typename T>
	constexpr void Write(const T& value)
	{
		m_writer.Write(value);
	}

	constexpr const Writer& GetWriter() const { return m_writer; }
	constexpr Writer& GetWriter() { return m_writer; }

private:
	Writer m_writer;

	Serializer(const Serializer&) = delete;
	Serializer& operator=(const Serializer&) = delete;
};

} // namespace raven
