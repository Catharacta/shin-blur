# 改訂版 要件定義書：Windows 11 向け Blur ライブラリ（C/C++ DLL） — 最終版

作成日: 2025-12-24  
対象: Windows 11 x64（Tauri アプリから呼び出す前提）  
責務: 与えられたウィンドウハンドル（HWND）に対して「背後をぼかす（blur-behind）」効果を適用／解除することのみ。ウィンドウ選定・ホットキー等の制御はアプリ側（Tauri）が行う。

本書は実装に移れるレベルでの確定版要件定義であり、API 呼び出し規約・エラー体系・同期モデル・バイナリレイアウト・テスト SLO・フォールバック方針などを明確に定義しています。ソースコードは含みません。

---

## ハイライト（確定事項）
- 呼び出し規約: extern "C" + WINAPI（__stdcall）を採用。文字列は UTF-8。
- 同期モデル: MVP では `apply` / `clear` は同期的に完了を返す（タイムアウト指定可能）。
- エラー設計: 数値コードを用いる（0 = SUCCESS）。get_last_error_message による詳細取得・free_string による解放ルールを明示。
- EffectParams: 構造体バイナリレイアウト（バージョン付）を定義。
- init() は capability ビットを返し、実行時に使用可能な手段を明示。

---

## 1. 基本方針（確定）
- ライブラリは「ぼかし（blur-behind）の適用／解除のみ」を行う。ウィンドウ選定、ホットキー、UI、ポリシーはアプリ（Tauri）側の責務。
- インターフェースは C ABI（extern "C"）でエクスポートし、呼び出し規約は WINAPI を採用する。
- 文字列は UTF-8 で受け渡し、ライブラリが割り当てた文字列は `free_string()` で解放する仕様とする。
- MVP の `apply` / `clear` は同期的 API とする。長時間処理の懸念がある場合は `timeout_ms` 引数で上限を指定できる。

---

## 2. ライフサイクルと主要 API（決定）
注：関数名と引数は実装時に C シグネチャで確定する。ここでは機能仕様を示す。

必須 API（MVP）
- init(out uint32_t *capabilities) -> int32_t  
  - 内部初期化と使用可能手段の検出。成功時は capabilities ビットを設定して返す（0 = SUCCESS）。
- shutdown() -> void  
  - リソース解放。オプションフラグに従い既適用の blur を解除するか選択可能。
- apply_blur_to_window(uintptr_t window_handle, const EffectParams* params, uint32_t timeout_ms) -> int32_t  
  - 指定ウィンドウに blur を適用。同期 API。`timeout_ms` で最大待機時間指定（0 = デフォルトSLO）。
- clear_blur_from_window(uintptr_t window_handle, uint32_t timeout_ms) -> int32_t  
  - 指定ウィンドウの blur を解除し、可能な限り元状態へ復元。同期 API。

補助 API（推奨）
- restore_all() -> int32_t  
  - ライブラリ管理下の全適用を解除。
- get_currently_blurred_list(char** out_json_utf8) -> int32_t  
  - 適用中ウィンドウ一覧を JSON 文字列で返す（`free_string()` で解放）。
- get_last_error_message(char** out_utf8) -> int32_t  
  - 直近の詳細エラーを UTF-8 文字列で返す（`free_string()` で解放）。
- get_version(char** out_utf8) -> int32_t  
  - semver 形式のバージョン文字列を返す（`free_string()` で解放）。
- free_string(char* ptr) -> void  
  - ライブラリ割当て文字列の解放ユーティリティ。

ログ・診断 API（推奨）
- set_log_level(int level) -> void  (0=ERROR,1=WARN,2=INFO,3=DEBUG)
- set_log_callback(void (*callback)(int level, const char* utf8_msg, void* user_data), void* user_data) -> void

すべての外部ポインタ引数は NULL チェックを行い、無効値時は `INVALID_HANDLE` などのエラーを返す。

---

## 3. EffectParams — バイナリ仕様（確定）
バージョン付き固定レイアウト構造体を定義（例: `EffectParams_V1`）。

フィールド（固定サイズ）
- uint32_t struct_version;      // = 1
- float    intensity;           // 0.0 .. 1.0（必須）
- uint32_t color_argb;          // 0xAARRGGBB（0 = no override）
- uint8_t  animate;             // 0/1
- uint32_t animation_ms;        // ミリ秒（0 = デフォルト）
- uint32_t reserved_flags;      // 拡張用
- uint8_t  reserved_padding[4]; // アラインメント

