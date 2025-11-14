#include "Include/Core/Engine.h"
#include "Include/Core/Components.h"
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

        std::cout << "Sample Application initialized successfully!" << std::endl;
        return true;
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
        // Rendering logic will be implemented with the render graph
        auto& graphicsDevice = GetGraphicsDevice();

        // For now, just clear the screen
        // Actual rendering will be set up once we have shaders and pipeline states
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

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
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

        app.Shutdown();

        std::cout << "Application exited with code: " << exitCode << std::endl;
        return exitCode;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}
