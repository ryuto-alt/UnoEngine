# UnoEngine - エンジン要件仕様書

## エンジンの方向性

**ターゲットユーザー**: 中級者プログラマー（C++とDirectX12の基礎理解者）
**開発期間**: 3-6ヶ月
**開発方針**: 
- コア機能完成優先
- 使いやすさ向上
- パフォーマンス最適化
- 先進機能追加

**基本設計思想**:
- C++20/23の最新機能を積極活用
- プラグイン不要のシンプルな設計
- コードベースでのプロジェクト管理
- Visual Studio 2022のみサポート

---

## 既存実装済み機能

### レンダリング基盤
- ✅ DirectX 12 初期化
- ✅ Deferred Rendering パイプライン
- ✅ PBR (Physically Based Rendering) シェーダー
- ✅ Cascaded Shadow Maps
- ✅ 一部のポストエフェクト（詳細は確認必要）
- ✅ BufferManager（Vertex/Index/Constant Buffer管理）
- ✅ RenderGraph（ClearPass実装済み）

### アーキテクチャ
- ✅ ECS (Entity Component System) - 一部実装
- ✅ Logger (spdlog + C++20 source_location)
- ✅ Expected<T, E> エラーハンドリング

### 外部ライブラリ
- ✅ ImGui（Git Submodule）
- ✅ spdlog（Git Submodule）
- ✅ DirectXTex（NuGet）

---

## 必要な追加機能（優先順位順）

### 🔴 Week 1-2: 基本レンダリング完成

#### 1. **ShaderManager** 🔴 高優先度
**目的**: HLSL シェーダーのコンパイル・管理
**機能**:
- HLSL → DXIL/SPIRV コンパイル
- ランタイムコンパイルとプリコンパイル両対応
- シェーダーバリエーション（#define による分岐）
- include 対応
- エラーメッセージの詳細表示
- ホットリロード（開発時に自動再コンパイル）

**実装アプローチ**:
```cpp
class ShaderManager {
    auto CompileFromFile(
        std::string_view path,
        std::string_view entryPoint,
        std::string_view target, // "vs_6_0", "ps_6_0" など
        std::span<const ShaderDefine> defines = {}
    ) -> Expected<ComPtr<ID3DBlob>, Error>;
    
    auto GetOrCompile(...) -> Expected<ComPtr<ID3DBlob>, Error>; // キャッシュ機能
    auto ReloadShader(std::string_view path) -> Expected<void, Error>; // ホットリロード
};
```

#### 2. **PrimitiveRenderer** 🔴 高優先度
**目的**: 三角形・基本図形の描画
**機能**:
- 三角形描画（最初の目標）
- Quad, Cube, Sphere などのプリミティブ
- ワイヤーフレーム描画
- デバッグ用ライン・ボックス描画

**実装アプローチ**:
```cpp
class PrimitiveRenderer {
    auto DrawTriangle(
        std::span<const Vertex> vertices,
        const Material& material,
        const Matrix4x4& transform
    ) -> Expected<void, Error>;
    
    auto DrawDebugLine(Vector3 start, Vector3 end, Color color) -> void;
    auto DrawDebugBox(const BoundingBox& box, Color color) -> void;
};
```

#### 3. **CommandListManager** 🔴 高優先度
**目的**: CommandList の生成・管理・並列記録対応
**機能**:
- CommandList プール
- マルチスレッド並列記録
- 自動リセット・再利用

**実装アプローチ**:
```cpp
class CommandListManager {
    auto AllocateCommandList(D3D12_COMMAND_LIST_TYPE type) 
        -> Expected<ID3D12GraphicsCommandList*, Error>;
    
    auto ExecuteCommandLists(std::span<ID3D12CommandList*> lists) -> void;
    auto ResetCommandList(ID3D12GraphicsCommandList* cmdList) -> Expected<void, Error>;
};
```

---

### 🟡 Week 3-4: モデルローディング

