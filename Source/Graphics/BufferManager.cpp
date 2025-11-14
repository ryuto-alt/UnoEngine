#include "../../Include/Graphics/BufferManager.h"
#include "../../Include/Core/Logger.h"
#include <algorithm>

namespace UnoEngine::Graphics
{
    using namespace UnoEngine::Core;

    BufferManager::BufferManager(ID3D12Device* device)
        : m_device(device)
    {
        Logger::Info("BufferManager initialized");
    }

    auto BufferManager::CreateVertexBuffer(
        std::span<const std::byte> data,
        uint32 stride,
        std::string_view name
    ) -> Expected<BufferHandle, Error>
    {
        if (data.empty())
        {
            return Expected<BufferHandle, Error>::Unexpected(
                Error("Vertex data is empty")
            );
        }

        // Default Heapにバッファ作成
        auto resourceResult = CreateDefaultBuffer(data, name);
        if (!resourceResult)
        {
            return Expected<BufferHandle, Error>::Unexpected(resourceResult.error());
        }

        // バッファ情報を格納
        BufferInfo info{};
        info.resource = resourceResult.value();
        info.gpuAddress = info.resource->GetGPUVirtualAddress();
        info.sizeInBytes = data.size();
        info.stride = stride;
        info.type = BufferType::Vertex;
        info.name = String(name);

        BufferHandle handle = m_nextHandle++;
        m_buffers.push_back(std::move(info));

        Logger::Info("Created VertexBuffer '{}' (Handle: {}, Size: {} bytes, Stride: {})",
            name, handle, data.size(), stride);

        return handle;
    }

    auto BufferManager::CreateIndexBuffer(
        std::span<const uint32> indices,
        std::string_view name
    ) -> Expected<BufferHandle, Error>
    {
        if (indices.empty())
        {
            return Expected<BufferHandle, Error>::Unexpected(
                Error("Index data is empty")
            );
        }

        // uint32配列をbyteスパンに変換
        auto byteData = std::as_bytes(indices);

        auto resourceResult = CreateDefaultBuffer(byteData, name);
        if (!resourceResult)
        {
            return Expected<BufferHandle, Error>::Unexpected(resourceResult.error());
        }

        BufferInfo info{};
        info.resource = resourceResult.value();
        info.gpuAddress = info.resource->GetGPUVirtualAddress();
        info.sizeInBytes = byteData.size();
        info.stride = sizeof(uint32);
        info.type = BufferType::Index;
        info.name = String(name);

        BufferHandle handle = m_nextHandle++;
        m_buffers.push_back(std::move(info));

        Logger::Info("Created IndexBuffer '{}' (Handle: {}, Count: {})",
            name, handle, indices.size());

        return handle;
    }

    auto BufferManager::CreateConstantBuffer(
        uint64 sizeInBytes,
        std::string_view name
    ) -> Expected<BufferHandle, Error>
    {
        // 定数バッファは256バイトアライメント必要
        uint64 alignedSize = (sizeInBytes + 255) & ~255;

        auto resourceResult = CreateUploadBuffer(alignedSize, name);
        if (!resourceResult)
        {
            return Expected<BufferHandle, Error>::Unexpected(resourceResult.error());
        }

        auto resource = resourceResult.value();

        // メモリマップ
        void* mappedData = nullptr;
        D3D12_RANGE readRange{ 0, 0 }; // CPUからは読み込まない
        HRESULT hr = resource->Map(0, &readRange, &mappedData);
        if (FAILED(hr))
        {
            return Expected<BufferHandle, Error>::Unexpected(
                Error("Failed to map constant buffer")
            );
        }

        BufferInfo info{};
        info.resource = resource;
        info.gpuAddress = resource->GetGPUVirtualAddress();
        info.mappedData = mappedData;
        info.sizeInBytes = alignedSize;
        info.type = BufferType::Constant;
        info.name = String(name);

        BufferHandle handle = m_nextHandle++;
        m_buffers.push_back(std::move(info));

        Logger::Info("Created ConstantBuffer '{}' (Handle: {}, Size: {} bytes)",
            name, handle, alignedSize);

        return handle;
    }

    auto BufferManager::UpdateConstantBuffer(
        BufferHandle handle,
        std::span<const std::byte> data
    ) -> Expected<void, Error>
    {
        auto bufferResult = GetBuffer(handle);
    if (!bufferResult)
    {
        return Expected<void, Error>::Unexpected(bufferResult.error());
    }

    const auto* buffer = bufferResult.value();

    if (buffer->type != BufferType::Constant)
        {
            return Expected<void, Error>::Unexpected(
                Error("Buffer is not a constant buffer")
            );
        }

        if (data.size() > buffer->sizeInBytes)
    {
        return Expected<void, Error>::Unexpected(
            Error("Data size exceeds buffer size")
        );
    }

    // メモリコピー
    std::memcpy(buffer->mappedData, data.data(), data.size());

        return {};
    }

    auto BufferManager::GetBuffer(BufferHandle handle) const
    -> Expected<const BufferInfo*, Error>
{
        // ハンドルからインデックスを計算（handle - 1）
        size_t index = static_cast<size_t>(handle) - 1;

        if (index >= m_buffers.size())
    {
        return Expected<const BufferInfo*, Error>::Unexpected(
            Error("Invalid buffer handle")
        );
    }

    return &m_buffers[index];
    }

