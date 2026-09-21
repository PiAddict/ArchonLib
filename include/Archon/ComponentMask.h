#ifndef ARCHON_COMPONENTMASK_H
#define ARCHON_COMPONENTMASK_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include <Archon/ComponentRegistry.h>

namespace Archon
{
    class ComponentMask
    {
        std::vector<uint64_t> m_mask;

    public:
        ComponentMask() = default;

        template<typename ... ComponentTypes>
        static ComponentMask Create();

        void Set(ComponentId id);
        [[nodiscard]] bool Test(ComponentId id) const;

        template<typename ComponentType>
        void Set();

        template<typename ComponentType>
        [[nodiscard]] bool Test() const;

        void Clear(ComponentId id);
        [[nodiscard]] uint64_t GetHash() const;

        void Clear();
        bool operator==(const ComponentMask&) const;

        void ForEachComponentId(const std::function<void(ComponentId)>& callback) const;
    };

    template <typename ... ComponentTypes>
    ComponentMask ComponentMask::Create()
    {
        ComponentMask componentMask;
        (componentMask.Set<ComponentTypes>(), ...);
        return componentMask;
    }

    template <typename ComponentType>
    void ComponentMask::Set()
    {
        Set(ComponentRegistry::GetId<ComponentType>());
    }

    template <typename ComponentType>
    bool ComponentMask::Test() const
    {
        return Test(ComponentRegistry::GetId<ComponentType>());
    }
}

template<>
struct std::hash<Archon::ComponentMask>
{
    size_t operator()(const Archon::ComponentMask& componentMask) const noexcept
    {
        return static_cast<size_t>(componentMask.GetHash());
    }
};

#endif // ARCHON_COMPONENTMASK_H
