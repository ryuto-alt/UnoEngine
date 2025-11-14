# UnoEngine - DirectX 12 Game Engine

UnoEngineは、モダンなC++20とDirectX 12を使用した、モジュラー設計のゲームエンジンです。

## 🎯 主な特徴

- **モジュラーアーキテクチャ**: 明確に分離された5つのコアモジュール
- **Entity Component System (ECS)**: データ指向設計による高性能なゲームオブジェクト管理
- **レンダーグラフ**: 自動リソースバリア管理と最適化
- **C++20**: Concepts、Ranges、constexprなど最新機能を活用
- **DirectX 12**: 低レベルグラフィックスAPIによる最大のパフォーマンス

## 📁 プロジェクト構造

```
UnoEngine/
├── Include/                   # ヘッダーファイル
│   ├── Core/                 # コアシステム
│   │   ├── CoreTypes.h       # 基本型定義とC++20 Concepts
│   │   ├── EntityComponentSystem.h  # ECS実装
│   │   ├── Components.h      # 標準コンポーネント定義
│   │   └── Engine.h          # メインエンジンクラス
│   ├── Graphics/             # グラフィックスAPI抽象化層
│   │   └── GraphicsDevice.h  # DirectX12デバイス管理
│   ├── Renderer/             # レンダリングシステム
│   │   └── RenderGraph.h     # レンダーグラフ実装
│   ├── Platform/             # プラットフォーム固有処理
│   │   └── Window.h          # ウィンドウ管理
│   └── Scene/                # シーン管理
├── Source/                   # 実装ファイル
│   ├── Core/
│   ├── Graphics/
│   │   └── GraphicsDevice.cpp
│   ├── Renderer/
│   ├── Platform/
│   └── Scene/
├── Shaders/                  # HLSL シェーダー
├── Assets/                   # ゲームアセット
│   ├── Textures/
│   ├── Models/
│   └── Materials/
├── Build/                    # ビルド出力
├── Intermediate/             # 中間ファイル
└── main.cpp                  # エントリーポイント
```

## 🏗️ アーキテクチャ概要

### 1. Core Module (コアモジュール)
- **EntityComponentSystem**: メモリ効率的なECS実装
- **Components**: Transform、Mesh、Material、Camera、Lightなど
- **Engine**: メインループとシステム統合

### 2. Graphics Module (グラフィックスモジュール)
- **GraphicsDevice**: DirectX 12デバイス、コマンドキュー、スワップチェーン管理
- リソース管理とGPU同期

### 3. Renderer Module (レンダラーモジュール)
- **RenderGraph**: レンダリングパスの依存関係管理
- 自動リソースバリア挿入
- リソースエイリアシング最適化

### 4. Platform Module (プラットフォームモジュール)
- **Window**: Win32ウィンドウ管理
- 入力処理（実装予定）

### 5. Scene Module (シーンモジュール)
- シーングラフ（実装予定）
- アセット管理（実装予定）

## 🛠️ 開発環境セットアップ

### 必要要件

- **Windows 10/11** (64-bit)
- **Visual Studio 2022** (17.0以降)
  - C++20サポート
  - Windows SDK 10.0.19041.0以降
- **DirectX 12対応GPU**

### セットアップ手順

#### 1. Visual Studio 2022のインストール

以下のワークロードをインストール：
- デスクトップ環境でのC++によるゲーム開発
- C++によるデスクトップ開発

以下の個別コンポーネントを追加：
- Windows 10 SDK (最新版)
- C++ CMake tools for Windows
- C++20標準ライブラリモジュール

#### 2. プロジェクトのビルド設定

Visual Studioでプロジェクトを開く：
```
UnoEngine.sln または UnoEngine.slnx
```

プロジェクト設定を確認：

**C/C++ → 全般:**
- C++言語標準: ISO C++20 Standard (/std:c++20)
- 追加のインクルードディレクトリ: `$(ProjectDir)Include`

