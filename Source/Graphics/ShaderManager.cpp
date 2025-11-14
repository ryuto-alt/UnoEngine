#include "Graphics/ShaderManager.h"
#include "Core/Logger.h"
#include <fstream>
#include <sstream>

ShaderManager::ShaderManager() {
    Logger::Info("ShaderManager initialized");
}

auto ShaderManager::GenerateCacheKey(
    std::string_view path,
    std::string_view entryPoint,
    std::string_view target,
    std::span<const ShaderDefine> defines
) const -> std::string {
    std::string key = std::string(path) + "|" + std::string(entryPoint) + "|" + std::string(target);
    
    // Add defines to key for variation support
    for (const auto& define : defines) {
        key += "|" + define.name + "=" + define.value;
    }
    
    return key;
}

auto ShaderManager::ConvertToD3DMacros(
    std::span<const ShaderDefine> defines
) const -> std::vector<D3D_SHADER_MACRO> {
    std::vector<D3D_SHADER_MACRO> macros;
    macros.reserve(defines.size() + 1);
    
    for (const auto& define : defines) {
        D3D_SHADER_MACRO macro{};
        macro.Name = define.name.c_str();
        macro.Definition = define.value.c_str();
        macros.push_back(macro);
    }
    
    // Null terminator required by D3DCompile
    macros.push_back({nullptr, nullptr});
    
    return macros;
}

auto ShaderManager::CompileFromFile(
    std::string_view path,
    std::string_view entryPoint,
    std::string_view target,
    std::span<const ShaderDefine> defines
) -> Expected<ComPtr<ID3DBlob>, Error> {
    Logger::Info("Compiling shader: {} (Entry: {}, Target: {})", path, entryPoint, target);
    
    // Convert path to wide string for D3DCompileFromFile
    std::filesystem::path filePath(path);
    if (!std::filesystem::exists(filePath)) {
        auto errorMsg = fmt::format("Shader file not found: {}", path);
        Logger::Error(errorMsg);
        return Expected<ComPtr<ID3DBlob>, Error>::Unexpected(Error(errorMsg));
    }
    
    // Convert defines to D3D_SHADER_MACRO
    auto macros = ConvertToD3DMacros(defines);
    
    // Setup include handler for #include support
    auto shaderDir = filePath.parent_path();
    IncludeHandler includeHandler(shaderDir);
    
    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;
    
    UINT compileFlags = 0;
#ifdef _DEBUG
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    
    auto hr = D3DCompileFromFile(
        filePath.wstring().c_str(),
        macros.data(),
        &includeHandler,
        std::string(entryPoint).c_str(),
        std::string(target).c_str(),
        compileFlags,
        0,
        &shaderBlob,
        &errorBlob
    );
    
    if (FAILED(hr)) {
        std::string errorMsg = "Unknown shader compilation error";
        
        if (errorBlob) {
            errorMsg = std::string(
                static_cast<const char*>(errorBlob->GetBufferPointer()),
                errorBlob->GetBufferSize()
            );
            Logger::Error("Shader compilation failed:\n{}\nFile: {}\nEntry: {}\nTarget: {}",
                errorMsg, path, entryPoint, target);
        } else {
            errorMsg = fmt::format("Failed to compile shader: {} (HRESULT: {:#x})", path, static_cast<uint32>(hr));
            Logger::Error(errorMsg);
        }
        
        return Expected<ComPtr<ID3DBlob>, Error>::Unexpected(Error(errorMsg));
    }
    
    Logger::Info("Successfully compiled shader: {} (Size: {} bytes)", 
        path, shaderBlob->GetBufferSize());
    
    return shaderBlob;
}

