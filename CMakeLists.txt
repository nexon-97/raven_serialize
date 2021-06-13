project(raven_serialize)

set(CMAKE_CXX_STANDARD 17)
cmake_minimum_required(VERSION 3.18)

set(SERIALIZE_SRCS
	src/ContextPath.cpp
	src/SerializationContext.cpp
	src/actions/CallObjectMutatorAction.cpp
	src/actions/CollectionInsertAction.cpp
	src/actions/ResolvePointerAction.cpp
	src/readers/BaseReader.cpp
	src/readers/JsonReader.cpp
	src/readers/ReadResult.cpp
	src/rs/SerializationKeywords.cpp
	src/rs/log/Log.cpp
	src/rttr/Manager.cpp
	src/rttr/Type.cpp
	src/writers/JsonWriter.cpp
	src/writers/StreamJsonWriter.cpp)

add_library(raven_serialize SHARED ${SERIALIZE_SRCS})

target_include_directories(raven_serialize
	PUBLIC
		src
		${CMAKE_CURRENT_BINARY_DIR}
)

include(GenerateExportHeader)
generate_export_header(raven_serialize
	EXPORT_MACRO_NAME RAVEN_SERIALIZE_API)

find_library(JSONCPP_LIB jsoncpp REQUIRED)
target_link_libraries(raven_serialize jsoncpp)

