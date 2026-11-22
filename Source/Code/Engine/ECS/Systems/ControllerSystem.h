#pragma once
#pragma comment(lib, "Xinput.lib")

#include <windows.h>
#include <Xinput.h>
#include <DualSenseWindows/IO.h>

#include <thread>
#include <mutex>
#include <atomic>

#include "entt.hpp"
#include "Engine/ECS/Components/ControllerComponent.h"
#include "Engine/ECS/Components/ControllerSettings.h"

class ControllerSystem
{
public:
    ControllerSystem();
    ~ControllerSystem();

    ControllerSystem(const ControllerSystem&) = delete;
    ControllerSystem& operator=(const ControllerSystem&) = delete;
    ControllerSystem(ControllerSystem&&) = delete;
    ControllerSystem& operator=(ControllerSystem&&) = delete;

    // 更新
    void Update(entt::registry& registry, float elapsedTime);

    // 設定への参照
    ControllerSettings& GetSettings() { return settings; }
    const ControllerSettings& GetSettings() const { return settings; }

private:
    // Xbox 入力をControllerComponentに書く
    void UpdateXboxInput(ControllerComponent& ctrl);

    // PS5 入力をControllerComponentに書く(共有バッファから書く)
    void UpdatePS5Input(const DS5W::DS5InputState& snapshot, ControllerComponent& ctrl);

	// トリガーのヒステリシス判定
    bool UpdateTriggerDigital(float triggerValue, float pressThreshold, float releaseThreshold, TriggerHysteresisState& state);

    // AdaptiveTrigger::TriggerEffectからDS5W::TriggerEffectへの変換
    void ApplyTriggerEffect(DS5W::TriggerEffect& dst, const AdaptiveTrigger::TriggerEffect& src, float currentPosition);

    // 2つのTriggerEffectが同じ内容かどうか比較する
    bool IsSameTriggerEffect(const DS5W::TriggerEffect& a, const DS5W::TriggerEffect& b);



    // 専用スレッドエントリーポイント(PS5との通信を繰り返す)
    void DS5ThreadFunc();

    // PS5 接続試行(専用スレッドから呼ぶ)
    void TryConnectPS5();

    // PS5 出力送信（専用スレッドから呼ぶ）
    void SendPS5Output(const AdaptiveTrigger::TriggerEffect& left, const AdaptiveTrigger::TriggerEffect& right, float leftPos, float rightPos);


    // Xbox
    bool         xboxConnected = false;
    XINPUT_STATE xboxState = {};

    // PS5専用スレッド
    std::thread       ds5Thread;
    std::atomic<bool> ds5ThreadRunning{ false };

    // PS5
    DS5W::DeviceContext  ds5Context = {};
    DS5W::DS5OutputState ds5OutputState = {};
    bool                 ds5Connected = false;
    float                ds5ReconnectTimer = 0.0f;
    static constexpr float ds5ReconnectInterval = 3.0f;
    DS5W::TriggerEffect lastLeftEffect = {};
    DS5W::TriggerEffect lastRightEffect = {};

    // メインスレッドと専用のスレッドの橋渡し
    std::mutex ds5Mutex;

    // 専用スレッドからメインスレッドへ最新の入力状態
    bool                 ds5ConnectedShared = false;
    DS5W::DS5InputState  ds5InputStateShared = {};

    // メインスレッド空専用スレッドへ行いたいトリガーエフェクト
    AdaptiveTrigger::TriggerEffect ds5RequestedLeftEffect = {};
    AdaptiveTrigger::TriggerEffect ds5RequestedRightEffect = {};

    // 設定値
    ControllerSettings settings;

};