#pragma once
#include "entt.hpp"
#include "Game/ECS/Components/GameInputComponent.h"
#include "Engine/ECS/Components/ControllerComponent.h"
#include "Engine/ECS/Components/ControllerSettings.h"

class GameInputSystem
{
public:
    GameInputSystem() = default;
    ~GameInputSystem() = default;

    GameInputSystem(const GameInputSystem&) = delete;
    GameInputSystem& operator=(const GameInputSystem&) = delete;
    GameInputSystem(GameInputSystem&&) = delete;
    GameInputSystem& operator=(GameInputSystem&&) = delete;

    // 更新
    // ControllerComponent + ControllerSettings をもとに GameInputComponent に変換する
    void Update(entt::registry& registry, const ControllerSettings& settings);

private:
    // デッドゾーン適用（スティック用）
    float ApplyStickDeadzone(float value, float deadzone);

    // デッドゾーン適用（トリガー用）
    float ApplyTriggerDeadzone(float value, float deadzone);

	// ボタン状態の更新
    void UpdateButtonState(TriggerState& state, bool isHeldNow);
    void UpdateButtonState(ButtonState& state, bool isHeldNow);
};