#### 4. **glTF Model Loader** 🟡 中優先度
**目的**: glTF 2.0 モデルの読み込み
**依存ライブラリ**: tinygltf（Git Submodule で追加）
**機能**:
- glTF/GLB ファイル読み込み
- Mesh データ → Vertex/Index Buffer 変換
- Material データ読み込み（PBR）
- Texture 読み込み
- ノード階層構造の読み込み

**実装アプローチ**:
```cpp
class ModelLoader {
    auto LoadFromFile(std::string_view path) -> Expected<Model, Error>;
};

struct Model {
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::vector<Texture> textures;
    SceneGraph sceneGraph; // ノード階層
};
```

#### 5. **TextureManager** 🟡 中優先度
**目的**: テクスチャの読み込み・管理
**機能**:
- PNG, JPEG, DDS, KTX2 読み込み
- Mipmap 自動生成
- 参照カウント管理
- 非同期ロード対応
- ホットリロード

**実装アプローチ**:
```cpp
class TextureManager {
    auto LoadTexture(std::string_view path) -> Expected<TextureHandle, Error>;
    auto GetTexture(TextureHandle handle) -> Expected<ID3D12Resource*, Error>;
    auto ReloadTexture(std::string_view path) -> Expected<void, Error>;
private:
    std::unordered_map<std::string, TextureHandle> m_cache;
    std::vector<ComPtr<ID3D12Resource>> m_textures;
};
```

---

### 🟡 Week 5-6: シーン管理

#### 6. **SceneGraph & Transform System** 🟡 中優先度
**目的**: 階層型シーングラフ
**機能**:
- 親子関係管理
- Transform 継承（親の Transform が子に影響）
- ワールド行列の自動計算

**実装アプローチ**:
```cpp
class SceneNode {
    auto AddChild(std::shared_ptr<SceneNode> child) -> void;
    auto SetLocalTransform(const Transform& transform) -> void;
    auto GetWorldTransform() const -> Matrix4x4; // 親のTransformを考慮
private:
    std::weak_ptr<SceneNode> m_parent;
    std::vector<std::shared_ptr<SceneNode>> m_children;
    Transform m_localTransform;
};
```

#### 7. **Multi-Scene Management** 🟡 中優先度
**目的**: 複数シーンの同時ロード・切り替え
**機能**:
- シーンの動的ロード/アンロード
- シーン間の切り替え
- バックグラウンドローディング

**実装アプローチ**:
```cpp
class SceneManager {
    auto LoadScene(std::string_view name) -> Expected<void, Error>;
    auto UnloadScene(std::string_view name) -> Expected<void, Error>;
    auto SwitchToScene(std::string_view name) -> Expected<void, Error>;
    auto GetActiveScene() -> Scene*;
};
```

---

### 🟢 Week 7-8: カメラ・入力・UI

#### 8. **Camera System** 🟢 中優先度
**目的**: FPS/Orbit/FreeFly カメラ
**機能**:
- 複数カメラサポート（メイン/ミニマップ/PiP）
- カメラエフェクト（Screen Shake, Motion Blur, DoF）
- カメラパスシステム（ベジェ曲線）
- 自動カメラシステム（障害物回避、ターゲット追跡）

**実装アプローチ**:
```cpp
class Camera {
    auto SetProjection(float fov, float aspect, float nearZ, float farZ) -> void;
    auto SetViewMatrix(const Matrix4x4& view) -> void;
    auto GetViewProjectionMatrix() const -> Matrix4x4;
};

class CameraController {
    virtual auto Update(float deltaTime) -> void = 0; // FPS/Orbit/FreeFly で継承
};

class FPSCameraController : public CameraController { /*...*/ };
class OrbitCameraController : public CameraController { /*...*/ };
```

#### 9. **Input Action Mapping** 🟢 中優先度
**目的**: 論理アクションと物理キーの分離
**機能**:
- "Jump" → スペース/Aボタン のマッピング
- キーボード・マウス・XInput ゲームパッド対応
- GetKeyDown/GetKeyUp バッファリング
- 振動サポート（XInput）

