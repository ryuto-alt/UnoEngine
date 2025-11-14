#pragma once

#include "../Core/CoreTypes.h"
#include "../Core/Expected.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <span>
#include <memory>
#include <string>
#include <vector>

namespace UnoEngine::Graphics
{
    using Microsoft::WRL::ComPtr;
    using namespace UnoEngine::Core;

    /// <summary>
    /// バッファハンドル（ID）
    /// </summary>
    using BufferHandle = uint32;

    /// <summary>
    /// バッファの種類
    /// </summary>
    enum class BufferType : uint8
    {
        Vertex,      // 頂点バッファ
        Index,       // インデックスバッファ
        Constant,    // 定数バッファ
        Upload       // アップロードバッファ
    };

    /// <summary>
    /// バッファ情報
    /// </summary>
    struct BufferInfo
    {
        ComPtr<ID3D12Resource> resource;
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress{};
        void* mappedData{};       // Constant/Uploadバッファの場合のみ
        uint64 sizeInBytes{};
        uint32 stride{};          // Vertex/Indexバッファの場合
        BufferType type{};
        String name;

        [[nodiscard]] constexpr auto IsMapped() const noexcept -> bool
        {
            return mappedData != nullptr;
        }
    };

    /// <summary>
    /// バッファ管理クラス
    /// VertexBuffer、IndexBuffer、ConstantBufferを作成・管理
    /// </summary>
    class BufferManager
    {
    public:
        explicit BufferManager(ID3D12Device* device);
        ~BufferManager() = default;

        // コピー禁止
        BufferManager(const BufferManager&) = delete;
        auto operator=(const BufferManager&) -> BufferManager& = delete;

        // ムーブ許可
        BufferManager(BufferManager&&) noexcept = default;
        auto operator=(BufferManager&&) noexcept -> BufferManager& = default;

        /// <summary>
        /// 頂点バッファを作成
        /// </summary>
        /// <param name="data">頂点データ（std::span使用）</param>
        /// <param name="stride">1頂点のサイズ（バイト）</param>
        /// <param name="name">バッファ名（デバッグ用）</param>
        [[nodiscard]] auto CreateVertexBuffer(
            std::span<const std::byte> data,
            uint32 stride,
            std::string_view name = "VertexBuffer"
        ) -> Expected<BufferHandle, Error>;

        /// <summary>
        /// インデックスバッファを作成
        /// </summary>
        [[nodiscard]] auto CreateIndexBuffer(
            std::span<const uint32> indices,
            std::string_view name = "IndexBuffer"
        ) -> Expected<BufferHandle, Error>;

        /// <summary>
        /// 定数バッファを作成（CPUから更新可能）
        /// </summary>
        [[nodiscard]] auto CreateConstantBuffer(
            uint64 sizeInBytes,
            std::string_view name = "ConstantBuffer"
        ) -> Expected<BufferHandle, Error>;

        /// <summary>
        /// 定数バッファのデータを更新
        /// </summary>
        auto UpdateConstantBuffer(
            BufferHandle handle,
            std::span<const std::byte> data
        ) -> Expected<void, Error>;

        /// <summary>
        /// バッファ情報を取得
        /// </summary>
        [[nodiscard]] auto GetBuffer(BufferHandle handle) const
            -> Expected<const BufferInfo*, Error>;

        /// <summary>
        /// 頂点バッファビューを取得
        /// </summary>
        [[nodiscard]] auto GetVertexBufferView(BufferHandle handle) const
            -> Expected<D3D12_VERTEX_BUFFER_VIEW, Error>;

        /// <summary>
        /// インデックスバッファビューを取得
        /// </summary>
        [[nodiscard]] auto GetIndexBufferView(BufferHandle handle) const
            -> Expected<D3D12_INDEX_BUFFER_VIEW, Error>;

        /// <summary>
        /// バッファを破棄
        /// </summary>
        auto DestroyBuffer(BufferHandle handle) -> void;

        /// <summary>
        /// 全バッファを破棄
        /// </summary>
        auto Clear() -> void;

    private:
        /// <summary>
        /// Default Heapにバッファを作成（GPU専用）
        /// </summary>
        [[nodiscard]] auto CreateDefaultBuffer(
            std::span<const std::byte> data,
            std::string_view name
        ) -> Expected<ComPtr<ID3D12Resource>, Error>;

        /// <summary>
        /// Upload Heapにバッファを作成（CPU→GPU転送用）
        /// </summary>
        [[nodiscard]] auto CreateUploadBuffer(
            uint64 sizeInBytes,
            std::string_view name
        ) -> Expected<ComPtr<ID3D12Resource>, Error>;

        ID3D12Device* m_device{};
        std::vector<BufferInfo> m_buffers;
        BufferHandle m_nextHandle{ 1 }; // 0は無効なハンドル

        // 一時的なアップロードバッファ（Default Heapへのコピー用）
        std::vector<ComPtr<ID3D12Resource>> m_uploadBuffers;
    };

} // namespace UnoEngine::Graphics
