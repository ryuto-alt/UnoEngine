# 三角形レンダリング実装計画

## 実装日時
2025年（セッション2）

## 目標
**Week 1 の目標達成**: 画面に三角形を表示する

## 実装順序（前回ヒアリング結果）

### 1. DescriptorHeapManager ✅
- **目的**: DirectX 12 の Descriptor Heap 管理
- **実装方針**:
  - CBV/SRV/UAV を共通 Heap で管理
  - RTV/DSV は別 Heap で管理（DX12 仕様）
  - 動的フリーリスト方式（使用済み Descriptor を再利用）
  - 固定サイズ（十分大きいサイズを最初に確保）

**設計**:
```cpp
class DescriptorHeapManager {
public:
    explicit DescriptorHeapManager(ID3D12Device* device);
    
    // Descriptor 割り当て
    [[nodiscard]] auto AllocateCBV() -> Expected<DescriptorHandle, Error>;
    [[nodiscard]] auto AllocateSRV() -> Expected<DescriptorHandle, Error>;
    [[nodiscard]] auto AllocateUAV() -> Expected<DescriptorHandle, Error>;
    [[nodiscard]] auto AllocateRTV() -> Expected<DescriptorHandle, Error>;
    [[nodiscard]] auto AllocateDSV() -> Expected<DescriptorHandle, Error>;
    
    // Descriptor 解放（フリーリストに戻す）
    auto FreeCBV(DescriptorHandle handle) -> void;
    auto FreeSRV(DescriptorHandle handle) -> void;
    auto FreeUAV(DescriptorHandle handle) -> void;
    auto FreeRTV(DescriptorHandle handle) -> void;
    auto FreeDSV(DescriptorHandle handle) -> void;
    
    // Heap 取得
    [[nodiscard]] auto GetCBVSRVUAVHeap() -> ID3D12DescriptorHeap*;
    [[nodiscard]] auto GetRTVHeap() -> ID3D12DescriptorHeap*;
    [[nodiscard]] auto GetDSVHeap() -> ID3D12DescriptorHeap*;
    
private:
    ComPtr<ID3D12Device> m_device;
    
    // CBV/SRV/UAV 共通 Heap
    ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
    uint32 m_cbvSrvUavDescriptorSize;
    uint32 m_cbvSrvUavHeapSize; // 固定サイズ（例: 10000）
    std::vector<uint32> m_freeCBVSRVUAVIndices; // フリーリスト
    
    // RTV Heap
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    uint32 m_rtvDescriptorSize;
    uint32 m_rtvHeapSize; // 固定サイズ（例: 100）
    std::vector<uint32> m_freeRTVIndices;
    
    // DSV Heap
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    uint32 m_dsvDescriptorSize;
    uint32 m_dsvHeapSize; // 固定サイズ（例: 100）
    std::vector<uint32> m_freeDSVIndices;
};

struct DescriptorHandle {
    D3D12_CPU_DESCRIPTOR_HANDLE cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu;
    uint32 heapIndex;
};
```

---

### 2. ShaderManager ✅
- **目的**: HLSL シェーダーのコンパイル・管理
- **実装方針**:
  - **ハイブリッドコンパイル**: Debug ビルドはランタイムコンパイル、Release ビルドはプリコンパイル
  - **ホットリロード**: Win32 `ReadDirectoryChangesW` でファイル監視
  - **エラーハンドリング**: 詳細ログ表示（Logger に出力）

**設計**:
```cpp
class ShaderManager {
public:
    explicit ShaderManager(ID3D12Device* device);
    
    // HLSL コンパイル
    [[nodiscard]] auto CompileFromFile(
        std::string_view path,
        std::string_view entryPoint,
        std::string_view target, // "vs_6_0", "ps_6_0" など
        std::span<const ShaderDefine> defines = {}
    ) -> Expected<ComPtr<ID3DBlob>, Error>;
    
    // キャッシュ機能付きコンパイル
    [[nodiscard]] auto GetOrCompile(
        std::string_view path,
        std::string_view entryPoint,
        std::string_view target,
        std::span<const ShaderDefine> defines = {}
    ) -> Expected<ComPtr<ID3DBlob>, Error>;
    
    // ホットリロード
    auto EnableHotReload(std::string_view directory) -> Expected<void, Error>;
    auto CheckForReloads() -> void; // 毎フレーム呼ぶ
    
private:
    ComPtr<ID3D12Device> m_device;
    
    struct ShaderCacheEntry {
        ComPtr<ID3DBlob> blob;
        std::filesystem::path path;
        std::filesystem::file_time_type lastWriteTime;
    };
    
    std::unordered_map<std::string, ShaderCacheEntry> m_cache;
    
    // ホットリロード用
    HANDLE m_directoryHandle{INVALID_HANDLE_VALUE};
    std::vector<std::string> m_changedFiles;
};

struct ShaderDefine {
    std::string name;
    std::string value;
};
```