    auto BufferManager::GetVertexBufferView(BufferHandle handle) const
        -> Expected<D3D12_VERTEX_BUFFER_VIEW, Error>
    {
        auto bufferResult = GetBuffer(handle);
    if (!bufferResult)
    {
        return Expected<D3D12_VERTEX_BUFFER_VIEW, Error>::Unexpected(
            bufferResult.error()
        );
    }

    const auto* buffer = bufferResult.value();

        if (buffer->type != BufferType::Vertex)
    {
        return Expected<D3D12_VERTEX_BUFFER_VIEW, Error>::Unexpected(
            Error("Buffer is not a vertex buffer")
        );
    }

    D3D12_VERTEX_BUFFER_VIEW view{};
    view.BufferLocation = buffer->gpuAddress;
    view.SizeInBytes = static_cast<UINT>(buffer->sizeInBytes);
    view.StrideInBytes = buffer->stride;

        return view;
    }

    auto BufferManager::GetIndexBufferView(BufferHandle handle) const
        -> Expected<D3D12_INDEX_BUFFER_VIEW, Error>
    {
        auto bufferResult = GetBuffer(handle);
    if (!bufferResult)
    {
        return Expected<D3D12_INDEX_BUFFER_VIEW, Error>::Unexpected(
            bufferResult.error()
        );
    }

    const auto* buffer = bufferResult.value();

        if (buffer->type != BufferType::Index)
    {
        return Expected<D3D12_INDEX_BUFFER_VIEW, Error>::Unexpected(
            Error("Buffer is not an index buffer")
        );
    }

    D3D12_INDEX_BUFFER_VIEW view{};
    view.BufferLocation = buffer->gpuAddress;
    view.SizeInBytes = static_cast<UINT>(buffer->sizeInBytes);
    view.Format = DXGI_FORMAT_R32_UINT;

        return view;
    }

    auto BufferManager::DestroyBuffer(BufferHandle handle) -> void
    {
        size_t index = static_cast<size_t>(handle) - 1;

        if (index >= m_buffers.size())
        {
            Logger::Warn("Attempted to destroy invalid buffer handle: {}", handle);
            return;
        }

        auto& buffer = m_buffers[index];

        // マップされている場合はアンマップ
        if (buffer.IsMapped())
        {
            buffer.resource->Unmap(0, nullptr);
        }

        Logger::Info("Destroyed buffer '{}'", buffer.name);

        // リソース解放（ComPtrが自動的に行う）
        buffer.resource.Reset();
    }

    auto BufferManager::Clear() -> void
    {
        for (auto& buffer : m_buffers)
        {
            if (buffer.IsMapped())
            {
                buffer.resource->Unmap(0, nullptr);
            }
            buffer.resource.Reset();
        }

        m_buffers.clear();
        m_uploadBuffers.clear();
        m_nextHandle = 1;

        Logger::Info("BufferManager cleared");
    }

    auto BufferManager::CreateDefaultBuffer(
        std::span<const std::byte> data,
        std::string_view name
    ) -> Expected<ComPtr<ID3D12Resource>, Error>
    {
        // Default Heap用リソース作成
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = data.size();
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ComPtr<ID3D12Resource> defaultBuffer;
        HRESULT hr = m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&defaultBuffer)
        );

        if (FAILED(hr))
        {
            return Expected<ComPtr<ID3D12Resource>, Error>::Unexpected(
                Error("Failed to create default buffer")
            );
        }

        // Upload Heap用リソース作成（データ転送用）
        auto uploadResult = CreateUploadBuffer(data.size(), "UploadBuffer");
        if (!uploadResult)
        {
            return Expected<ComPtr<ID3D12Resource>, Error>::Unexpected(
                uploadResult.error()
            );
        }

        auto uploadBuffer = uploadResult.value();

        // データをUpload Bufferにコピー
        void* mappedData = nullptr;
        D3D12_RANGE readRange{ 0, 0 };
        hr = uploadBuffer->Map(0, &readRange, &mappedData);
        if (FAILED(hr))
        {
            return Expected<ComPtr<ID3D12Resource>, Error>::Unexpected(
                Error("Failed to map upload buffer")
            );
        }

        std::memcpy(mappedData, data.data(), data.size());
        uploadBuffer->Unmap(0, nullptr);

        // TODO: コマンドリストでUpload→Defaultにコピー
        // 現在は簡易実装のため、Upload Bufferを保持しておく
        m_uploadBuffers.push_back(uploadBuffer);

        // デバッグ名設定
        defaultBuffer->SetName(std::wstring(name.begin(), name.end()).c_str());

        return defaultBuffer;
    }

    auto BufferManager::CreateUploadBuffer(
        uint64 sizeInBytes,
        std::string_view name
    ) -> Expected<ComPtr<ID3D12Resource>, Error>
    {
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = sizeInBytes;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ComPtr<ID3D12Resource> uploadBuffer;
        HRESULT hr = m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadBuffer)
        );

        if (FAILED(hr))
        {
            return Expected<ComPtr<ID3D12Resource>, Error>::Unexpected(
                Error("Failed to create upload buffer")
            );
        }

        uploadBuffer->SetName(std::wstring(name.begin(), name.end()).c_str());

        return uploadBuffer;
    }

} // namespace UnoEngine::Graphics
