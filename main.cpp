#include "Include/Core/Engine.h"
#include "Include/Core/Components.h"
#include "Include/Renderer/RenderGraph.h"
#include "Include/Renderer/ClearPass.h"
#include <Windows.h>
#include <iostream>

// ========================================
// DirectX 12 Agility SDK Configuration (Version 1.618.3)
// ========================================
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 618; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using namespace UnoEngine;
using namespace UnoEngine::Core;
using namespace UnoEngine::Core::ECS;
using namespace UnoEngine::Core::Components;
using namespace UnoEngine::Renderer;

// ========================================
// Sample Application
// ========================================

class SampleApplication : public Engine
{
public:
    auto OnInitialize() -> bool override
    {
        std::cout << "Initializing Sample Application..." << std::endl;

        // Initialize ECS
        auto& ecs = GetECSCoordinator();

        // Register components
        ecs.RegisterComponent<TransformComponent>();
        ecs.RegisterComponent<MeshComponent>();
        ecs.RegisterComponent<MaterialComponent>();
        ecs.RegisterComponent<CameraComponent>();
        ecs.RegisterComponent<LightComponent>();
        ecs.RegisterComponent<TagComponent>();

        // Create a camera entity
        CreateCamera();

        // Create a simple light
        CreateDirectionalLight();

        // Create a sample cube
        CreateCube();

        // Setup RenderGraph once during initialization
        SetupRenderGraph();

        std::cout << "Sample Application initialized successfully!" << std::endl;
        return true;
    }

    auto SetupRenderGraph() -> void
    {
        auto& graphicsDevice = GetGraphicsDevice();
        auto& renderGraph = GetRenderGraph();

        // Declare BackBuffer resource
        ResourceDescriptor backBufferDesc;
        backBufferDesc.name = "BackBuffer";
        backBufferDesc.type = ResourceType::Texture2D;
        backBufferDesc.usage = ResourceUsage::RenderTarget;
        backBufferDesc.format = graphicsDevice.GetBackBufferFormat();
        backBufferDesc.width = GetWindow().GetWidth();
        backBufferDesc.height = GetWindow().GetHeight();
        renderGraph.DeclareResource(backBufferDesc);

        // Create ClearPass
        ClearPassConfig clearConfig;
        clearConfig.clearColor[0] = 0.0f;  // R
        clearConfig.clearColor[1] = 0.4f;  // G
        clearConfig.clearColor[2] = 0.6f;  // B
        clearConfig.clearColor[3] = 1.0f;  // A
        clearConfig.clearDepth = true;

        ClearPass clearPass(clearConfig);

        // Add ClearPass to RenderGraph
        renderGraph.AddPass(clearPass.CreatePassDescriptor(graphicsDevice, "BackBuffer"));

        // Compile RenderGraph
        if (!renderGraph.Compile(graphicsDevice.GetDevice()))
        {
            std::cerr << "Failed to compile RenderGraph!" << std::endl;
        }
    }

    auto OnUpdate(float deltaTime) -> void override
    {
        // Update logic here
        // For example: rotate the cube
        auto& ecs = GetECSCoordinator();

        // Simple rotation animation (will be implemented later with proper systems)
        // This is just a placeholder demonstration
    }