**コンパイル実装**:
- Debug: `D3DCompileFromFile()` でランタイムコンパイル
- Release: プリコンパイル済み `.cso` ファイル読み込み（または同じくランタイム）

**ホットリロード実装**:
```cpp
auto ShaderManager::EnableHotReload(std::string_view directory) -> Expected<void, Error> {
    m_directoryHandle = CreateFileW(
        /* directory path */,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );
    
    if (m_directoryHandle == INVALID_HANDLE_VALUE) {
        return Expected<void, Error>::Unexpected("Failed to open directory for monitoring");
    }
    
    return {};
}

auto ShaderManager::CheckForReloads() -> void {
    // ReadDirectoryChangesW で変更をチェック
    // 変更されたファイルを m_changedFiles に追加
    // m_cache から削除して、次回の GetOrCompile() で再コンパイル
}
```

**エラーハンドリング**:
```cpp
if (FAILED(hr)) {
    if (errorBlob) {
        auto errorMsg = static_cast<const char*>(errorBlob->GetBufferPointer());
        Logger::Error("Shader compilation failed: {}\nFile: {}\nEntry: {}", 
            errorMsg, path, entryPoint);
    }
    return Expected<ComPtr<ID3DBlob>, Error>::Unexpected(
        fmt::format("Failed to compile shader: {}", path)
    );
}
```

---

### 3. Camera ✅
- **目的**: View/Projection 行列の提供
- **実装方針**:
  - 最小限の Camera クラス（詳細は Week 7-8 で実装）
  - GetViewProjection() だけ実装

**設計**:
```cpp
class Camera {
public:
    Camera() = default;
    
    auto SetPosition(const Vector3& position) -> void;
    auto SetRotation(const Vector3& rotation) -> void; // Euler angles
    
    auto SetProjection(float fovY, float aspect, float nearZ, float farZ) -> void;
    
    [[nodiscard]] auto GetViewMatrix() const -> Matrix4x4;
    [[nodiscard]] auto GetProjectionMatrix() const -> Matrix4x4;
    [[nodiscard]] auto GetViewProjectionMatrix() const -> Matrix4x4;
    
private:
    Vector3 m_position{0.0f, 0.0f, -5.0f};
    Vector3 m_rotation{0.0f, 0.0f, 0.0f}; // pitch, yaw, roll
    
    float m_fovY{DirectX::XM_PIDIV4}; // 45度
    float m_aspect{16.0f / 9.0f};
    float m_nearZ{0.1f};
    float m_farZ{1000.0f};
};
```

---

### 4. GeometryPass ✅
- **目的**: RenderGraph に Geometry 描画パスを追加
- **実装方針**:
  - RenderGraph::AddPass() で GeometryPass を追加
  - ClearPass → GeometryPass の順で実行

**設計**:
```cpp
class GeometryPass : public RenderPass {
public:
    GeometryPass(
        ID3D12Device* device,
        DescriptorHeapManager* descriptorHeapManager,
        BufferManager* bufferManager
    );
    
    auto Setup(RenderGraphBuilder& builder) -> void override;
    auto Execute(ID3D12GraphicsCommandList* commandList) -> void override;
    
    // 描画するオブジェクトを追加
    auto AddRenderObject(const RenderObject& obj) -> void;
    auto ClearRenderObjects() -> void;
    
private:
    ComPtr<ID3D12Device> m_device;
    DescriptorHeapManager* m_descriptorHeapManager;
    BufferManager* m_bufferManager;
    
    std::vector<RenderObject> m_renderObjects;
    
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
};

struct RenderObject {
    BufferHandle vertexBuffer;
    BufferHandle indexBuffer;
    uint32 indexCount;
    Matrix4x4 worldMatrix;
    // 将来的に Material などを追加
};
```