**実装アプローチ**:
```cpp
class InputManager {
    auto MapAction(std::string_view action, KeyCode key) -> void;
    auto MapAction(std::string_view action, GamepadButton button) -> void;
    
    auto IsActionPressed(std::string_view action) const -> bool;
    auto IsActionJustPressed(std::string_view action) const -> bool; // GetKeyDown
    
    auto SetVibration(uint32 controllerIndex, float leftMotor, float rightMotor) -> void;
};
```

#### 10. **ImGui Debug UI Integration** 🟢 中優先度
**目的**: デバッグ情報表示
**機能**:
- FPS/フレームタイム表示
- メモリ使用量表示
- GPU統計情報
- エンティティインスペクター
- シーン階層表示
- リアルタイムパラメータ調整

**実装アプローチ**:
```cpp
class DebugUI {
    auto ShowPerformanceWindow() -> void; // FPS, FrameTime
    auto ShowMemoryWindow() -> void; // CPU/GPU メモリ
    auto ShowEntityInspector(Entity entity) -> void; // ECS コンポーネント表示
    auto ShowSceneHierarchy() -> void; // SceneGraph 表示
};
```

#### 11. **Console System** 🟢 中優先度
**目的**: インゲームコンソール
**機能**:
- コマンド登録・実行
- オートコンプリート
- コマンド履歴

**実装アプローチ**:
```cpp
class Console {
    using CommandFunc = std::function<void(std::span<const std::string_view>)>;
    
    auto RegisterCommand(std::string_view name, CommandFunc func) -> void;
    auto ExecuteCommand(std::string_view commandLine) -> void;
};

// 使用例:
console.RegisterCommand("spawn", [](auto args) {
    auto entityName = args[0];
    // エンティティ生成
});
```

---

### 🔵 Week 9-12: ライティング・最適化

#### 12. **Dynamic Light Management** 🔵 中優先度
**目的**: ライトの動的追加・削除・移動
**機能**:
- Directional/Point/Spot Light サポート
- ライトカリング（可視ライトのみ処理）
- Deferred での多数ライト対応

**実装アプローチ**:
```cpp
class LightManager {
    auto AddLight(const Light& light) -> LightHandle;
    auto RemoveLight(LightHandle handle) -> void;
    auto UpdateLight(LightHandle handle, const Light& light) -> void;
    
    auto GetVisibleLights(const Frustum& frustum) -> std::span<const Light>;
};
```

#### 13. **Frustum & Occlusion Culling** 🔵 高優先度
**目的**: 描画オブジェクト削減
**機能**:
- Frustum Culling（視錐台外のオブジェクトを除外）
- Occlusion Culling（遮蔽物で隠れたオブジェクトを除外）
- Octree/BVH 空間分割

**実装アプローチ**:
```cpp
class CullingSystem {
    auto CullObjects(
        std::span<const RenderObject> objects,
        const Frustum& frustum
    ) -> std::vector<const RenderObject*>;
};
```

#### 14. **LOD System** 🔵 中優先度
**目的**: 距離に応じたメッシュ詳細度変更
**機能**:
- 複数LODレベルのサポート
- 距離ベースの自動切り替え

**実装アプローチ**:
```cpp
struct LODLevel {
    float distance;
    Mesh mesh;
};

class LODManager {
    auto SelectLOD(const std::vector<LODLevel>& levels, float distance) -> const Mesh&;
};
```

#### 15. **Instancing** 🔵 高優先度
**目的**: 同じメッシュを1回のドローコールで描画
**機能**:
- DrawInstanced サポート
- インスタンスバッファ管理

**実装アプローチ**:
```cpp
class InstancedRenderer {
    auto DrawInstanced(
        const Mesh& mesh,
        std::span<const Matrix4x4> instanceTransforms
    ) -> void;
};
```

#### 16. **Memory Tracker** 🔵 中優先度
**目的**: メモリリーク検出・アロケーション追跡
**機能**:
- カスタムアロケータ
- アロケーション統計
- CPU/GPU メモリ監視

