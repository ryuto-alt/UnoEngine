#pragma once

#include "CoreTypes.h"
#include <array>
#include <bitset>
#include <queue>
#include <unordered_map>
#include <set>
#include <stdexcept>

namespace UnoEngine::Core::ECS
{
    // ========================================
    // Entity Component System Configuration
    // ========================================

    constexpr uint32 MaxEntities   = 5000;
    constexpr uint32 MaxComponents = 32;

    // ========================================
    // Entity Definition
    // ========================================

    // Entity is simply a unique ID
    using Entity = uint32;

    // Component type identifier
    using ComponentType = uint8;

    // Signature to identify which components an entity has
    using Signature = std::bitset<MaxComponents>;

    // ========================================
    // Entity Manager
    // ========================================

    class EntityManager
    {
    public:
        EntityManager()
        {
            // Initialize the queue with all possible entity IDs
            for (Entity entity = 0; entity < MaxEntities; ++entity)
            {
                m_availableEntities.push(entity);
            }
        }

        [[nodiscard]] auto CreateEntity() -> Entity
        {
            if (m_livingEntityCount >= MaxEntities)
            {
                throw std::runtime_error("Entity limit reached. Cannot create more entities.");
            }

            Entity id = m_availableEntities.front();
            m_availableEntities.pop();
            ++m_livingEntityCount;

            return id;
        }

        auto DestroyEntity(Entity entity) -> void
        {
            if (entity >= MaxEntities)
            {
                throw std::out_of_range("Entity ID out of range.");
            }

            // Reset the signature for this entity
            m_signatures[entity].reset();

            // Return the entity ID to the available pool
            m_availableEntities.push(entity);
            --m_livingEntityCount;
        }

        auto SetSignature(Entity entity, Signature signature) -> void
        {
            if (entity >= MaxEntities)
            {
                throw std::out_of_range("Entity ID out of range.");
            }

            m_signatures[entity] = signature;
        }

        [[nodiscard]] auto GetSignature(Entity entity) const -> Signature
        {
            if (entity >= MaxEntities)
            {
                throw std::out_of_range("Entity ID out of range.");
            }

            return m_signatures[entity];
        }

        [[nodiscard]] auto GetLivingEntityCount() const noexcept -> uint32
        {
            return m_livingEntityCount;
        }

    private:
        std::queue<Entity> m_availableEntities{};
        std::array<Signature, MaxEntities> m_signatures{};
        uint32 m_livingEntityCount{ 0 };
    };

    // ========================================
    // Component Array Interface
    // ========================================

    class IComponentArray
    {
    public:
        virtual ~IComponentArray() = default;
        virtual auto OnEntityDestroyed(Entity entity) -> void = 0;
    };

    // ========================================
    // Component Array Implementation
    // ========================================

    template<typename T>
    class ComponentArray : public IComponentArray
    {
    public:
        auto InsertData(Entity entity, T component) -> void
        {
            if (m_entityToIndexMap.contains(entity))
            {
                throw std::runtime_error("Component already exists for this entity.");
            }

            // Put new entry at end and update the maps
            size_t newIndex = m_size;
            m_entityToIndexMap[entity] = newIndex;
            m_indexToEntityMap[newIndex] = entity;
            m_componentArray[newIndex] = component;
            ++m_size;
        }

        auto RemoveData(Entity entity) -> void
        {
            if (!m_entityToIndexMap.contains(entity))
            {
                throw std::runtime_error("Component does not exist for this entity.");
            }

            // Copy element at end into deleted element's place to maintain density
            size_t indexOfRemovedEntity = m_entityToIndexMap[entity];
            size_t indexOfLastElement = m_size - 1;
            m_componentArray[indexOfRemovedEntity] = m_componentArray[indexOfLastElement];

            // Update map to point to moved spot
            Entity entityOfLastElement = m_indexToEntityMap[indexOfLastElement];
            m_entityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
            m_indexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

            m_entityToIndexMap.erase(entity);
            m_indexToEntityMap.erase(indexOfLastElement);

            --m_size;
        }