設計ルール：
- リトルエンディアン、固定サイズ。将来 struct_version を上げて互換性を保つ。

---

## 4. エラーコード一覧（確定）
- 0  SUCCESS  
- 1  NOT_INITIALIZED  
- 2  INVALID_HANDLE  
- 3  PERMISSION_DENIED  
- 4  API_UNSUPPORTED  
- 5  TIMEOUT  
- 6  OUT_OF_MEMORY  
- 7  INTERNAL_ERROR  
- 8  INVALID_PARAMS  
- 9  ALREADY_APPLIED

各エラーは数値コードと併せて `get_last_error_message()` で詳細の UTF-8 文字列を取得可能。

---

## 5. 実行時 capability（init が返すビット）
ビット定義（例）
- CAP_SETWINDOWCOMPOSITION (0x1)  
- CAP_DWM_BLUR            (0x2)  
- CAP_OVERLAY_FALLBACK    (0x4)  
- CAP_COLOR_CONTROL       (0x8)  
- CAP_ANIMATION_CONTROL   (0x10)

`init()` はこれらのビットを組み合わせて返す。アプリは capability を参照して UI 表示や挙動を決定できる。

---

## 6. 同期モデルと性能目標（SLO）
- 同期モデル（MVP）: `apply` / `clear` は完了までブロックし、成功/失敗コードを返す。
- デフォルト SLO（目標）
  - 単一ウィンドウ apply: 95パーセンタイル < 300 ms
  - 単一ウィンドウ clear: 95パーセンタイル < 200 ms
- タイムアウト: `timeout_ms` 引数で上限を指定。タイムアウト時は `TIMEOUT` を返す。

将来的に非同期 API を追加可能（コールバックまたはイベント方式）。

---

## 7. メモリ所有権と文字列扱い
- ライブラリが割り当てて返す文字列は必ず `free_string()` で解放すること。これにより言語境界（Rust/JS）での管理を明確化。
- 外部から渡される文字列は UTF-8（必要時ライブラリ内で UTF-16 へ変換）。

---

## 8. スレッド & 再入設計
- 全公開 API はスレッドセーフとする（内部同期を実装）。
- 同一 HWND に対する並列 apply/clear は内部で直列化する。並列性は受け付けるが、同一ウィンドウの操作は順次実行される。

---

## 9. 復元ポリシー（clear / restore）
- 元状態は可能な限り保存して復元を試みる。OS制約で取得不可の場合は「ぼかし無効化（ACCENT_DISABLED 相当）」して見た目を戻す。
- ウィンドウが既に閉じられている場合は NO-OP として `SUCCESS` を返す。
- 復元不能や部分復元の状況は `get_last_error_message()` で報告する。

---

## 10. フォールバック戦略（優先順）
1. SetWindowCompositionAttribute（ACCENT 系） — 優先手段  
2. DWM blur-behind — 代替手段（パラメータ制限あり）  
3. オーバーレイ方式 — 最終手段（実装コスト高、入力透過や Z-order 管理の考慮が必要）

`init()` の capability ビットに基づき手段を選択する。オーバーレイ方式はプライバシーやパフォーマンス面の要件を満たす場合のみに限定する。

---

## 11. ロギング・診断
- `set_log_level` と `set_log_callback` を提供し、詳細診断やトラブルシュートを可能にする。
- `init()` 実行時の診断情報として OS ビルド、capabilities、API チェック結果をログ出力可能（オプション）。

---

## 12. テスト・自動化方針
必須テスト（MVP）
- 正常系: 有効 HWND に対して apply → 目視/自動でぼやけが確認できること、clear → 元に戻ること
- 異常系: 無効 HWND で `INVALID_HANDLE`、権限不足で `PERMISSION_DENIED`、API 未対応で `API_UNSUPPORTED` を返すこと
- 回復性: 適用中にウィンドウ閉鎖→安定していること
- パフォーマンス: SLO に準拠していること（95% < 300ms など）

自動化提案
- スクリーンショット差分テスト（CI 回帰用）
- API モックによる単体テスト（エラーパスの自動化）
- GPU/描画の互換性テストは実機（Windows 11）で実施し、CI にセルフホストランナーを用意することを推奨

---