---

### 5. PrimitiveRenderer ✅
- **目的**: 三角形・Quad・デバッグ図形の描画
- **実装方針**:
  - BufferManager を使って頂点バッファ作成
  - GeometryPass に RenderObject を追加

**設計**:
```cpp
class PrimitiveRenderer {
public:
    PrimitiveRenderer(
        BufferManager* bufferManager,
        GeometryPass* geometryPass
    );
    
    // 三角形描画
    auto DrawTriangle(
        const Vector3& v0,
        const Vector3& v1,
        const Vector3& v2,
        const Color& color,
        const Matrix4x4& transform = Matrix4x4::Identity
    ) -> Expected<void, Error>;
    
    // Quad 描画
    auto DrawQuad(
        const Vector3& position,
        const Vector2& size,
        const Color& color,
        const Matrix4x4& transform = Matrix4x4::Identity
    ) -> Expected<void, Error>;
    
    // デバッグライン描画
    auto DrawLine(
        const Vector3& start,
        const Vector3& end,
        const Color& color
    ) -> Expected<void, Error>;
    
    // デバッグボックス描画
    auto DrawWireBox(
        const BoundingBox& box,
        const Color& color
    ) -> Expected<void, Error>;
    
private:
    BufferManager* m_bufferManager;
    GeometryPass* m_geometryPass;
    
    // 一時的な頂点データ
    std::vector<Vertex> m_tempVertices;
    std::vector<uint32> m_tempIndices;
};

struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 texcoord;
    Color color;
};
```

---

## 実装の進め方

### コード品質基準
- ✅ C++20 機能を積極活用（std::span, std::source_location, concepts）
- ✅ Expected<T, E> で一貫したエラーハンドリング
- ✅ Logger で詳細ログ出力
- ✅ コメント充実（将来の Doxygen 対応）

### エラー対応
- **即座に報告**: エラーが出たらすぐにユーザーに報告
- Logger::Error() で詳細情報を出力

### コミットタイミング
- **機能単位**: 各機能完成時にコミット
  1. DescriptorHeapManager 完成 → コミット
  2. ShaderManager 完成 → コミット
  3. Camera 完成 → コミット
  4. GeometryPass 完成 → コミット
  5. PrimitiveRenderer 完成 → コミット
  6. 三角形表示成功 → 最終コミット

---

## シェーダーファイル

### SimpleVertex.hlsl
```hlsl
cbuffer SceneConstants : register(b0) {
    float4x4 viewProjection;
};

cbuffer ObjectConstants : register(b1) {
    float4x4 world;
};

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(VSInput input) {
    PSInput output;
    
    float4 worldPos = mul(float4(input.position, 1.0f), world);
    output.position = mul(worldPos, viewProjection);
    output.color = input.color;
    
    return output;
}
```

### SimplePixel.hlsl
```hlsl
struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 PSMain(PSInput input) : SV_TARGET {
    return input.color;
}
```

---

## 予想される課題

### 1. DescriptorHeapManager
- フリーリストの実装が複雑
- Descriptor のライフタイム管理

### 2. ShaderManager
- ホットリロードのスレッドセーフ性
- ファイル監視の信頼性

### 3. RootSignature
- CBV のバインド方法
- Descriptor Table vs Root Descriptor

### 4. PipelineState
- InputLayout の定義
- Rasterizer/Blend/DepthStencil State

---

## 成功基準

✅ 画面に三角形が表示される
✅ 三角形が正しい色で描画される
✅ カメラを動かすと三角形の見え方が変わる
✅ シェーダーファイルを変更すると即座に反映される（ホットリロード）
✅ ビルドエラーなし
✅ 詳細ログが出力される

---

## 次のステップ（Week 2-3）

- TextureManager 実装
- MaterialManager 実装
- glTF Model Loader 実装
- テクスチャ付きモデル表示