auto ShaderManager::GetOrCompile(
    std::string_view path,
    std::string_view entryPoint,
    std::string_view target,
    std::span<const ShaderDefine> defines
) -> Expected<ComPtr<ID3DBlob>, Error> {
    auto cacheKey = GenerateCacheKey(path, entryPoint, target, defines);
    
    // Check cache
    auto it = m_cache.find(cacheKey);
    if (it != m_cache.end()) {
        // Check if file has been modified
        std::filesystem::path filePath(path);
        if (std::filesystem::exists(filePath)) {
            auto currentWriteTime = std::filesystem::last_write_time(filePath);
            if (currentWriteTime == it->second.lastWriteTime) {
                m_cacheHits++;
                Logger::Trace("Shader cache hit: {}", path);
                return it->second.blob;
            } else {
                Logger::Info("Shader file modified, recompiling: {}", path);
            }
        }
    }
    
    m_cacheMisses++;
    
    // Compile shader
    auto result = CompileFromFile(path, entryPoint, target, defines);
    if (!result) {
        return result;
    }
    
    // Cache the compiled shader
    CacheEntry entry{};
    entry.blob = result.value();
    entry.path = path;
    
    if (std::filesystem::exists(path)) {
        entry.lastWriteTime = std::filesystem::last_write_time(path);
    }
    
    m_cache[cacheKey] = std::move(entry);
    
    Logger::Info("Shader cached: {}", path);
    
    return result;
}

auto ShaderManager::ReloadShader(std::string_view path) -> Expected<void, Error> {
    Logger::Info("Reloading shader: {}", path);
    
    // Remove all cache entries for this file path
    uint32 removedCount = 0;
    for (auto it = m_cache.begin(); it != m_cache.end(); ) {
        if (it->second.path == path) {
            it = m_cache.erase(it);
            removedCount++;
        } else {
            ++it;
        }
    }
    
    if (removedCount == 0) {
        auto errorMsg = fmt::format("Shader not found in cache: {}", path);
        Logger::Warn(errorMsg);
        return Expected<void, Error>::Unexpected(Error(errorMsg));
    }
    
    Logger::Info("Removed {} shader variant(s) from cache: {}", removedCount, path);
    return {};
}

auto ShaderManager::ReloadAllShaders() -> uint32 {
    uint32 count = static_cast<uint32>(m_cache.size());
    m_cache.clear();
    Logger::Info("Cleared all shaders from cache ({} shaders)", count);
    return count;
}

auto ShaderManager::GetStatistics() const -> Statistics {
    Statistics stats{};
    stats.totalShaders = static_cast<uint32>(m_cache.size());
    stats.cacheHits = m_cacheHits;
    stats.cacheMisses = m_cacheMisses;
    return stats;
}

auto ShaderManager::ClearStatistics() -> void {
    m_cacheHits = 0;
    m_cacheMisses = 0;
    Logger::Info("Shader statistics cleared");
}

// ========================================
// IncludeHandler Implementation
// ========================================

ShaderManager::IncludeHandler::IncludeHandler(const std::filesystem::path& shaderDir)
    : m_shaderDirectory(shaderDir)
{
    Logger::Trace("IncludeHandler created for directory: {}", shaderDir.string());
}

HRESULT ShaderManager::IncludeHandler::Open(
    D3D_INCLUDE_TYPE IncludeType,
    LPCSTR pFileName,
    LPCVOID pParentData,
    LPCVOID* ppData,
    UINT* pBytes
) {
    std::filesystem::path includePath;
    
    if (IncludeType == D3D_INCLUDE_LOCAL) {
        // Local include: search in shader directory
        includePath = m_shaderDirectory / pFileName;
    } else {
        // System include: search in shader directory (no separate system dir for now)
        includePath = m_shaderDirectory / pFileName;
    }
    
    if (!std::filesystem::exists(includePath)) {
        Logger::Error("Include file not found: {}", includePath.string());
        return E_FAIL;
    }
    
    // Read file content
    std::ifstream file(includePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        Logger::Error("Failed to open include file: {}", includePath.string());
        return E_FAIL;
    }
    
    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    auto buffer = std::make_unique<std::vector<char>>(fileSize);
    if (!file.read(buffer->data(), fileSize)) {
        Logger::Error("Failed to read include file: {}", includePath.string());
        return E_FAIL;
    }
    
    *ppData = buffer->data();
    *pBytes = static_cast<UINT>(fileSize);
    
    m_fileBuffers.push_back(std::move(buffer));
    
    Logger::Trace("Loaded include file: {} ({} bytes)", includePath.string(), static_cast<size_t>(fileSize));
    
    return S_OK;
}

HRESULT ShaderManager::IncludeHandler::Close(LPCVOID pData) {
    // Data is managed by m_fileBuffers, will be automatically freed
    return S_OK;
}
