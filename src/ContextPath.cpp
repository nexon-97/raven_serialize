#include "ContextPath.hpp"

namespace rs
{

ContextPath::PathItem::PathItem(const ActionType actionType)
	: actionType(actionType)
{}

ContextPath::PathItem::PathItem(const PathItem& other)
	: actionType(other.actionType)
	, arrayIndex(other.arrayIndex)
{
	if (actionType == ActionType::ObjectProperty)
	{
		propertyName.reset(new std::string(*other.propertyName));
	}
}

ContextPath::PathItem& ContextPath::PathItem::operator=(const PathItem& other)
{
	actionType = other.actionType;
	arrayIndex = other.arrayIndex;

	if (actionType == ActionType::ObjectProperty)
	{
		propertyName.reset(new std::string(*other.propertyName));
	}

	return *this;
}

///////////////////////////////////////////////////////////////////////////////

ContextPath::ContextPath(std::vector<PathItem>&& pathItems)
	: m_pathItems(std::move(pathItems))
{}

ContextPath::ContextPath(const ContextPath& other)
	: m_pathItems(other.m_pathItems)
{}

ContextPath& ContextPath::operator=(const ContextPath& other)
{
	m_pathItems = other.m_pathItems;

	return *this;
}

void ContextPath::PushObjectPropertyAction(std::string&& propertyName)
{
	auto& item = m_pathItems.emplace_back(ActionType::ObjectProperty);
	item.propertyName.reset(new std::string(std::move(propertyName)));
}

void ContextPath::PushArrayItemAction(const std::size_t arrayIndex)
{
	auto& item = m_pathItems.emplace_back(ActionType::ArrayItem);
	item.arrayIndex = arrayIndex;
}

void ContextPath::PopAction()
{
	if (!m_pathItems.empty())
	{
		m_pathItems.pop_back();
	}
}

const ContextPath::PathItem* ContextPath::GetTopAction() const
{
	if (!m_pathItems.empty())
	{
		return &m_pathItems.back();
	}
	else
	{
		return nullptr;
	}
}

const std::vector<ContextPath::PathItem>& ContextPath::GetActions() const
{
	return m_pathItems;
}

std::size_t ContextPath::GetSize() const
{
	return m_pathItems.size();
}

ContextPath::PropertyData ContextPath::ResolvePropertyData(const rttr::Type& rootType, void* contextRoot) const
{
	PropertyData result;

	const PathItem* topAction = GetTopAction();
	if (nullptr != topAction && topAction->actionType == ActionType::ObjectProperty)
	{
		void* currentObjectPtr = contextRoot;
		rttr::Type currentObjectType = rootType;

		for (const PathItem& item : m_pathItems)
		{
			switch (item.actionType)
			{
				case ActionType::ObjectProperty:
					rttr::Property* objectProperty = currentObjectType.FindProperty(*item.propertyName);
					if (nullptr != objectProperty)
					{

					}
					break;
			}
		}

		result.object = currentObjectPtr;
	}

	return result;
}

} // namespace rs
