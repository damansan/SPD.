#include "AdaptiveTriggerToolWindow.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace
{
    constexpr ImU32 AccentColor = IM_COL32(66, 150, 250, 255);   // ImGuiらしい青
    constexpr ImU32 IndicatorColor = IM_COL32(255, 70, 70, 255); // 現在地の赤線
}

void AdaptiveTriggerToolWindow::Draw(entt::registry& registry)
{
    if (!isOpen) return;

    if (!ImGui::Begin("Adaptive Trigger Tool", &isOpen))
    {
        ImGui::End();
        return;
    }

    // 最初に見つかったコントローラーを編集対象にする
    ControllerComponent* found = nullptr;
    auto view = registry.view<ControllerComponent>();
    for (auto entity : view)
    {
        found = &view.get<ControllerComponent>(entity);
        break;
    }

    if (!found)
    {
        ImGui::TextDisabled("ControllerComponent を持つエンティティがありません");
        ImGui::End();
        return;
    }
    ControllerComponent& ctrl = *found;

    if (ImGui::BeginTable("##layout", 2, ImGuiTableFlags_None))
    {
        ImGui::TableSetupColumn("main", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("panel", ImGuiTableColumnFlags_WidthFixed, 160.0f);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        DrawTriggerSection("L2", ctrl.leftTriggerEffect, leftZone, leftRangeDrag, ctrl.leftTrigger);
        ImGui::Spacing();
        DrawTriggerSection("R2", ctrl.rightTriggerEffect, rightZone, rightRangeDrag, ctrl.rightTrigger);

        ImGui::TableSetColumnIndex(1);
        if (leftZone.selectedIndex >= 0)
        {
            DrawSelectedPointPanel(leftZone.localPoints, leftZone.selectedIndex);
        }
        else if (rightZone.selectedIndex >= 0)
        {
            DrawSelectedPointPanel(rightZone.localPoints, rightZone.selectedIndex);
        }
        else
        {
            ImGui::TextDisabled("ポイントを選択してください");
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void AdaptiveTriggerToolWindow::DrawTriggerSection(const char* label, AdaptiveTrigger::TriggerEffect& effect, ZoneState& zone, RangeDragState& rangeDrag, float currentValue)
{
    ImGui::PushID(label);

    ImGui::Text(label);

    if (!zone.exists)
    {
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80.0f + ImGui::GetCursorPosX());
        if (ImGui::Button("+ ゾーン追加"))
        {
            zone.exists = true;
            zone.start = 0.0f;
            zone.end = 1.0f;
            zone.localPoints = { {0.0f, 0.5f}, {1.0f, 0.5f} };
        }
    }
    else
    {
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f + ImGui::GetCursorPosX());
        if (ImGui::Button("削除"))
        {
            zone.exists = false;
            zone.selectedIndex = -1;
        }
    }

    float width = ImGui::GetContentRegionAvail().x;

    DrawRangeBar(zone, width, rangeDrag, currentValue);

    ImGui::Spacing();

    if (zone.exists)
    {
        bool withinZone = (currentValue >= zone.start && currentValue <= zone.end && zone.end > zone.start);
        float localCurrent = withinZone ? (currentValue - zone.start) / (zone.end - zone.start) : 0.0f;

        int dummyOther = -1;
        DrawGraph(zone.localPoints, zone.selectedIndex, dummyOther, localCurrent, withinZone);
    }

    auto& points = effect.forcePoints;
    points.clear();
    if (zone.exists)
    {
        for (const auto& lp : zone.localPoints)
        {
            float absPos = zone.start + lp.position * (zone.end - zone.start);
            points.push_back({ absPos, lp.value });
        }
    }

    ImGui::PopID();
}

void AdaptiveTriggerToolWindow::DrawRangeBar(ZoneState& zone, float width, RangeDragState& rangeDrag, float currentValue)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    const float height = 24.0f;
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(60, 60, 60, 255), 4.0f);

    ImGui::InvisibleButton("##rangeBar", ImVec2(width, height));
    bool barClicked = ImGui::IsItemClicked();
    bool barActive = ImGui::IsItemActive();
    ImVec2 mousePos = ImGui::GetMousePos();

    if (zone.exists)
    {
        float x1 = pos.x + zone.start * width;
        float x2 = pos.x + zone.end * width;

        drawList->AddRectFilled(ImVec2(x1, pos.y), ImVec2(x2, pos.y + height), AccentColor);

        const float handleHalf = 3.0f;
        drawList->AddRectFilled(ImVec2(x1 - handleHalf, pos.y - 3.0f), ImVec2(x1 + handleHalf, pos.y + height + 3.0f), IM_COL32(255, 255, 255, 255));
        drawList->AddRectFilled(ImVec2(x2 - handleHalf, pos.y - 3.0f), ImVec2(x2 + handleHalf, pos.y + height + 3.0f), IM_COL32(255, 255, 255, 255));

        const float pickRange = 8.0f;

        if (barClicked)
        {
            if (std::abs(mousePos.x - x1) <= pickRange)
            {
                rangeDrag.mode = RangeDragState::Mode::Left;
            }
            else if (std::abs(mousePos.x - x2) <= pickRange)
            {
                rangeDrag.mode = RangeDragState::Mode::Right;
            }
            else if (mousePos.x >= x1 && mousePos.x <= x2)
            {
                rangeDrag.mode = RangeDragState::Mode::Move;
                rangeDrag.startMouseX = mousePos.x;
                rangeDrag.startZoneStart = zone.start;
                rangeDrag.startZoneEnd = zone.end;
            }
            else
            {
                rangeDrag.mode = RangeDragState::Mode::None;
            }
        }

        if (barActive)
        {
            float mouseNormX = std::clamp((mousePos.x - pos.x) / width, 0.0f, 1.0f);

            if (rangeDrag.mode == RangeDragState::Mode::Left)
            {
                zone.start = std::clamp(mouseNormX, 0.0f, zone.end - 0.01f);
            }
            else if (rangeDrag.mode == RangeDragState::Mode::Right)
            {
                zone.end = std::clamp(mouseNormX, zone.start + 0.01f, 1.0f);
            }
            else if (rangeDrag.mode == RangeDragState::Mode::Move)
            {
                float rawDelta = (mousePos.x - rangeDrag.startMouseX) / width;
                float zoneWidth = rangeDrag.startZoneEnd - rangeDrag.startZoneStart;
                float clampedDelta = std::clamp(rawDelta, -rangeDrag.startZoneStart, 1.0f - rangeDrag.startZoneEnd);
                zone.start = rangeDrag.startZoneStart + clampedDelta;
                zone.end = rangeDrag.startZoneStart + clampedDelta + zoneWidth;
            }
        }

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            rangeDrag.mode = RangeDragState::Mode::None;
        }
    }

    // 現在のトリガー押し込み位置（赤線）
    float curX = pos.x + std::clamp(currentValue, 0.0f, 1.0f) * width;
    drawList->AddLine(ImVec2(curX, pos.y - 4.0f), ImVec2(curX, pos.y + height + 4.0f), IndicatorColor, 2.0f);

    ImGui::Dummy(ImVec2(width, height));

    ImGui::TextDisabled("0");
    ImGui::SameLine(width - 24.0f);
    ImGui::TextDisabled("255");
}