        [[nodiscard]] auto GetData(Entity entity) -> T&
        {
            if (!m_entityToIndexMap.contains(entity))
            {
                throw std::runtime_error("Component does not exist for this entity.");
            }

            return m_componentArray[m_entityToIndexMap[entity]];
        }

        [[nodiscard]] auto GetData(Entity entity) const -> const T&
        {
            if (!m_entityToIndexMap.contains(entity))
            {
                throw std::runtime_error("Component does not exist for this entity.");
            }

            return m_componentArray[m_entityToIndexMap.at(entity)];
        }

        auto OnEntityDestroyed(Entity entity) -> void override
        {
            if (m_entityToIndexMap.contains(entity))
            {
                RemoveData(entity);
            }
        }

    private:
        // Packed array of components (to maintain cache efficiency)
        std::array<T, MaxEntities> m_componentArray{};

        // Map from entity ID to array index
        std::unordered_map<Entity, size_t> m_entityToIndexMap{};

        // Map from array index to entity ID
        std::unordered_map<size_t, Entity> m_indexToEntityMap{};

        // Total number of valid entries in the array
        size_t m_size{ 0 };
    };

    // ========================================
    // Component Manager
    // ========================================

    class ComponentManager
    {
    public:
        template<typename T>
        auto RegisterComponent() -> void
        {
            const char* typeName = typeid(T).name();

            if (m_componentTypes.contains(typeName))
            {
                throw std::runtime_error("Component type already registered.");
            }

            // Add this component type to the component type map
            m_componentTypes[typeName] = m_nextComponentType;

            // Create a ComponentArray pointer and add it to the component arrays map
            m_componentArrays[typeName] = MakeShared<ComponentArray<T>>();

            ++m_nextComponentType;
        }

        template<typename T>
        [[nodiscard]] auto GetComponentType() const -> ComponentType
        {
            const char* typeName = typeid(T).name();

            if (!m_componentTypes.contains(typeName))
            {
                throw std::runtime_error("Component type not registered before use.");
            }

            return m_componentTypes.at(typeName);
        }

        template<typename T>
        auto AddComponent(Entity entity, T component) -> void
        {
            GetComponentArray<T>()->InsertData(entity, component);
        }

        template<typename T>
        auto RemoveComponent(Entity entity) -> void
        {
            GetComponentArray<T>()->RemoveData(entity);
        }

        template<typename T>
        [[nodiscard]] auto GetComponent(Entity entity) -> T&
        {
            return GetComponentArray<T>()->GetData(entity);
        }

        template<typename T>
        [[nodiscard]] auto GetComponent(Entity entity) const -> const T&
        {
            return GetComponentArray<T>()->GetData(entity);
        }

        auto OnEntityDestroyed(Entity entity) -> void
        {
            // Notify each component array that an entity has been destroyed
            for (auto const& [typeName, componentArray] : m_componentArrays)
            {
                componentArray->OnEntityDestroyed(entity);
            }
        }

    private:
        // Map from type string pointer to a component type
        std::unordered_map<const char*, ComponentType> m_componentTypes{};

        // Map from type string pointer to a component array
        std::unordered_map<const char*, SharedPtr<IComponentArray>> m_componentArrays{};

        // The component type to be assigned to the next registered component
        ComponentType m_nextComponentType{ 0 };

        template<typename T>
        [[nodiscard]] auto GetComponentArray() const -> SharedPtr<ComponentArray<T>>
        {
            const char* typeName = typeid(T).name();

            if (!m_componentTypes.contains(typeName))
            {
                throw std::runtime_error("Component type not registered before use.");
            }

            return std::static_pointer_cast<ComponentArray<T>>(m_componentArrays.at(typeName));
        }
    };

    // ========================================
    // System Base Class
    // ========================================

    class System
    {
    public:
        std::set<Entity> m_entities;
    };

    // ========================================
    // System Manager
    // ========================================

    class SystemManager
    {
    public:
        template<typename T>
        [[nodiscard]] auto RegisterSystem() -> SharedPtr<T>
        {
            const char* typeName = typeid(T).name();

            if (m_systems.contains(typeName))
            {
                throw std::runtime_error("System already registered.");
            }

            auto system = MakeShared<T>();
            m_systems[typeName] = system;
            return system;
        }

