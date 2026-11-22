#pragma once
#include <vector>
#include "entt.hpp"
#include "Engine/ECS/Components/ControllerComponent.h"

class AdaptiveTriggerToolWindow
{
public:
    // registry から ControllerComponent を探して表示する
    void Draw(entt::registry& registry);

    bool isOpen = false; // メニューから開閉する

private:
    // ゾーンの状態
    struct ZoneState
    {
        bool  exists = false;
        float start = 0.0f;
        float end = 1.0f;
        std::vector<AdaptiveTrigger::CurvePoint> localPoints;
        int   selectedIndex = -1;
    };

    struct RangeDragState
    {
        enum class Mode { None, Left, Right, Move };
        Mode mode = Mode::None;
        float startMouseX = 0.0f;
        float startZoneStart = 0.0f;
        float startZoneEnd = 0.0f;
    };

    void DrawTriggerSection(const char* label, AdaptiveTrigger::TriggerEffect& effect, ZoneState& zone, RangeDragState& rangeDrag, float currentValue);
    void DrawRangeBar(ZoneState& zone, float width, RangeDragState& rangeDrag, float currentValue);
    void DrawGraph(std::vector<AdaptiveTrigger::CurvePoint>& points, int& selectedIndex, int& otherSelectedIndex, float localCurrentValue, bool showIndicator);
    void DrawSelectedPointPanel(std::vector<AdaptiveTrigger::CurvePoint>& points, int& selectedIndex);

    ZoneState leftZone;
    ZoneState rightZone;
    RangeDragState leftRangeDrag;
    RangeDragState rightRangeDrag;
};