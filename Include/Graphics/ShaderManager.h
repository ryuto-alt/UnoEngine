#pragma once

#include "Core/CoreTypes.h"
#include "Core/Expected.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <span>
#include <filesystem>

using Microsoft::WRL::ComPtr;

/**
 * @brief Shader macro definition for #define support
 */
struct ShaderDefine {
    std::string name;
    std::string value;
    
    ShaderDefine() = default;
    ShaderDefine(std::string_view n, std::string_view v) 
        : name(n), value(v) {}
};

/**
 * @brief Manages HLSL shader compilation, caching, and hot-reloading
 * 
 * Features:
 * - Runtime compilation using D3DCompile API
 * - File path-based caching for performance
 * - #include support for code reuse
 * - #define support for shader variations (Uber Shader pattern)
 * - Manual hot-reload via API function
 * - Detailed error logging with line numbers
 * - Expected<T,E> error handling
 * 
 * Usage:
 * @code
 * auto shaderMgr = std::make_unique<ShaderManager>();
 * 
 * // Compile vertex shader
 * auto vsResult = shaderMgr->CompileFromFile(
 *     "Shaders/SimpleVertex.hlsl",
 *     "VSMain",
 *     "vs_6_0"
 * );
 * 
 * if (vsResult) {
 *     auto blob = vsResult.value();
 *     // Use blob for pipeline state
 * }
 * 
 * // With defines
 * std::vector<ShaderDefine> defines = {
 *     {"USE_NORMAL_MAP", "1"},
 *     {"NUM_LIGHTS", "4"}
 * };
 * auto psResult = shaderMgr->CompileFromFile(
 *     "Shaders/SimplePixel.hlsl",
 *     "PSMain",
 *     "ps_6_0",
 *     defines
 * );
 * 
 * // Hot-reload
 * shaderMgr->ReloadShader("Shaders/SimpleVertex.hlsl");
 * @endcode
 */
class ShaderManager {
public:
    ShaderManager();
    ~ShaderManager() = default;
    
    // Non-copyable, movable
    ShaderManager(const ShaderManager&) = delete;
    auto operator=(const ShaderManager&) -> ShaderManager& = delete;
    ShaderManager(ShaderManager&&) = default;
    auto operator=(ShaderManager&&) -> ShaderManager& = default;
    
    /**
     * @brief Compile HLSL shader from file
     * @param path Shader file path (relative or absolute)
     * @param entryPoint Entry point function name (e.g., "VSMain", "PSMain")
     * @param target Shader model target (e.g., "vs_6_0", "ps_6_0")
     * @param defines Optional shader macro definitions
     * @return Compiled shader blob on success, error message on failure
     */
    [[nodiscard]] auto CompileFromFile(
        std::string_view path,
        std::string_view entryPoint,
        std::string_view target,
        std::span<const ShaderDefine> defines = {}
    ) -> Expected<ComPtr<ID3DBlob>, Error>;
    
    /**
     * @brief Get or compile shader from cache
     * 
     * If the shader was previously compiled, returns cached version.
     * Otherwise, compiles and caches it.
     * 
     * @param path Shader file path
     * @param entryPoint Entry point function name
     * @param target Shader model target
     * @param defines Optional shader macro definitions
     * @return Cached or newly compiled shader blob
     */
    [[nodiscard]] auto GetOrCompile(
        std::string_view path,
        std::string_view entryPoint,
        std::string_view target,
        std::span<const ShaderDefine> defines = {}
    ) -> Expected<ComPtr<ID3DBlob>, Error>;
    
    /**
     * @brief Reload shader from disk (manual hot-reload)
     * 
     * Removes shader from cache and forces recompilation on next request.
     * 
     * @param path Shader file path to reload
     * @return void on success, error message on failure
     */
    [[nodiscard]] auto ReloadShader(std::string_view path) -> Expected<void, Error>;
    
    /**
     * @brief Reload all cached shaders
     * 
     * Clears entire cache. Useful for "Reload All Shaders" button in ImGui.
     * 
     * @return Number of shaders cleared from cache
     */
    auto ReloadAllShaders() -> uint32;
    
    /**
     * @brief Get cache statistics for debugging
     */
    struct Statistics {
        uint32 totalShaders;
        uint32 cacheHits;
        uint32 cacheMisses;
    };
    
    [[nodiscard]] auto GetStatistics() const -> Statistics;
    
    /**
     * @brief Clear all cache statistics
     */
    auto ClearStatistics() -> void;

private:
    /**
     * @brief Generate cache key from shader parameters
     */
    [[nodiscard]] auto GenerateCacheKey(
        std::string_view path,
        std::string_view entryPoint,
        std::string_view target,
        std::span<const ShaderDefine> defines
    ) const -> std::string;
    
    /**
     * @brief Convert ShaderDefine to D3D_SHADER_MACRO
     */
    [[nodiscard]] auto ConvertToD3DMacros(
        std::span<const ShaderDefine> defines
    ) const -> std::vector<D3D_SHADER_MACRO>;
    
    /**
     * @brief Custom include handler for #include support
     */
    class IncludeHandler : public ID3DInclude {
    public:
        explicit IncludeHandler(const std::filesystem::path& shaderDir);
        
        HRESULT __stdcall Open(
            D3D_INCLUDE_TYPE IncludeType,
            LPCSTR pFileName,
            LPCVOID pParentData,
            LPCVOID* ppData,
            UINT* pBytes
        ) override;
        
        HRESULT __stdcall Close(LPCVOID pData) override;
        
    private:
        std::filesystem::path m_shaderDirectory;
        std::vector<std::unique_ptr<std::vector<char>>> m_fileBuffers;
    };
    
    struct CacheEntry {
        ComPtr<ID3DBlob> blob;
        std::filesystem::path path;
        std::filesystem::file_time_type lastWriteTime;
    };
    
    std::unordered_map<std::string, CacheEntry> m_cache;
    
    // Statistics
    mutable uint32 m_cacheHits{0};
    mutable uint32 m_cacheMisses{0};
};
