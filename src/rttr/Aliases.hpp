#pragma once

namespace rttr
{

template <class ClassType, typename ValueType>
using AccessorMethod = const ValueType& (ClassType::*)() const;
template <class ClassType, typename ValueType>
using MutatorMethod = void(ClassType::*)(const ValueType&);
template <class ClassType, typename ValueType>
using MemberSignature = ValueType ClassType::*;

} // namespace rttr