void AdaptiveTriggerToolWindow::DrawGraph(std::vector<AdaptiveTrigger::CurvePoint>& points, int& selectedIndex, int& otherSelectedIndex, float localCurrentValue, bool showIndicator)
{
    const float margin = 8.0f;

    ImVec2 graphSize(ImGui::GetContentRegionAvail().x, 150.0f);
    ImVec2 graphPos = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton("##graphBg", graphSize);
    bool bgClicked = ImGui::IsItemClicked();
    bool bgActive = ImGui::IsItemActive();
    ImVec2 mousePos = ImGui::GetMousePos();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRect(graphPos, ImVec2(graphPos.x + graphSize.x, graphPos.y + graphSize.y), IM_COL32(150, 150, 150, 255));

    float innerX = graphPos.x + margin;
    float innerWidth = graphSize.x - margin * 2.0f;
    float innerY = graphPos.y + margin;
    float innerHeight = graphSize.y - margin * 2.0f;

    auto ToScreen = [&](const AdaptiveTrigger::CurvePoint& p) -> ImVec2
        {
            return ImVec2(innerX + p.position * innerWidth, innerY + (1.0f - p.value) * innerHeight);
        };
    auto FromScreen = [&](ImVec2 screen) -> AdaptiveTrigger::CurvePoint
        {
            AdaptiveTrigger::CurvePoint p;
            p.position = std::clamp((screen.x - innerX) / innerWidth, 0.0f, 1.0f);
            p.value = std::clamp(1.0f - (screen.y - innerY) / innerHeight, 0.0f, 1.0f);
            return p;
        };

    for (size_t i = 0; i + 1 < points.size(); ++i)
    {
        drawList->AddLine(ToScreen(points[i]), ToScreen(points[i + 1]), AccentColor, 2.0f);
    }

    for (size_t i = 0; i < points.size(); ++i)
    {
        ImVec2 p = ToScreen(points[i]);
        bool isSelected = (static_cast<int>(i) == selectedIndex);
        float radius = isSelected ? 7.0f : 5.0f;
        drawList->AddCircleFilled(p, radius, AccentColor);
        if (isSelected)
        {
            drawList->AddCircle(p, radius + 2.0f, IM_COL32(255, 255, 255, 255), 0, 2.0f);
        }
    }

    // 現在地の赤線（ゾーン内にいる時だけ表示）
    if (showIndicator)
    {
        float curX = innerX + std::clamp(localCurrentValue, 0.0f, 1.0f) * innerWidth;
        drawList->AddLine(ImVec2(curX, graphPos.y), ImVec2(curX, graphPos.y + graphSize.y), IndicatorColor, 2.0f);
    }

    const float pickRadius = 8.0f;
    const int lastIndex = static_cast<int>(points.size()) - 1;

    if (bgClicked)
    {
        int picked = -1;
        for (size_t i = 0; i < points.size(); ++i)
        {
            ImVec2 p = ToScreen(points[i]);
            float dx = mousePos.x - p.x;
            float dy = mousePos.y - p.y;
            if ((dx * dx + dy * dy) <= (pickRadius * pickRadius))
            {
                picked = static_cast<int>(i);
                break;
            }
        }

        if (picked == -1)
        {
            AdaptiveTrigger::CurvePoint newPoint = FromScreen(mousePos);

            // 末尾の境界点(100%)の手前に挿入してから、中間部分だけ並び替える
            points.insert(points.end() - 1, newPoint);
            if (points.size() > 2)
            {
                std::sort(points.begin() + 1, points.end() - 1,
                    [](const AdaptiveTrigger::CurvePoint& a, const AdaptiveTrigger::CurvePoint& b)
                    {
                        return a.position < b.position;
                    });
            }

            // 挿入したポイントの新しいindexを探す
            for (size_t i = 0; i < points.size(); ++i)
            {
                if (points[i].position == newPoint.position && points[i].value == newPoint.value)
                {
                    picked = static_cast<int>(i);
                    break;
                }
            }
        }

        selectedIndex = picked;
        otherSelectedIndex = -1;
    }

    if (bgActive && selectedIndex >= 0 && selectedIndex < static_cast<int>(points.size()))
    {
        // 先頭・末尾（ゾーンの境界）はX固定、Yだけ動かす
        if (selectedIndex == 0)
        {
            points[selectedIndex].value = FromScreen(mousePos).value;
            points[selectedIndex].position = 0.0f;
        }
        else if (selectedIndex == lastIndex)
        {
            points[selectedIndex].value = FromScreen(mousePos).value;
            points[selectedIndex].position = 1.0f;
        }
        else
        {
            points[selectedIndex] = FromScreen(mousePos);
        }
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        selectedIndex > 0 && selectedIndex < lastIndex) // 境界点は並び替え不要
    {
        AdaptiveTrigger::CurvePoint selected = points[selectedIndex];
        std::sort(points.begin() + 1, points.end() - 1,
            [](const AdaptiveTrigger::CurvePoint& a, const AdaptiveTrigger::CurvePoint& b)
            {
                return a.position < b.position;
            });
        for (size_t i = 1; i + 1 < points.size(); ++i)
        {
            if (points[i].position == selected.position && points[i].value == selected.value)
            {
                selectedIndex = static_cast<int>(i);
                break;
            }
        }
    }
}

