#pragma once

#include <entt/entt.hpp>
#include <string_view>

struct System {
    entt::registry &m_registry;
    const std::string_view m_system_name;

    constexpr System(entt::registry &registry, const std::string_view &system_name) :
            m_registry(registry), m_system_name(system_name)
    {
    }

    virtual ~System() = default;
    virtual void Run() = 0;
};
