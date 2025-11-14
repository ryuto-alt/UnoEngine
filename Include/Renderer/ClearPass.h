#pragma once

#include "../Core/CoreTypes.h"
#include "RenderGraph.h"
#include "../Graphics/GraphicsDevice.h"
#include <d3d12.h>

namespace UnoEngine::Renderer
{
    using namespace UnoEngine::Core;
    using namespace UnoEngine::Graphics;

    // ========================================
    // Clear Pass
    // ========================================

    struct ClearPassConfig
    {
        float clearColor[4]{ 0.0f, 0.4f, 0.6f, 1.0f }; // Default: Cornflower blue
        bool clearDepth{ false };
        float depthValue{ 1.0f };
        uint8 stencilValue{ 0 };
    };

    class ClearPass
    {
    public:
        explicit ClearPass(const ClearPassConfig& config = {})
            : m_config(config)
        {
        }

        // Create RenderPassDescriptor for RenderGraph
        [[nodiscard]] auto CreatePassDescriptor(
            GraphicsDevice& graphicsDevice,
            const String& outputResourceName = "BackBuffer"
        ) const -> RenderPassDescriptor
        {
            RenderPassDescriptor descriptor;
            descriptor.name = "ClearPass";
            descriptor.outputResources.push_back(outputResourceName);

            // Capture necessary data for the lambda
            auto clearColor = m_config.clearColor;
            auto clearDepth = m_config.clearDepth;
            auto depthValue = m_config.depthValue;
            auto stencilValue = m_config.stencilValue;

            // Create execute callback
            descriptor.executeCallback = [&graphicsDevice, clearColor, clearDepth, depthValue, stencilValue]
            (ID3D12GraphicsCommandList* commandList)
            {
                // Get current RTV
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = graphicsDevice.GetCurrentRenderTargetView();

                // Clear render target
                commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

                // Clear depth/stencil if needed
                if (clearDepth)
                {
                    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = graphicsDevice.GetDepthStencilView();
                    commandList->ClearDepthStencilView(
                        dsvHandle,
                        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                        depthValue,
                        stencilValue,
                        0,
                        nullptr
                    );
                }
            };

            return descriptor;
        }

        // Update clear color
        auto SetClearColor(float r, float g, float b, float a) -> void
        {
            m_config.clearColor[0] = r;
            m_config.clearColor[1] = g;
            m_config.clearColor[2] = b;
            m_config.clearColor[3] = a;
        }

        // Update depth clear settings
        auto SetDepthClear(bool enabled, float value = 1.0f, uint8 stencil = 0) -> void
        {
            m_config.clearDepth = enabled;
            m_config.depthValue = value;
            m_config.stencilValue = stencil;
        }

        [[nodiscard]] auto GetConfig() const noexcept -> const ClearPassConfig&
        {
            return m_config;
        }

    private:
        ClearPassConfig m_config;
    };

} // namespace UnoEngine::Renderer