void AdaptiveTriggerToolWindow::DrawSelectedPointPanel(std::vector<AdaptiveTrigger::CurvePoint>& points, int& selectedIndex)
{
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(points.size()))
    {
        ImGui::TextDisabled("ポイントを選択してください");
        return;
    }

    const int lastIndex = static_cast<int>(points.size()) - 1;
    const bool isBoundary = (selectedIndex == 0 || selectedIndex == lastIndex);

    ImGui::PushID(selectedIndex);
    ImGui::TextDisabled("選択中のポイント");
    ImGui::Spacing();

    AdaptiveTrigger::CurvePoint& p = points[selectedIndex];

    ImGui::Text("X (ゾーン内位置)");
    ImGui::SetNextItemWidth(-1);
    if (isBoundary)
    {
        ImGui::BeginDisabled();
        float fixedX = p.position;
        ImGui::DragFloat("##posX", &fixedX, 0.0f, 0.0f, 1.0f, "%.3f");
        ImGui::EndDisabled();
    }
    else
    {
        ImGui::DragFloat("##posX", &p.position, 0.005f, 0.0f, 1.0f, "%.3f");
    }

    ImGui::Spacing();

    ImGui::Text("Y (強さ)");
    ImGui::SetNextItemWidth(-1);
    ImGui::DragFloat("##posY", &p.value, 0.005f, 0.0f, 1.0f, "%.3f");

    ImGui::Spacing();

    if (isBoundary)
    {
        ImGui::BeginDisabled();
        ImGui::Button("削除（境界のため不可）", ImVec2(-1, 0));
        ImGui::EndDisabled();
    }
    else if (ImGui::Button("削除", ImVec2(-1, 0)))
    {
        points.erase(points.begin() + selectedIndex);
        selectedIndex = -1;
        ImGui::PopID();
        return;
    }

    ImGui::PopID();
}