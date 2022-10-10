#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <exception>
#include <iostream>
#include <unordered_set>
#include <vector>

namespace ecs {
using ECSId = uint32_t;
using Column = std::vector<uint8_t>;

ECSId GenerateNewArchetypeId();
ECSId GenerateNewColumnId();
ECSId GenerateNewComponentId();
ECSId GenerateNewEntityId();

template <typename T>
ECSId GetTypeId()
{
    const static ECSId id = GenerateNewComponentId();
    return id;
}

template <typename CompositeType, typename Component>
ECSId GetColumnId()
{
    const static ECSId id = GenerateNewColumnId();
    return id;
}

struct ComponentType
{
    ECSId id;
    uint64_t size;
};

class CompositeType
{
 private:
    std::unordered_map<ECSId, ComponentType> types;
    std::unordered_map<ECSId, ECSId> cols_to_types;
    std::unordered_map<ECSId, ECSId> types_to_cols;

 public:
    CompositeType() = default;
    CompositeType(std::unordered_map<ECSId, ComponentType> types) : types(types) {
        ECSId column_id = 0;
        for (auto& [type_id, type] : types) {
            cols_to_types.emplace(column_id, type_id);
            types_to_cols.emplace(type_id, column_id);
            ++column_id;
        }
    }

    size_t Empty() const
    {
        return types.empty();
    }

    size_t Size() const
    {
        return types.size();
    }

    uint64_t GetTypeSize(ECSId id) const
    {
        return types.at(id).size;
    }

    ComponentType GetType(ECSId id) const
    {
        return types.at(id);
    }

    bool IsSupersetOf(const CompositeType& other)
    {
        for (const auto& [id, type] : other.types) {
            if (types.count(id) == 0) return false;
        }

        return true;
    }

    int GetColumnId(ECSId component_id)
    {
        if (types.count(component_id) > 0) {
            return types_to_cols.at(component_id);
        }
        return -1;
    }

    int GetTypeIdx(ECSId column_id)
    {
        if (cols_to_types.count(column_id) > 0) {
            return cols_to_types.at(column_id);
        }
        return -1;
    }

    bool Equal(const CompositeType& other)
    {
        return (types.size() == other.types.size()) && IsSupersetOf(other);
    }

    void RemoveComponent(ECSId component_id)
    {
        types.erase(component_id);
        auto col_id = types_to_cols.at(component_id);
        types_to_cols.erase(component_id);
        cols_to_types.erase(col_id);
    }
};

template <typename... Components>
CompositeType GetCompositeType()
{
    //CompositeType result;
    std::unordered_map<ECSId, ComponentType> types;
    ([&] {
        ComponentType type;
        type.id = GetTypeId<std::remove_reference_t<Components>>();
        type.size = sizeof(std::remove_reference_t<Components>);
        types.emplace(type.id, type);
    }(),
        ...);

    return CompositeType(types);
}

struct Archetype
{
    std::vector<Column> columns;
    CompositeType type;
    int rowsCount = 0;

    template <typename... Components>
    void AddRow(Components&&... components)
    {
        ([&] {
            ECSId component_id = GetTypeId<Components>();
            int idx = type.GetColumnId(component_id);

            columns[idx].resize(columns[idx].size() + sizeof(Components));
            Components* buf = reinterpret_cast<Components*>(columns[idx].data());
            buf[rowsCount] = components;
        }(),
            ...);

        ++rowsCount;
    }

    void SetColumns(std::vector<Column> other_cols, int rows)
    {
        columns = other_cols;
        ++rowsCount;
    }
};

struct Entity
{
    uint32_t id;
    uint32_t archetype_id;
    uint32_t row;
    CompositeType type;
};

struct World
{
    std::unordered_map<ECSId, Entity> entities;
    std::unordered_map<ECSId, Archetype> archetypes;

    template <typename... Components>
    ECSId CreateEntity(Components&&... components)
    {
        Entity entity;
        entity.id = GenerateNewEntityId();
        entity.type = GetCompositeType<Components...>();

        AddToArchetypes(entity, std::forward<Components>(components)...);

        entities.emplace(entity.id, entity);

        return entity.id;

    }