**C/C++ → プリプロセッサ:**
- プリプロセッサの定義:
  ```
  _DEBUG (デバッグビルド)
  UNICODE
  _UNICODE
  WIN32_LEAN_AND_MEAN
  ```

**リンカー → システム:**
- サブシステム: Windows (/SUBSYSTEM:WINDOWS)

**リンカー → 入力:**
- 追加の依存ファイル:
  ```
  d3d12.lib
  dxgi.lib
  dxguid.lib
  d3dcompiler.lib
  ```

#### 3. DirectX 12 Agility SDKの設定（推奨）

最新のDirectX 12機能を使用するため、Agility SDKの使用を推奨：

1. NuGetパッケージマネージャーで以下をインストール：
   - `Microsoft.Direct3D.D3D12`

2. または、手動でダウンロード：
   - [DirectX Agility SDK](https://devblogs.microsoft.com/directx/directx12agility/)

#### 4. d3dx12.hのダウンロード

DirectX 12ヘルパーライブラリをダウンロード：

```
https://github.com/microsoft/DirectX-Headers
```

`include/directx/d3dx12.h`を`Include/`フォルダにコピー

### ビルド手順

1. **ソリューションを開く**
   ```
   Visual Studioで UnoEngine.sln を開く
   ```

2. **構成を選択**
   - Debug x64（開発時）
   - Release x64（リリース時）

3. **ビルド**
   ```
   Ctrl + Shift + B または ビルド → ソリューションのビルド
   ```

4. **実行**
   ```
   F5 または デバッグ → デバッグの開始
   ```

## 📝 使用例

```cpp
#include "Include/Core/Engine.h"
#include "Include/Core/Components.h"

class MyGame : public UnoEngine::Core::Engine
{
public:
    auto OnInitialize() -> bool override
    {
        // ECSコーディネーターを取得
        auto& ecs = GetECSCoordinator();

        // コンポーネントを登録
        ecs.RegisterComponent<TransformComponent>();
        ecs.RegisterComponent<MeshComponent>();

        // エンティティを作成
        Entity player = ecs.CreateEntity();

        // コンポーネントを追加
        TransformComponent transform{};
        transform.position = {0.0f, 0.0f, 0.0f};
        ecs.AddComponent(player, transform);

        return true;
    }

    auto OnUpdate(float deltaTime) -> void override
    {
        // ゲームロジックを更新
    }

    auto OnRender() -> void override
    {
        // レンダリング処理
    }
};

int main()
{
    MyGame game;
    EngineConfig config{};

    if (game.Initialize(config))
    {
        return game.Run();
    }

    return -1;
}
```

## 🎨 C++20機能の活用

### Concepts（コンセプト）
```cpp
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<Numeric T>
auto Add(T a, T b) -> T {
    return a + b;
}
```

### Ranges（範囲）
```cpp
// 将来の実装予定
auto activeEntities = entities
    | std::views::filter([](auto& e) { return e.isActive; })
    | std::views::transform([](auto& e) { return e.id; });
```

### constexpr
```cpp
[[nodiscard]] constexpr auto HasTexture() const noexcept -> bool
{
    return textureId != 0;
}
```

## 🔧 今後の実装予定

- [ ] シェーダーコンパイルシステム
- [ ] メッシュローダー（OBJ、FBX）
- [ ] テクスチャローダー（PNG、JPG、DDS）
- [ ] 物理エンジン統合
- [ ] オーディオシステム
- [ ] スクリプティングシステム
- [ ] エディターUI（ImGui）
- [ ] シーンシリアライゼーション

## 📚 参考資料

- [DirectX 12 Programming Guide](https://docs.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide)
- [C++20 Reference](https://en.cppreference.com/w/cpp/20)
- [Game Engine Architecture (Jason Gregory)](https://www.gameenginebook.com/)
- [Real-Time Rendering](https://www.realtimerendering.com/)

## 📄 ライセンス

このプロジェクトは個人学習・研究目的で開発されています。

## 🤝 コントリビューション

フィードバックや改善提案を歓迎します！

---

**UnoEngine** - Modern C++20 DirectX 12 Game Engine