## 13. CI / ビルド / 開発環境
- 推奨ツールチェーン: MSVC（Visual Studio 2022+） + Windows 11 SDK（明記）
- CI: GitHub Actions（windows-latest）でビルド・ユニットテスト。描画互換テストはセルフホスト Windows 11 ランナーで実行することを推奨。
- リリース: Release ビルドを署名付きで出力する手順を CI に組み込む。

---

## 14. 配布・バージョニング方針
- SemVer を採用（major = 非互換、minor = 後方互換追加、patch = バグ修正）。
- リリースノートに capabilities の変更と既知の制限を明記する。
- バイナリ配布時はコード署名を強く推奨。

---

## 15. セキュリティ・プライバシー配慮
- 背景をキャプチャして処理する方式は MVP では避け、合成属性変更を優先する（キャプチャを用いる場合はユーザー同意とプライバシー考慮を必須とする）。
- DLL 配布は署名推奨。配布に関するセキュリティ注意事項をドキュメント化する。

---

## 16. ドキュメント（納品物）
必須ドキュメント：
- API リファレンス（関数シグネチャ、引数、戻り値、エラーコード、メモリ所有権）
- ビルド手順・開発者向けドキュメント
- 配布手順（署名、依存）
- Tauri 統合ガイド（hwnd の取得、動的ロード、非同期ラップのベストプラクティス）
- テスト手順書（自動化含む）
- リリースノート（capabilities、既知の制限）

---

## 17. スケジュール（最終版・概算）
- フェーズ0（準備）: 1日 — 要件確定、環境セットアップ、API 可用性確認  
- フェーズ1（コア実装）: 4–8日 — init/shutdown/capability 検出、apply/clear 実装、EffectParams 対応、エラー体系  
- フェーズ2（検証）: 3–6日 — Windows 11 上での検証、SLO 測定、フォールバック確認  
- フェーズ3（ドキュメント・パッケージ）: 1–2日 — API ドキュメント、署名、配布パッケージ作成

（フォールバックの追加や追加テストが必要な場合はスケジュール延伸）

---

## 18. リスクと対策（最終版）
- 低レベル API の非公式挙動（将来変更リスク）  
  - 対策: 実行時 capability 検出とフォールバック、init が非対応を返す場合は安全に失敗する設計
- 他プロセスウィンドウ改変による互換性問題  
  - 対策: 入力検証、例外抑制、広範な互換性テスト
- パフォーマンス悪化（多数ウィンドウ）  
  - 対策: バッチ処理・デバウンス・非同期化の検討

---

## 19. 決定済み推奨事項（実装方針として確定）
- 同期モデルの採用（MVP）: `apply/clear` は同期 API とし、`timeout_ms` をサポートする。  
- 呼び出し規約と文字列: extern "C" + WINAPI、文字列は UTF-8、ライブラリ割当て文字列は `free_string()` で解放する。  
- init() の capability ビット採用: 実行時に利用可能手段を返す。アプリはこれに基づき UI/フォールバックを行う。  
- 必須 API セット（MVP）: `init`, `shutdown`, `apply_blur_to_window`, `clear_blur_from_window`, `get_last_error_message`, `free_string`, `get_version` を実装必須とする。補助 API（`restore_all`, `get_currently_blurred_list`, ログ API）は推奨実装とする。  
- テスト SLO（目標応答時間）: apply 95% < 300 ms、clear 95% < 200 ms を目標とする。  

これらは実装方針として確定済みの推奨事項であり、実装はこの方針に従って行われます。

---

## 20. 次のアクション（実行フロー）
1. 開発者は本ドキュメントに従いフェーズ0 の API 可用性検査を実施し、`init()` が返す capability を確定する。  
2. フェーズ1（コア実装）に着手する。実装に合わせて API シグネチャを正式化し、ヘッダ／インポート定義を作成する。  
3. フェーズ2（検証）で SLO 検証、互換性テスト、フォールバック検証を行い、問題がなければドキュメントを完成させる。  
4. フェーズ3 で署名・リリースパッケージを作成する。

---

## 21. 備考
- 本要件定義は「ライブラリはぼかし効果だけを担当し、ウィンドウの選定や UI は Tauri 側で行う」という前提で作成されています。実装中に新たな技術的制約が発見された場合は、フォールバックの具体実装方針（オーバーレイ等）を別途追加ドキュメントとして取りまとめます。

---

（以上）