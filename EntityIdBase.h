//
// Created by Mika Goetze on 20/9/2026.
//

#ifndef ARCHON_ENTITYID_H
#define ARCHON_ENTITYID_H
#include <cstdint>
#include <type_traits>

namespace Archon
{
    using IndexType = uint16_t;
    using VersionType = uint8_t;
    using EntityType = uint16_t;

    class EntityIdBase
    {
    private:
        static constexpr IndexType IndexBits = 12;
        static constexpr IndexType InvalidIndex = 0;
        static constexpr IndexType FirstIndex = 1;

        static constexpr VersionType VersionBits = 4;
        static constexpr VersionType InvalidVersion = 0;
        static constexpr VersionType FirstVersion = 1;

        static_assert(sizeof(EntityType) <= (IndexBits + VersionBits) / 8);

        union
        {
            EntityType m_id;

            struct
            {
                IndexType m_index : IndexBits;
                VersionType m_version : VersionBits;
            };
        };

    public:
        EntityIdBase() : m_index(InvalidIndex), m_version(InvalidVersion)
        {
        }

        EntityIdBase(IndexType index) : m_index(index), m_version(FirstVersion)
        {
        }

        EntityIdBase(IndexType index, VersionType version) : m_index(index), m_version(version)
        {
        }

        bool operator==(const EntityIdBase&) const;
        bool operator!=(const EntityIdBase&) const;

        void IncrementVersion();
        [[nodiscard]] EntityType GetUnderlyingId() const;
    };

    template <typename... ComponentTypes>
    class EntityId : protected EntityIdBase
    {
    public:
        template <typename ComponentType>
            requires(std::is_same_v<ComponentType, ComponentTypes> || ...)
        const ComponentType& Get() const;
    };

    template <typename... ComponentTypes>
    template <typename ComponentType> requires (std::is_same_v<ComponentType, ComponentTypes> || ...)
    const ComponentType& EntityId<ComponentTypes...>::Get() const
    {
        // TODO: Hook up to EntityManager...
        return nullptr;
    }
}
#endif //ARCHON_ENTITYID_H
