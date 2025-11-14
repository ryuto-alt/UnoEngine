# UnoEngine ビルドガイド

## 🚀 クイックスタート

### 前提条件チェックリスト

- [ ] Windows 10/11 (64-bit)
- [ ] Visual Studio 2022 (17.0以降)
- [ ] Windows SDK 10.0.19041.0以降
- [ ] DirectX 12対応GPU

### ステップ1: 必要なファイルのダウンロード

#### d3dx12.h のダウンロード

DirectX 12ヘルパーライブラリが必要です：

1. [DirectX-Headers GitHub](https://github.com/microsoft/DirectX-Headers)にアクセス
2. `include/directx/d3dx12.h`をダウンロード
3. プロジェクトの`Include/`フォルダに配置

または、以下のコマンドでダウンロード：

```bash
# PowerShellで実行
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/microsoft/DirectX-Headers/main/include/directx/d3dx12.h" -OutFile "Include/d3dx12.h"
```

### ステップ2: Visual Studioプロジェクト設定

#### プロジェクトプロパティを開く

1. Visual Studioで`UnoEngine.vcxproj`を開く
2. プロジェクトを右クリック → **プロパティ**

#### 構成プロパティ → 全般

```
C++言語標準: ISO C++20 Standard (/std:c++20)
Windows SDK バージョン: 10.0 (最新インストール済みバージョン)
```

#### C/C++ → 全般

```
追加のインクルードディレクトリ:
$(ProjectDir)Include
```

警告レベル: レベル4 (/W4)

#### C/C++ → 言語

```
C++言語標準: ISO C++20 Standard (/std:c++20)
準拠モード: はい (/permissive-)
```

#### C/C++ → プリプロセッサ

**デバッグ構成:**
```
_DEBUG
UNICODE
_UNICODE
WIN32_LEAN_AND_MEAN
```

**リリース構成:**
```
NDEBUG
UNICODE
_UNICODE
WIN32_LEAN_AND_MEAN
```

#### C/C++ → コード生成

**デバッグ構成:**
```
ランタイムライブラリ: マルチスレッドデバッグDLL (/MDd)
```

**リリース構成:**
```
ランタイムライブラリ: マルチスレッドDLL (/MD)
最適化: 速度を優先する最適化 (/O2)
```

#### リンカー → システム

```
サブシステム: Windows (/SUBSYSTEM:WINDOWS)
```

#### リンカー → 入力

```
追加の依存ファイル:
d3d12.lib
dxgi.lib
dxguid.lib
d3dcompiler.lib
kernel32.lib
user32.lib
gdi32.lib
```

### ステップ3: ソースファイルの追加

Visual Studioのソリューションエクスプローラーで、以下のファイルがプロジェクトに含まれているか確認：

#### ヘッダーファイル

```
Include/Core/
  - CoreTypes.h
  - EntityComponentSystem.h
  - Components.h
  - Engine.h

Include/Graphics/
  - GraphicsDevice.h

Include/Renderer/
  - RenderGraph.h

Include/Platform/
  - Window.h
```

#### ソースファイル

```
Source/Core/
  - Engine.cpp

Source/Graphics/
  - GraphicsDevice.cpp

Source/Platform/
  - Window.cpp

main.cpp
```

#### シェーダーファイル

```
Shaders/
  - BasicVertex.hlsl
  - BasicPixel.hlsl
```

### ステップ4: ビルド

#### デバッグビルド

1. ソリューション構成を**Debug**に設定
2. プラットフォームを**x64**に設定
3. **ビルド** → **ソリューションのビルド** (Ctrl+Shift+B)

#### リリースビルド

1. ソリューション構成を**Release**に設定
2. プラットフォームを**x64**に設定
3. **ビルド** → **ソリューションのビルド**

### ステップ5: 実行

F5キーを押すか、**デバッグ** → **デバッグの開始**

## ⚠️ よくあるエラーと解決方法

### エラー1: d3dx12.h が見つからない

```
fatal error C1083: インクルード ファイルを開けません。'd3dx12.h': No such file or directory
```

**解決方法:**
- ステップ1のd3dx12.hダウンロード手順を実行
- `Include/d3dx12.h`が存在することを確認

### エラー2: Windows SDK が見つからない

```
error MSB8036: Windows SDK バージョン 10.0 が見つかりませんでした
```

**解決方法:**
1. Visual Studio Installerを起動
2. **変更** → **個別のコンポーネント**
3. **Windows 10 SDK (10.0.19041.0以降)** をインストール

### エラー3: C++20がサポートされていない

```
error C7525: inline variables require at least '/std:c++17'
```

**解決方法:**
- プロジェクトプロパティでC++言語標準を`/std:c++20`に設定

### エラー4: DirectX 12ライブラリのリンクエラー

```
error LNK2019: 未解決の外部シンボル D3D12CreateDevice
```

**解決方法:**
- リンカーの追加の依存ファイルに`d3d12.lib`が含まれているか確認

### エラー5: エンティティが定義されていない

```
identifier "Entity" is undefined
```

**解決方法:**
- `using namespace UnoEngine::Core::ECS;`を追加

## 🔧 推奨設定

### デバッグ設定

デバッグビルドでは以下を有効にすることを推奨：

```cpp
config.graphicsConfig.enableDebugLayer = true;
config.graphicsConfig.enableGpuValidation = true;  // パフォーマンス影響大
```

### リリース設定

リリースビルドではデバッグレイヤーを無効に：

```cpp
config.graphicsConfig.enableDebugLayer = false;
config.graphicsConfig.enableGpuValidation = false;
```

## 📁 ビルド出力

ビルドが成功すると、以下のディレクトリに実行ファイルが生成されます：

```
x64/Debug/UnoEngine.exe      (デバッグビルド)
x64/Release/UnoEngine.exe    (リリースビルド)
```

## 🎯 次のステップ

ビルドが成功したら：

1. **サンプルの実行**: main.cppのサンプルアプリケーションを実行
2. **独自アプリの作成**: `Engine`クラスを継承して独自のゲームを作成
3. **機能の追加**: メッシュローダー、テクスチャシステムなどを実装

## 📞 サポート

問題が解決しない場合は、以下を確認：

1. Visual Studio 2022が最新版か
2. Windows SDKが正しくインストールされているか
3. DirectX 12対応GPUが搭載されているか
4. すべての必要なファイルがプロジェクトに含まれているか

---

**Happy Coding with UnoEngine!** 🎮