    std::pair<ECSId, Archetype&> FindOrCreateArchtype(const CompositeType& type)
    {
        auto archetype = std::find_if(archetypes.begin(), archetypes.end(), [&](auto& p) {
            auto& [id, arch] = p;
            return  arch.type.Equal(type);
        });

        if (archetype != archetypes.end()) {
            auto [id, arch] = *archetype;
            return {id, arch};
        }

        Archetype newArchtype;
        newArchtype.columns.resize(type.Size());
        newArchtype.type = type;
        newArchtype.rowsCount = 0;
        ECSId arhc_id = GenerateNewArchetypeId();
        archetypes.emplace(arhc_id, newArchtype);
        return {arhc_id, archetypes[arhc_id]};
    }

    template <typename... Components>
    void AddToArchetypes(Entity& entity, Components&&... components)
    {
        auto [id, archtype] = FindOrCreateArchtype(entity.type);
        entity.archetype_id = id;
        entity.row = archtype.rowsCount;
        archtype.AddRow(std::forward<std::remove_reference_t<Components>>(components)...);
    }

    template <typename... Components>
    void RemoveComponents(ECSId entity_id)
    {
        if (!entities.count(entity_id)) return;

        Entity& entity = entities.at(entity_id);
        //entities_to_delete.push_back(entity);
        Archetype arch = archetypes[entity.archetype_id];
        CompositeType new_cmp_type = arch.type;

        // fill components ids to delete
        std::unordered_map<ECSId, ComponentType> components_to_delete;
        ([&] {
            ECSId type_id = GetTypeId<Components>();
            components_to_delete.emplace(type_id, arch.type.GetType(type_id));
            new_cmp_type.RemoveComponent(type_id);
        }(),
        ...);

        // copy row from archtype to tmp var without components to delete
        std::vector<Column> new_columns;
        for (size_t i = 0; i < arch.columns.size(); ++i ) {
            ECSId type_id = arch.type.GetTypeIdx(i);
            auto start = arch.columns[i].begin() + entity.row;
            auto cmp_size = arch.type.GetTypeSize(type_id);

            bool leave = components_to_delete.count(type_id) == 0; // if not in components to delete
            if (leave) {
                new_columns.push_back({start, start + cmp_size});
            }

            // move row to delete to the end
            for (size_t j = 0; j < cmp_size; ++j) {
                std::swap(*(start + j), *(arch.columns[i].end() - cmp_size + j));
            }

            arch.columns[i].resize(arch.columns[i].size() - cmp_size);
            // delete acrhetype if is empty
            bool arch_is_empty = std::all_of(arch.columns.begin(), arch.columns.end(), [](const Column& c) {
                return c.empty();
            });
            if (arch_is_empty) {
                archetypes.erase(entity.archetype_id);
            }
        }

        // create of find archtype according with new composite type
        if (!new_cmp_type.Empty()) {
            auto [id, archetype] = FindOrCreateArchtype(new_cmp_type);
            entity.type = new_cmp_type;
            entity.archetype_id = id;
            entity.row = archetype.rowsCount;
            archetype.SetColumns(new_columns, entity.row);
        } else {
            entities.erase(entity_id);
        }
    }

    template <typename Tuple, typename Func, size_t... I>
    void ForEachImpl(Func f, Tuple t, int rows_count, std::index_sequence<I...>)
    {
        for (int i = 0; i < rows_count; i++) {
            f(std::get<I>(t)[i]...);
        }
    }

    template <typename... Components, typename Func>
    void ForEach(Func f)
    {
        CompositeType type = GetCompositeType<Components...>();

        for (auto& [id, a] : archetypes) {
            if (a.type.IsSupersetOf(type)) {
                auto selectedColumns = std::make_tuple(
                    (Components*)a.columns[a.type.GetColumnId(GetTypeId<Components>())].data()...);

                static constexpr auto size = std::tuple_size<decltype(selectedColumns)>::value;

                ForEachImpl(f, selectedColumns, a.rowsCount, std::make_index_sequence<size>{});
            }
        }
    }
};
}  // cse