        template<typename T>
        auto SetSignature(Signature signature) -> void
        {
            const char* typeName = typeid(T).name();

            if (!m_systems.contains(typeName))
            {
                throw std::runtime_error("System not registered before use.");
            }

            m_signatures[typeName] = signature;
        }

        auto OnEntityDestroyed(Entity entity) -> void
        {
            for (auto const& [typeName, system] : m_systems)
            {
                system->m_entities.erase(entity);
            }
        }

        auto OnEntitySignatureChanged(Entity entity, Signature entitySignature) -> void
        {
            for (auto const& [typeName, system] : m_systems)
            {
                auto const& systemSignature = m_signatures[typeName];

                // Entity signature matches system signature - insert into set
                if ((entitySignature & systemSignature) == systemSignature)
                {
                    system->m_entities.insert(entity);
                }
                // Entity signature does not match system signature - erase from set
                else
                {
                    system->m_entities.erase(entity);
                }
            }
        }

    private:
        // Map from system type string to a signature
        std::unordered_map<const char*, Signature> m_signatures{};

        // Map from system type string to a system pointer
        std::unordered_map<const char*, SharedPtr<System>> m_systems{};
    };

    // ========================================
    // Coordinator (Facade Pattern)
    // ========================================

    class Coordinator
    {
    public:
        auto Initialize() -> void
        {
            m_entityManager = MakeUnique<EntityManager>();
            m_componentManager = MakeUnique<ComponentManager>();
            m_systemManager = MakeUnique<SystemManager>();
        }

        // Entity methods
        [[nodiscard]] auto CreateEntity() -> Entity
        {
            return m_entityManager->CreateEntity();
        }

        auto DestroyEntity(Entity entity) -> void
        {
            m_entityManager->DestroyEntity(entity);
            m_componentManager->OnEntityDestroyed(entity);
            m_systemManager->OnEntityDestroyed(entity);
        }

        // Component methods
        template<typename T>
        auto RegisterComponent() -> void
        {
            m_componentManager->RegisterComponent<T>();
        }

        template<typename T>
        auto AddComponent(Entity entity, T component) -> void
        {
            m_componentManager->AddComponent<T>(entity, component);

            auto signature = m_entityManager->GetSignature(entity);
            signature.set(m_componentManager->GetComponentType<T>(), true);
            m_entityManager->SetSignature(entity, signature);

            m_systemManager->OnEntitySignatureChanged(entity, signature);
        }

        template<typename T>
        auto RemoveComponent(Entity entity) -> void
        {
            m_componentManager->RemoveComponent<T>(entity);

            auto signature = m_entityManager->GetSignature(entity);
            signature.set(m_componentManager->GetComponentType<T>(), false);
            m_entityManager->SetSignature(entity, signature);

            m_systemManager->OnEntitySignatureChanged(entity, signature);
        }

        template<typename T>
        [[nodiscard]] auto GetComponent(Entity entity) -> T&
        {
            return m_componentManager->GetComponent<T>(entity);
        }

        template<typename T>
        [[nodiscard]] auto GetComponent(Entity entity) const -> const T&
        {
            return m_componentManager->GetComponent<T>(entity);
        }

        template<typename T>
        [[nodiscard]] auto GetComponentType() const -> ComponentType
        {
            return m_componentManager->GetComponentType<T>();
        }

        // System methods
        template<typename T>
        [[nodiscard]] auto RegisterSystem() -> SharedPtr<T>
        {
            return m_systemManager->RegisterSystem<T>();
        }

        template<typename T>
        auto SetSystemSignature(Signature signature) -> void
        {
            m_systemManager->SetSignature<T>(signature);
        }

    private:
        UniquePtr<EntityManager> m_entityManager;
        UniquePtr<ComponentManager> m_componentManager;
        UniquePtr<SystemManager> m_systemManager;
    };

} // namespace UnoEngine::Core::ECS
