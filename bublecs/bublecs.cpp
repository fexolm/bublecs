#include "bublecs.h"

ecs::ECSId ecs::GenerateNewArchetypeId()
{
	static std::atomic<ecs::ECSId> id{ 0 };
	return id++;
}

ecs::ECSId ecs::GenerateNewComponentId()
{
	static std::atomic<ecs::ECSId> id{ 0 };
	return id++;
}

ecs::ECSId ecs::GenerateNewEntityId()
{
	static std::atomic<ECSId> id{ 0 };
	return id++;
}
