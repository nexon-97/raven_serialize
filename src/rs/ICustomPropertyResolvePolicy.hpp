#pragma once

namespace rs
{

class ICustomPropertyResolvePolicy
{
public:
	virtual void Serialize(const void* object) = 0;
	virtual void Deserialize(void* object) = 0;
};

}
