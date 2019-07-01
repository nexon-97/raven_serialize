#pragma once
#include <vector>
#include <memory>
#include <string>

#include "rttr/Property.hpp"

namespace rs
{

class ContextPath
{
public:
	enum class ActionType
	{
		ObjectProperty,
		ArrayItem,
	};

	struct PathItem
	{
		ActionType actionType;
		std::unique_ptr<std::string> propertyName;
		std::size_t arrayIndex = 0U;

		PathItem(const ActionType actionType);

		PathItem(const PathItem& other);
		PathItem& operator=(const PathItem& other);
	};

	struct PropertyData
	{
		rttr::Property* property = nullptr;
		void* object = nullptr;
	};

	ContextPath() = default;
	ContextPath(std::vector<PathItem>&& pathItems);

	ContextPath(const ContextPath& other);
	ContextPath& operator=(const ContextPath& other);

	void PushObjectPropertyAction(std::string&& propertyName);
	void PushArrayItemAction(const std::size_t arrayIndex);

	void PopAction();
	const PathItem* GetTopAction() const;
	const std::vector<PathItem>& GetActions() const;

	std::size_t GetSize() const;

	PropertyData ResolvePropertyData(const rttr::Type& rootType, void* contextRoot) const;

private:
	std::vector<PathItem> m_pathItems;
};

} // namespace rs
