#include "readers/JsonReader.hpp"
#include "rttr/Property.hpp"
#include "rttr/CustomTypeResolver.hpp"
#include "rttr/Manager.hpp"
#include "ptr/DefaultPointerFiller.hpp"
#include "ptr/PropertyPointerFiller.hpp"
#include "actions/CallObjectMutatorAction.hpp"
#include "actions/ResolvePointerAction.hpp"
#include "actions/CustomResolverAction.hpp"

namespace
{
const char* k_typeId = "$type$";
const char* k_contextTypeName = "@context@";
const char* k_contextObjectsKey = "$objects$";
const char* k_smartptrTypeKey = "@smartptr@";
const char* k_sysTypeId = "$sys_type$";
const char* k_smartptrPointedValueKey = "ptr_value";
const char* k_ptrMarkerKey = "$marker_id$";
}

namespace rs
{

JsonReader::JsonReader(std::istream& stream)
	: m_stream(stream)
{
	std::size_t startOffset = stream.tellg();
	m_stream.seekg(0, std::ios::end);
	std::size_t bufferSize = static_cast<std::size_t>(m_stream.tellg()) - startOffset;
	m_stream.seekg(startOffset, std::ios::beg);

	char* buffer = new char[bufferSize];
	m_stream.read(buffer, bufferSize);

	Json::CharReaderBuilder builder;
	auto reader = builder.newCharReader();
	std::string errorStr;

	m_isOk = reader->parse(buffer, buffer + bufferSize, &m_jsonRoot, &errorStr);

	if (!m_isOk)
	{
		m_stream.seekg(startOffset, std::ios::beg);
	}

	delete[] buffer;
}

void JsonReader::SortActions()
{
	using ActionPtr = std::unique_ptr<detail::IReaderAction>;
	auto predicate = [](const ActionPtr& lhs, const ActionPtr& rhs)
	{
		// First sort by action types
		if (lhs->GetActionType() != rhs->GetActionType())
		{
			return static_cast<int>(lhs->GetActionType()) < static_cast<int>(rhs->GetActionType());
		}

		// Then, if the same class, go the deeper operations
		return lhs->GetDepth() > rhs->GetDepth();
	};
	std::stable_sort(m_actions.begin(), m_actions.end(), predicate);
}

void JsonReader::Read(const rttr::Type& type, void* value)
{
	//
	auto readerAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(this));
	Log::LogMessage("=========\nJsonReader [0x%llX] started reading session\n=========", readerAsInt);

	std::string className = GetObjectClassName(m_jsonRoot);
	m_context = std::make_unique<rs::detail::SerializationContext>();

	if (className == k_contextTypeName)
	{
		// Parse context
		const Json::Value& contextObjects = m_jsonRoot[k_contextObjectsKey];
		std::size_t idx = 0U;
		for (const Json::Value& contextObject : contextObjects)
		{
			rttr::Type objectType = DeduceType(contextObject);
			m_currentRootObject = nullptr;

			if (idx == 0U)
			{
				m_currentRootObject = value;
			}
			else
			{
				m_currentRootObject = objectType.Instantiate();
			}

			//
			auto valueAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(m_currentRootObject));
			auto readerAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(this));
			Log::LogMessage("JsonReader [0x%llX] started reading object: (%s, 0x%llX)", readerAsInt, objectType.GetName(), valueAsInt);

			ReadImpl(objectType, m_currentRootObject, contextObject);

			//
			valueAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(m_currentRootObject));
			readerAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(this));
			Log::LogMessage("JsonReader [0x%llX] ended reading object: (%s, 0x%llX)", readerAsInt, objectType.GetName(), valueAsInt);

			// Check object marker
			if (contextObject.isObject() && contextObject.isMember(k_ptrMarkerKey))
			{
				std::size_t markerId = static_cast<std::size_t>(contextObject[k_ptrMarkerKey].asUInt());
				m_context->AddObject(markerId, objectType, m_currentRootObject);
			}

			m_currentRootObject = nullptr;

			++idx;
		}
	}
	else
	{
		m_currentRootObject = value;

		ReadImpl(type, m_currentRootObject, m_jsonRoot);

		m_currentRootObject = nullptr;
	}

	// Perform actions
	SortActions();
	for (const auto& action : m_actions)
	{
		action->Perform();
	}

	m_context->ClearTempVariables();

	//
	readerAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(this));
	Log::LogMessage("=========\nJsonReader [0x%llX] ended reading session\n=========", readerAsInt);
}

