#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <unordered_set>
#include <vector>

using ComponentId = uint32_t;

ComponentId GenerateNewComponentId()
{
	static std::atomic<ComponentId> id{ 0 };
	return id++;
}

template <typename T>
ComponentId GetTypeId()
{
	const static ComponentId id = GenerateNewComponentId();
	return id;
}

struct ComponentType
{
	ComponentId id;
	uint64_t size;
};

using Column = std::vector<uint8_t>;

struct CompositeType
{
	std::vector<ComponentType> types;

	bool IsSupersetOf(const CompositeType& other)
	{
		for (const ComponentType& type : other.types) {
			bool containsType = std::any_of(types.begin(), types.end(), [&](const ComponentType& t) {
				return t.id == type.id;
			});

			if (!containsType) {
				return false;
			}
		}

		return true;
	}

	int IndexOf(ComponentId componentId)
	{
		for (int i = 0; i < types.size(); i++) {
			if (types[i].id == componentId) {
				return i;
			}
		}
		return -1;
	}

	bool Equal(const CompositeType& other)
	{
		return (types.size() == other.types.size()) && IsSupersetOf(other);
	}
};

template <typename... Components>
CompositeType GetCompositeType()
{
	CompositeType result;

	([&] {
		ComponentType type;
		type.id = GetTypeId<std::remove_reference_t<Components>>();
		type.size = sizeof(std::remove_reference_t<Components>);
		result.types.push_back(type);
	}(),
		...);

	return result;
}

struct Archtype
{
	std::vector<Column> columns;
	CompositeType type;
	int rowsCount;

	template <typename... Components>
	void AddRow(Components&&... components)
	{
		([&] {
			ComponentId columnId = GetTypeId<Components>();
			int idx = type.IndexOf(columnId);

			columns[idx].resize(columns[idx].size() + sizeof(Components));
			Components* buf = reinterpret_cast<Components*>(columns[idx].data());
			buf[rowsCount] = components;
		}(),
			...);

		rowsCount++;
	}
};

struct Entity
{
	uint32_t id;
	uint32_t archtypeId;
	uint32_t row;
	CompositeType type;
};

struct World
{
	std::vector<Entity> entities;
	std::vector<Archtype> archtypes;

	template <typename... Components>
	int CreateEntity(Components&&... components)
	{
		static uint32_t id{ 0 };
		Entity entity;
		entity.id = id++;
		entity.type = GetCompositeType<Components...>();

		AddToArchtypes(entity, std::forward<Components>(components)...);

		entities.push_back(entity);

		return entity.id;
	}

	Archtype& FindOfCreateArchtype(const CompositeType& type)
	{
		auto archtype = std::find_if(archtypes.begin(), archtypes.end(), [&](Archtype& a) {
			return a.type.Equal(type);
		});

		if (archtype != archtypes.end()) {
			return *archtype;
		}

		Archtype newArchtype;
		newArchtype.columns.resize(type.types.size());
		newArchtype.type = type;
		newArchtype.rowsCount = 0;
		archtypes.emplace_back(newArchtype);
		return archtypes.back();
	}

	template <typename... Components>
	void AddToArchtypes(Entity& entity, Components&&... components)
	{
		Archtype& archtype = FindOfCreateArchtype(entity.type);
		entity.row = archtype.rowsCount;
		archtype.AddRow(std::forward<std::remove_reference_t<Components>>(components)...);
	}

	template <typename Tuple, typename Func, size_t... I>
	void for_each_impl(Func f, Tuple t, int rows_count, std::index_sequence<I...>)
	{
		for (int i = 0; i < rows_count; i++) {
			f(std::get<I>(t)[i]...);
		}
	}

	template <typename... Components, typename Func>
	void for_each(Func f)
	{
		CompositeType type = GetCompositeType<Components...>();

		for (Archtype& a : archtypes) {
			if (a.type.IsSupersetOf(type)) {
				std::vector<int> componentIndices(type.types.size());

				for (int i = 0; i < componentIndices.size(); i++) {
					componentIndices[i] = a.type.IndexOf(type.types[i].id);
				}

				auto selectedColumns = std::make_tuple(
					(Components*)a.columns[a.type.IndexOf(GetTypeId<Components>())].data()...);

				static constexpr auto size = std::tuple_size<decltype(selectedColumns)>::value;

				for_each_impl(f, selectedColumns, a.rowsCount, std::make_index_sequence<size>{});
			}
		}
	}
};
