#pragma once

#include "../Core/CoreTypes.h"
#include <d3d12.h>
#include <vector>
#include <functional>
#include <unordered_map>
#include <string>
#include <optional>
#include <stdexcept>

namespace UnoEngine::Renderer
{
    using namespace UnoEngine::Core;

    // ========================================
    // Resource Description
    // ========================================

    enum class ResourceType
    {
        Buffer,
        Texture2D,
        Texture3D,
        TextureCube
    };

    enum class ResourceUsage
    {
        None,
        RenderTarget,
        DepthStencil,
        ShaderResource,
        UnorderedAccess,
        ConstantBuffer
    };

    struct ResourceDescriptor
    {
        String name;
        ResourceType type{ ResourceType::Texture2D };
        ResourceUsage usage{ ResourceUsage::ShaderResource };
        DXGI_FORMAT format{ DXGI_FORMAT_R8G8B8A8_UNORM };
        uint32 width{ 0 };
        uint32 height{ 0 };
        uint32 depth{ 1 };
        uint32 mipLevels{ 1 };
        uint32 arraySize{ 1 };
    };

    // ========================================
    // Render Pass
    // ========================================

    class RenderPass;

    using RenderPassExecuteCallback = std::function<void(ID3D12GraphicsCommandList*)>;

    struct RenderPassDescriptor
    {
        String name;
        std::vector<String> inputResources;
        std::vector<String> outputResources;
        RenderPassExecuteCallback executeCallback;
    };

    class RenderPass
    {
    public:
        explicit RenderPass(const RenderPassDescriptor& descriptor)
            : m_descriptor(descriptor)
        {
        }

        [[nodiscard]] auto GetName() const noexcept -> const String&
        {
            return m_descriptor.name;
        }

        [[nodiscard]] auto GetInputResources() const noexcept -> const std::vector<String>&
        {
            return m_descriptor.inputResources;
        }

        [[nodiscard]] auto GetOutputResources() const noexcept -> const std::vector<String>&
        {
            return m_descriptor.outputResources;
        }

        auto Execute(ID3D12GraphicsCommandList* commandList) const -> void
        {
            if (m_descriptor.executeCallback)
            {
                m_descriptor.executeCallback(commandList);
            }
        }

        [[nodiscard]] auto IsValid() const noexcept -> bool
        {
            return !m_descriptor.name.empty() && m_descriptor.executeCallback != nullptr;
        }

    private:
        RenderPassDescriptor m_descriptor;
    };

    // ========================================
    // Render Resource
    // ========================================

    class RenderResource
    {
    public:
        explicit RenderResource(const ResourceDescriptor& descriptor)
            : m_descriptor(descriptor)
        {
        }

        [[nodiscard]] auto GetName() const noexcept -> const String&
        {
            return m_descriptor.name;
        }

        [[nodiscard]] auto GetDescriptor() const noexcept -> const ResourceDescriptor&
        {
            return m_descriptor;
        }

        auto SetD3DResource(ID3D12Resource* resource) noexcept -> void
        {
            m_d3dResource = resource;
        }

        [[nodiscard]] auto GetD3DResource() const noexcept -> ID3D12Resource*
        {
            return m_d3dResource;
        }

        auto SetCurrentState(D3D12_RESOURCE_STATES state) noexcept -> void
        {
            m_currentState = state;
        }

        [[nodiscard]] auto GetCurrentState() const noexcept -> D3D12_RESOURCE_STATES
        {
            return m_currentState;
        }

    private:
        ResourceDescriptor m_descriptor;
        ID3D12Resource* m_d3dResource{ nullptr };
        D3D12_RESOURCE_STATES m_currentState{ D3D12_RESOURCE_STATE_COMMON };
    };

    // ========================================
    // Render Graph
    // ========================================

    class RenderGraph
    {
    public:
        RenderGraph() = default;
        ~RenderGraph() = default;

        // Non-copyable, but movable
        RenderGraph(const RenderGraph&) = delete;
        auto operator=(const RenderGraph&) -> RenderGraph& = delete;
        RenderGraph(RenderGraph&&) noexcept = default;
        auto operator=(RenderGraph&&) noexcept -> RenderGraph& = default;

        // ========================================
        // Resource Management
        // ========================================