    auto OnRender() -> void override
    {
        auto& graphicsDevice = GetGraphicsDevice();
        auto& renderGraph = GetRenderGraph();
        auto* commandList = graphicsDevice.GetCommandList();

        // Update BackBuffer resource (changes every frame due to swap chain)
        auto* backBufferResource = renderGraph.GetResource("BackBuffer");
        if (backBufferResource)
        {
            backBufferResource->SetD3DResource(graphicsDevice.GetCurrentBackBuffer());
            backBufferResource->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        // Execute RenderGraph
        renderGraph.Execute(commandList);
    }

    auto OnShutdown() -> void override
    {
        std::cout << "Shutting down Sample Application..." << std::endl;
    }

private:
    auto CreateCamera() -> void
    {
        auto& ecs = GetECSCoordinator();

        Entity camera = ecs.CreateEntity();

        // Add transform component
        TransformComponent transform{};
        transform.position = { 0.0f, 2.0f, -5.0f };
        ecs.AddComponent(camera, transform);

        // Add camera component
        CameraComponent cameraComp{};
        cameraComp.fieldOfView = 60.0f;
        cameraComp.aspectRatio = static_cast<float>(GetWindow().GetWidth()) /
                                  static_cast<float>(GetWindow().GetHeight());
        cameraComp.nearPlane = 0.1f;
        cameraComp.farPlane = 1000.0f;
        ecs.AddComponent(camera, cameraComp);

        // Add tag component
        TagComponent tag{};
        tag.name = "Main Camera";
        tag.tag = "Camera";
        ecs.AddComponent(camera, tag);
    }

    auto CreateDirectionalLight() -> void
    {
        auto& ecs = GetECSCoordinator();

        Entity light = ecs.CreateEntity();

        // Add transform component
        TransformComponent transform{};
        transform.position = { 0.0f, 10.0f, 0.0f };
        transform.SetRotationFromEuler(-45.0f, 0.0f, 0.0f);
        ecs.AddComponent(light, transform);

        // Add light component
        LightComponent lightComp{};
        lightComp.type = LightType::Directional;
        lightComp.color = { 1.0f, 1.0f, 1.0f };
        lightComp.intensity = 1.0f;
        ecs.AddComponent(light, lightComp);

        // Add tag component
        TagComponent tag{};
        tag.name = "Directional Light";
        tag.tag = "Light";
        ecs.AddComponent(light, tag);
    }

    auto CreateCube() -> void
    {
        auto& ecs = GetECSCoordinator();

        Entity cube = ecs.CreateEntity();

        // Add transform component
        TransformComponent transform{};
        transform.position = { 0.0f, 0.0f, 0.0f };
        transform.scale = { 1.0f, 1.0f, 1.0f };
        ecs.AddComponent(cube, transform);

        // Add mesh component (mesh data will be loaded later)
        MeshComponent mesh{};
        // TODO: Load actual mesh data
        ecs.AddComponent(cube, mesh);

        // Add material component
        MaterialComponent material{};
        material.albedoColor = { 0.8f, 0.3f, 0.3f, 1.0f };
        material.metallic = 0.2f;
        material.roughness = 0.6f;
        ecs.AddComponent(cube, material);

        // Add tag component
        TagComponent tag{};
        tag.name = "Sample Cube";
        tag.tag = "Mesh";
        ecs.AddComponent(cube, tag);
    }
};

// ========================================
// Application Entry Point
// ========================================

#ifdef _DEBUG
// Debug build: Use console application entry point
int main(int argc, char** argv)
#else
// Release build: Use Windows application entry point
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
#endif
{
    try
    {
        std::cout << "========================================" << std::endl;
        std::cout << "    UnoEngine - DirectX 12 Game Engine  " << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        // Create engine configuration
        EngineConfig config{};

        // Window configuration
        config.windowConfig.title = "UnoEngine - DirectX12 Sample";
        config.windowConfig.width = 1280;
        config.windowConfig.height = 720;
        config.windowConfig.resizable = true;
        config.windowConfig.vsync = true;

        // Graphics configuration
        config.graphicsConfig.enableDebugLayer = true;
        config.graphicsConfig.enableGpuValidation = false;
        config.graphicsConfig.backBufferCount = 2;
        config.graphicsConfig.width = config.windowConfig.width;
        config.graphicsConfig.height = config.windowConfig.height;

        // Application name
        config.applicationName = "UnoEngine Sample Application";

        // Create and run application
        SampleApplication app;

        if (!app.Initialize(config))
        {
            std::cerr << "Failed to initialize application!" << std::endl;
            return -1;
        }

        std::cout << "Starting main loop..." << std::endl;
        int32 exitCode = app.Run();

        std::cout << "[main] Run() returned, calling Shutdown()..." << std::endl;
        app.Shutdown();
        std::cout << "[main] Shutdown() complete" << std::endl;

        std::cout << "Application exited with code: " << exitCode << std::endl;
        return exitCode;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}
