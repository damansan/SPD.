#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

namespace AdaptiveTrigger
{
    struct CurvePoint
    {
        // 押し込み量
        float position = 0.0f; // (0,0～1.0)
        // 抵抗の強さ
        float value = 0.0f;    // (0.0～1.0)
    };

    struct TriggerEffect
    {
        // 出力リスト
        std::vector<CurvePoint> forcePoints;
    };

    // 現在地(押し込み量)から前後に存在するポイントを線形補完して現在の抵抗値を求める
    inline float SampleCurve(const std::vector<CurvePoint>& points, float position)
    {
        // 無抵抗
        if (points.empty())
        {
            return 0.0f;
        }
        // 点が1つだけ、または position が最初の点より手前の時は最初の点の値をそのまま使う
        if (points.size() == 1 || position <= points.front().position)
        {
            return points.front().value;
        }
        // position が最後の点より後ろなら最後の点の値をそのまま使う
        if (position >= points.back().position)
        {
            return points.back().value;
        }

        // 該当する2点を探してその間を線形補間する
        for (size_t i = 0; i + 1 < points.size(); ++i)
        {
            const CurvePoint& a = points[i];
            const CurvePoint& b = points[i + 1];
            if (position >= a.position && position <= b.position)
            {
                float t = (position - a.position) / (b.position - a.position);
                return a.value + (b.value - a.value) * t;
            }
        }
        return points.back().value;
    }
}

namespace AdaptiveTrigger
{
    struct Zone
    {
        // このゾーンに属するポイントのリスト
        std::vector<CurvePoint> points;
    };

    // レンジ内のゾーンを検出する
    inline std::vector<Zone> DetectZones(const std::vector<CurvePoint>& points)
    {
        // 検出したゾーンのリスト
        std::vector<Zone> zones;
		// 現在のゾーン
        Zone current;

        // レンジ内のポイントを先頭から順に走査する
        for (const auto& p : points)
        {
			// 抵抗値が0より大きければ現在のゾーンに追加する
            if (p.value > 0.0f)
            {
                current.points.push_back(p);
            }
            else
            {
				// 抵抗値が0なら抵抗がない場所へ来た
                if (!current.points.empty())
                {
                    // 一区切りとする
                    zones.push_back(current);
					// 現在のゾーンをリセット
                    current.points.clear();
                }
            }
        }

		// 押し込み切った時に抵抗値0以上があればその地点を最後のゾーンとする
        if (!current.points.empty())
        {
            zones.push_back(current);
        }

        return zones;
    }
}

namespace AdaptiveTrigger
{
    // 同じ値とみなす誤差の範囲
    constexpr float ValueEpsilon = 0.05f;

    // ゾーンの判定結果
    enum class ResistanceType
    {
        Constant, // 一定抵抗
        Changing, // 変化抵抗
        Complex,  // 複雑なカーブ
    };

    // ゾーンの種類を判定する
    inline ResistanceType ClassifyZone(const Zone& zone)
    {
		// ポイントが３点以上なら複雑なカーブとみなす
        if (zone.points.size() >= 3)
        {
            return ResistanceType::Complex;
        }

		// 2点以下なら、最初と最後の値の差を見て一定抵抗か判断する
        float diff = std::abs(zone.points.front().value - zone.points.back().value);
        if (diff < ValueEpsilon)
        {
            return ResistanceType::Constant;
        }
        return ResistanceType::Changing;
    }
}

namespace AdaptiveTrigger
{
    inline float ComputeForce(const TriggerEffect& effect, float currentPosition)
    {
        // 現在地の抵抗値を取得
        float force = SampleCurve(effect.forcePoints, currentPosition);
        // 範囲外保険
        return std::clamp(force, 0.0f, 1.0f); 
    }
}