void JsonReader::ReadImpl(const rttr::Type& type, void* value, const Json::Value& jsonVal)
{
	assert(m_isOk);

	// Find custom type resolver for this type
	auto customResolverIt = m_customTypeResolvers.find(type.GetTypeIndex());
	if (customResolverIt != m_customTypeResolvers.end())
	{
		rttr::CustomTypeResolver* customTypeResolver = customResolverIt->second;

		// Serialized value type
		rttr::Type serializedValueType = DeduceType(jsonVal);
		void* serializedValue = nullptr;

		// Deserialize custom value from data source to pass to resolver
		if (serializedValueType.GetTypeIndex() != typeid(nullptr_t))
		{
			serializedValue = m_context->CreateTempVariable(serializedValueType);
			ReadImpl(serializedValueType, serializedValue, jsonVal);
		}

		auto resolverAction = std::make_unique<detail::CustomResolverAction>(m_contextPath.GetSize()
			, type, value, serializedValueType, serializedValue, customTypeResolver);
		m_actions.push_back(std::move(resolverAction));
	}
	else if (type.GetTypeIndex() == typeid(nullptr_t))
	{
		// Skip nullptr
	}
	else if (type.GetTypeIndex() == typeid(bool))
	{
		*static_cast<bool*>(value) = jsonVal.asBool();
	}
	else if (type.IsString())
	{
		if (jsonVal.isNull())
		{
			if (type.GetTypeIndex() == typeid(std::string))
			{
				*static_cast<std::string*>(value) = std::string();
			}
			else if (type.GetTypeIndex() == typeid(std::wstring))
			{
				*static_cast<std::wstring*>(value) = std::wstring();
			}
			else if (type.GetTypeIndex() == typeid(const char*))
			{
				char** strSerializedValue = reinterpret_cast<char**>(value);
				*strSerializedValue = nullptr;
			}
		}
		else if (jsonVal.isString())
		{
			const char* strValue = jsonVal.asCString();

			if (type.GetTypeIndex() == typeid(std::string))
			{
				*static_cast<std::string*>(value) = strValue;
			}
			else if (type.GetTypeIndex() == typeid(std::wstring))
			{
				*static_cast<std::wstring*>(value) = std::wstring(L"default_wstring");
			}
			else if (type.GetTypeIndex() == typeid(const char*))
			{
				char** strSerializedValue = reinterpret_cast<char**>(value);
				*strSerializedValue = const_cast<char*>(strValue);
			}
		}
	}
	else if (type.IsIntegral())
	{
		if (type.IsSigned())
		{
			int64_t intValue = jsonVal.asInt64();

			switch (type.GetSize())
			{
				case 1:
					*static_cast<int8_t*>(value) = static_cast<int8_t>(intValue);
					break;
				case 2:
					*static_cast<int16_t*>(value) = static_cast<int16_t>(intValue);
					break;
				case 4:
					*static_cast<int32_t*>(value) = static_cast<int32_t>(intValue);
					break;
				case 8:
				default:
					*static_cast<int64_t*>(value) = intValue;
					break;
			}
		}
		else
		{
			uint64_t intValue = jsonVal.asUInt64();

			switch (type.GetSize())
			{
				case 1:
					*static_cast<uint8_t*>(value) = static_cast<uint8_t>(intValue);
					break;
				case 2:
					*static_cast<uint16_t*>(value) = static_cast<uint16_t>(intValue);
					break;
				case 4:
					*static_cast<uint32_t*>(value) = static_cast<uint32_t>(intValue);
					break;
				case 8:
				default:
					*static_cast<uint64_t*>(value) = intValue;
					break;
			}
		}
	}
	else if (type.IsFloatingPoint())
	{
		double floatValue = jsonVal.asDouble();

		if (type.GetTypeIndex() == typeid(float))
		{
			*static_cast<float*>(value) = static_cast<float>(floatValue);
		}
		else
		{
			*static_cast<double*>(value) = floatValue;
		}
	}
	else if (type.IsSmartPointer())
	{
		const Json::Value& smartPtrValue = jsonVal[k_smartptrPointedValueKey];
		if (smartPtrValue.isObject())
		{
			std::size_t markerId = static_cast<std::size_t>(smartPtrValue[k_ptrMarkerKey].asUInt());
			auto resolvePtrAction = std::make_unique<detail::ResolvePointerAction>(m_contextPath.GetSize(), m_context.get(), type, value, markerId);
			m_actions.push_back(std::move(resolvePtrAction));
		}
	}
	else if (type.IsArray())
	{
		// Resize dynamic array
		if (type.IsDynamicArray())
		{
			std::size_t count = jsonVal.size();
			type.SetDynamicArraySize(value, count);
		}

		rttr::Type itemType = type.GetUnderlyingType();

		std::size_t i = 0U;
		for (const Json::Value& jsonItem : jsonVal)
		{
			m_contextPath.PushArrayItemAction(i);

			void* itemValuePtr = type.GetArrayItemValuePtr(value, i);
			
			//
			auto valueAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(value));
			auto itemAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(itemValuePtr));
			auto readerAsInt = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(this));
			Log::LogMessage("JsonReader [0x%llX] array item ptr: (array at 0x%llX, item at 0x%llX)", readerAsInt, valueAsInt, itemAsInt);

			ReadImpl(itemType, itemValuePtr, jsonItem);
			++i;

			m_contextPath.PopAction();
		}

		// [DEBUG]
		type.GetDynamicArraySize(value);
	}
	else if (type.IsClass())
	{
		std::size_t propertiesCount = type.GetPropertiesCount();
		for (std::size_t i = 0U; i < propertiesCount; ++i)
		{
			rttr::Property* property = type.GetProperty(i);
			const rttr::Type& propertyType = property->GetType();
			const char* propertyName = property->GetName();

			if (jsonVal.isMember(propertyName))
			{
				const Json::Value& itemJsonVal = jsonVal[propertyName];

				m_contextPath.PushObjectPropertyAction(propertyName);

				// Decide to create temp variable or not
				void* propertyValuePtr = nullptr;
				if (property->NeedsTempVariable())
				{
					propertyValuePtr = m_context->CreateTempVariable(property->GetType());
				}
				else
				{
					propertyValuePtr = property->GetValueAddress(value);
				}

				ReadImpl(propertyType, propertyValuePtr, itemJsonVal);

				auto callMutatorAction = std::make_unique<detail::CallObjectMutatorAction>(m_contextPath.GetSize(), property, value, propertyValuePtr);
				m_actions.push_back(std::move(callMutatorAction));

				m_contextPath.PopAction();
			}
			else
			{
				// [TODO] Log property read error here
			}
		}
	}
	else if (type.IsPointer())
	{
		if (jsonVal.isObject() && jsonVal.isMember(k_ptrMarkerKey))
		{
			std::size_t markerId = static_cast<std::size_t>(jsonVal[k_ptrMarkerKey].asUInt());
			auto resolvePtrAction = std::make_unique<detail::ResolvePointerAction>(m_contextPath.GetSize(), m_context.get(), type, value, markerId);
			m_actions.push_back(std::move(resolvePtrAction));
		}
	}
}

