#pragma once

namespace rttr
{

class ProxyConverterBase
{
public:
	virtual ~ProxyConverterBase() = default;
	virtual void Convert(void* target, const void* source) = 0;
};

template <typename T, typename U>
class ConstructorProxyConverter
	: public ProxyConverterBase
{
	using RawU = typename std::remove_reference_t<U>;

public:
	void Convert(void* target, const void* source) override
	{
		const RawU* typedProxy = static_cast<const RawU*>(source);
		T* typedTarget = static_cast<T*>(target);
		*typedTarget = T(*typedProxy);
	}
};

template <typename MemFnSignature>
class MemberFuncProxyConverter
	: public ProxyConverterBase
{
	static_assert(IsMemberFuncPrototype<MemFnSignature>::value, "Template parameter must be member function prototype!");

	using T = typename ExtractClassType<MemFnSignature>::type;
	using RawU = typename ExtractValueType<MemFnSignature>::type;

public:
	explicit MemberFuncProxyConverter(MemFnSignature memFunPtr)
		: m_memFunPtr(memFunPtr)
	{}

	void Convert(void* target, const void* source) override
	{
		const T* typedSource = static_cast<const T*>(source);
		RawU* typedTargetPtr = static_cast<RawU*>(target);
		*typedTargetPtr = std::invoke(m_memFunPtr, typedSource);
	}

private:
	MemFnSignature m_memFunPtr;
};

}
