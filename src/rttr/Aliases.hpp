#pragma once

namespace rttr
{

template <class ClassType, typename ValueType>
using AccessorMethod = const ValueType& (ClassType::*)() const;
template <class ClassType, typename ValueType>
using AccessorMethodByValue = ValueType (ClassType::*)() const;
template <class ClassType, typename ValueType>
using AccessorMethodByValueNonConst = ValueType(ClassType::*)();
template <class ClassType, typename ValueType>
using AccessorMethodNonConst = const ValueType& (ClassType::*)();
template <class ClassType, typename ValueType>
using MutatorMethod = void(ClassType::*)(const ValueType&);
template <class ClassType, typename ValueType>
using MutatorMethodByValue = void(ClassType::*)(ValueType);
template <class ClassType, typename ValueType>
using MemberSignature = ValueType ClassType::*;

} // namespace rttr