**実装アプローチ**:
```cpp
class MemoryTracker {
    auto TrackAllocation(void* ptr, size_t size, const std::source_location& loc) -> void;
    auto TrackDeallocation(void* ptr) -> void;
    auto GetReport() -> MemoryReport;
};
```

---

### 🟣 Week 13-16: タスクシステム・並列化

#### 17. **Task System (Job System)** 🟣 高優先度
**目的**: CPU負荷分散
**機能**:
- ワーカースレッドプール
- タスクキュー
- 依存関係管理
- カリング・ECS更新・物理演算の並列化

**実装アプローチ**:
```cpp
class TaskSystem {
    auto SubmitTask(std::function<void()> task) -> TaskHandle;
    auto WaitForTask(TaskHandle handle) -> void;
    auto SubmitTaskWithDependency(
        std::function<void()> task,
        std::span<const TaskHandle> dependencies
    ) -> TaskHandle;
};
```

#### 18. **Parallel CommandList Recording** 🟣 高優先度
**目的**: 複数スレッドでCommandList生成
**機能**:
- スレッドごとの CommandList
- 描画コマンドの並列生成

**実装アプローチ**:
```cpp
// TaskSystem と CommandListManager を組み合わせ
taskSystem.SubmitTask([&]() {
    auto cmdList = commandListManager.AllocateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    // 描画コマンド記録
});
```

---

### 🟤 Week 17-20: オーディオ・物理・パーティクル

#### 19. **XAudio2 Audio System** 🟤 中優先度
**目的**: WAV/MP3再生
**機能**:
- 基本再生（BGM/SE）
- 3D Audio（位置情報、距離減衰、ドップラー）
- オーディオミキサー（BGM/SE/Voice別音量）
- オーディオエフェクト（リバーブ、エコー）

**実装アプローチ**:
```cpp
class AudioManager {
    auto LoadSound(std::string_view path) -> Expected<SoundHandle, Error>;
    auto PlaySound(SoundHandle handle, float volume = 1.0f) -> void;
    auto PlaySound3D(SoundHandle handle, Vector3 position) -> void;
    auto SetMasterVolume(AudioGroup group, float volume) -> void; // BGM/SE/Voice
};
```

#### 20. **PhysX Integration** 🟤 中優先度
**目的**: 物理演算
**機能**:
- 剛体シミュレーション
- コリジョン検出
- レイキャスト

#### 21. **GPU Particle System** 🟤 中優先度
**目的**: 大量パーティクル
**機能**:
- Compute Shader でパーティクル更新
- Instancing で描画

---

### 🟠 Week 21-24: ポストエフェクト・レイトレ

#### 22. **Post-Processing Stack** 🟠 中優先度
**目的**: フルスクリーンエフェクト
**機能**:
- Bloom
- Depth of Field (DoF)
- SSAO (Screen Space Ambient Occlusion)
- Tone Mapping
- Color Grading

#### 23. **Optional DXR Raytracing** 🟠 低優先度
**目的**: リアルタイムレイトレーシング
**機能**:
- RTX ON/OFF 切り替え
- レイトレーシングシャドウ
- レイトレーシング反射

---

## 実装されていない前提機能（Week 1-2で必要）

### ResourceManager (AssetManager)
**目的**: アセット一元管理
**機能**:
- テクスチャ・モデル・シェーダーを一元管理
- 自動キャッシング
- 参照カウント
- 遅延ロード

### DescriptorHeap Manager
**目的**: DirectX 12 の Descriptor Heap 管理
**機能**:
- CBV/SRV/UAV の動的割り当て
- RTV/DSV の管理

---

## メモリ保存時の注意

この仕様書は:
- **前回セッションの詳細ヒアリング結果を反映**
- **既存実装（BufferManager, RenderGraph, Logger, ECS）を考慮**
- **優先順位を Week 単位で整理**
- **中級者向け C++ エンジンとしての方向性を明確化**

次のセッションでは、この仕様書を参照して実装を進めること。
