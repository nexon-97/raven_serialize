#pragma once
#include "writers/JsonWriter.hpp"

namespace rs
{

class StreamJsonWriter : public JsonWriter
{
public:
	StreamJsonWriter() = delete;
	explicit RAVEN_SERIALIZE_API StreamJsonWriter(std::ostream& stream, const bool prettyPrint = true);
	~StreamJsonWriter() = default;

	bool RAVEN_SERIALIZE_API Write(const rttr::Type& type, const void* value) override;

private:
	void WritePadding();

private:
	std::ostream& m_stream;
	const bool m_prettyPrint;
	int m_padding = 0;
};

}