bool JsonReader::IsOk() const
{
	return m_isOk;
}

void JsonReader::AddCustomTypeResolver(const rttr::Type& type, rttr::CustomTypeResolver* resolver)
{
	m_customTypeResolvers.emplace(type.GetTypeIndex(), resolver);
}

std::string JsonReader::GetObjectClassName(const Json::Value& jsonVal) const
{
	if (jsonVal.isObject() && jsonVal.isMember(k_typeId))
	{
		return jsonVal[k_typeId].asString();
	}

	return std::string();
}

rttr::Type JsonReader::DeduceType(const Json::Value& jsonVal) const
{
	switch (jsonVal.type())
	{
		case Json::ValueType::stringValue:
			return rttr::Reflect<const char*>();
		case Json::ValueType::booleanValue:
			return rttr::Reflect<bool>();
		case Json::ValueType::intValue:
			return rttr::Reflect<int64_t>();
		case Json::ValueType::uintValue:
			return rttr::Reflect<uint64_t>();
		case Json::ValueType::realValue:
			return rttr::Reflect<double>();
		case Json::ValueType::arrayValue:
			return rttr::Reflect<nullptr_t>();
		case Json::ValueType::objectValue:
		{
			if (jsonVal.isMember(k_typeId))
			{
				std::string typeId = jsonVal[k_typeId].asString();
				if (typeId == k_smartptrTypeKey)
				{
					return rttr::Reflect(jsonVal[k_sysTypeId].asCString());
				}
				else
				{
					return rttr::Reflect(typeId.c_str());
				}
			}
			else
			{
				return rttr::Reflect<nullptr_t>();
			}
		}
		case Json::ValueType::nullValue:
		default:
			return rttr::Reflect<nullptr_t>();
	}
}

} // namespace rs