        auto DeclareResource(const ResourceDescriptor& descriptor) -> void
        {
            if (m_resources.contains(descriptor.name))
            {
                throw std::runtime_error("Resource already declared: " + descriptor.name);
            }

            m_resources[descriptor.name] = MakeUnique<RenderResource>(descriptor);
        }

        [[nodiscard]] auto GetResource(const String& name) -> RenderResource*
        {
            auto it = m_resources.find(name);
            if (it == m_resources.end())
            {
                return nullptr;
            }
            return it->second.get();
        }

        // ========================================
        // Pass Management
        // ========================================

        auto AddPass(const RenderPassDescriptor& descriptor) -> void
        {
            if (descriptor.name.empty())
            {
                throw std::invalid_argument("Render pass name cannot be empty");
            }

            m_passes.push_back(MakeUnique<RenderPass>(descriptor));
        }

        // ========================================
        // Compilation and Execution
        // ========================================

        auto Compile(ID3D12Device* device) -> bool
        {
            // Validate all passes
            for (const auto& pass : m_passes)
            {
                if (!pass->IsValid())
                {
                    return false;
                }

                // Validate input resources exist
                for (const auto& inputResource : pass->GetInputResources())
                {
                    if (!m_resources.contains(inputResource))
                    {
                        return false;
                    }
                }

                // Validate output resources exist
                for (const auto& outputResource : pass->GetOutputResources())
                {
                    if (!m_resources.contains(outputResource))
                    {
                        return false;
                    }
                }
            }

            // TODO: Implement dependency sorting (topological sort)
            // TODO: Implement resource barrier optimization
            // TODO: Implement resource aliasing

            m_isCompiled = true;
            return true;
        }

        auto Execute(ID3D12GraphicsCommandList* commandList) -> void
        {
            if (!m_isCompiled)
            {
                throw std::runtime_error("Render graph must be compiled before execution");
            }

            for (const auto& pass : m_passes)
            {
                // Insert resource barriers before pass execution
                InsertResourceBarriers(commandList, pass.get());

                // Execute pass
                pass->Execute(commandList);
            }
        }

        auto Reset() -> void
        {
            m_passes.clear();
            m_resources.clear();
            m_isCompiled = false;
        }

    private:
        // ========================================
        // Helper Methods
        // ========================================

        auto InsertResourceBarriers(ID3D12GraphicsCommandList* commandList, RenderPass* pass) -> void
        {
            std::vector<D3D12_RESOURCE_BARRIER> barriers;

            // Transition input resources to appropriate states
            for (const auto& inputName : pass->GetInputResources())
            {
                auto resource = m_resources[inputName].get();
                if (resource->GetCurrentState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
                {
                    D3D12_RESOURCE_BARRIER barrier{};
                    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                    barrier.Transition.pResource = resource->GetD3DResource();
                    barrier.Transition.StateBefore = resource->GetCurrentState();
                    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                    barriers.push_back(barrier);
                    resource->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                }
            }

            // Transition output resources to appropriate states
            for (const auto& outputName : pass->GetOutputResources())
            {
                auto resource = m_resources[outputName].get();
                D3D12_RESOURCE_STATES targetState = D3D12_RESOURCE_STATE_RENDER_TARGET;

                if (resource->GetDescriptor().usage == ResourceUsage::DepthStencil)
                {
                    targetState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
                }
                else if (resource->GetDescriptor().usage == ResourceUsage::UnorderedAccess)
                {
                    targetState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                }

                if (resource->GetCurrentState() != targetState)
                {
                    D3D12_RESOURCE_BARRIER barrier{};
                    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                    barrier.Transition.pResource = resource->GetD3DResource();
                    barrier.Transition.StateBefore = resource->GetCurrentState();
                    barrier.Transition.StateAfter = targetState;
                    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                    barriers.push_back(barrier);
                    resource->SetCurrentState(targetState);
                }
            }

            // Execute barriers if any
            if (!barriers.empty())
            {
                commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
            }
        }

        // ========================================
        // Member Variables
        // ========================================

        std::vector<UniquePtr<RenderPass>> m_passes;
        std::unordered_map<String, UniquePtr<RenderResource>> m_resources;
        bool m_isCompiled{ false };
    };

} // namespace UnoEngine::Renderer
