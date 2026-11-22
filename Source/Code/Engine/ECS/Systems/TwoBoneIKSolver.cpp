#include "TwoBoneIKSolver.h"
#include <algorithm>
#include <cmath>

namespace TwoBoneIKSolver
{
	namespace
	{
		// 安全なacos(浮動小数の誤差でxが[-1,1]をわずかに超えることがあるため)
		float SafeAcos(float x)
		{
			return std::acos(std::clamp(x, -1.0f, 1.0f));
		}

		// 3辺の長さ(a, b, c)から成る三角形の、辺a・辺bに挟まれた角(=辺cの対角)を求める
		// 余弦定理: c^2 = a^2 + b^2 - 2ab*cos(C)  →  C = acos((a^2+b^2-c^2) / (2ab))
		float TriangleAngle(float a, float b, float oppositeC)
		{
			float denom = 2.0f * a * b;
			if (denom < 1e-6f)
			{
				// ボーン長がほぼ0のような異常値の保険
				return 0.0f;
			}
			float cosAngle = (a * a + b * b - oppositeC * oppositeC) / denom;
			return SafeAcos(cosAngle);
		}

		// root-target間の距離を、実際に届く範囲にクランプする
		float ClampTargetLength(float rawLength, float upperLen, float lowerLen)
		{
			// 伸びきった状態(ロック)を防ぐため、わずかに内側に余裕を持たせる
			constexpr float epsilon = 0.001f;
			float maxLength = upperLen + lowerLen - epsilon;
			float minLength = std::abs(upperLen - lowerLen) + epsilon;
			return std::clamp(rawLength, minLength, maxLength);
		}

		// root→target方向を軸に、poleVectorが乗っている「曲げ平面」の法線を求める
		// この法線を軸にmid関節を回転させることで、膝が狙った方向に曲がる
		DirectX::XMVECTOR CalcBendAxis(
			DirectX::FXMVECTOR rootToTarget, // 正規化済みでなくてよい
			DirectX::FXMVECTOR rootToPole)   // 正規化済みでなくてよい
		{
			DirectX::XMVECTOR axis = DirectX::XMVector3Cross(rootToTarget, rootToPole);
			float lengthSq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(axis));
			if (lengthSq < 1e-8f)
			{
				// rootToTargetとrootToPoleがほぼ平行(poleVectorが役に立たない位置にある)
				// 保険として適当な垂直軸を作る
				DirectX::XMVECTOR fallback = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
				axis = DirectX::XMVector3Cross(rootToTarget, fallback);
				if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(axis)) < 1e-8f)
				{
					fallback = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
					axis = DirectX::XMVector3Cross(rootToTarget, fallback);
				}
			}
			return DirectX::XMVector3Normalize(axis);
		}

		// 現在のdirectionを、axis周りにangle分だけ回転させたベクトルを返す
		DirectX::XMVECTOR RotateAroundAxis(DirectX::FXMVECTOR direction, DirectX::FXMVECTOR axis, float angle)
		{
			DirectX::XMVECTOR rotationQuat = DirectX::XMQuaternionRotationAxis(axis, angle);
			return DirectX::XMVector3Rotate(direction, rotationQuat);
		}
	}

	// fromベクトルをtoベクトルに向けて、最短回転のクォータニオンを求める
	DirectX::XMVECTOR RotationFromToRotation(DirectX::FXMVECTOR from, DirectX::FXMVECTOR to)
	{
		DirectX::XMVECTOR fromN = DirectX::XMVector3Normalize(from);
		DirectX::XMVECTOR toN = DirectX::XMVector3Normalize(to);

		float dot = std::clamp(DirectX::XMVectorGetX(DirectX::XMVector3Dot(fromN, toN)), -1.0f, 1.0f);

		if (dot > 0.9999f)
		{
			// ほぼ同じ向き: 回転不要
			return DirectX::XMQuaternionIdentity();
		}
		if (dot < -0.9999f)
		{
			// ほぼ正反対: 任意の垂直軸で180度回転
			DirectX::XMVECTOR fallback = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
			DirectX::XMVECTOR axis = DirectX::XMVector3Cross(fromN, fallback);
			if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(axis)) < 1e-8f)
			{
				fallback = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
				axis = DirectX::XMVector3Cross(fromN, fallback);
			}
			return DirectX::XMQuaternionRotationAxis(DirectX::XMVector3Normalize(axis), DirectX::XM_PI);
		}

		DirectX::XMVECTOR axis = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(fromN, toN));
		float angle = SafeAcos(dot);
		return DirectX::XMQuaternionRotationAxis(axis, angle);
	}

	Result Solve(
		const DirectX::XMFLOAT3& root,
		const DirectX::XMFLOAT3& mid,
		const DirectX::XMFLOAT3& end,
		const DirectX::XMFLOAT4& rootRotation,
		const DirectX::XMFLOAT4& midRotation,
		const DirectX::XMFLOAT3& target,
		const DirectX::XMFLOAT3& poleVector)
	{
		DirectX::XMVECTOR rootV = DirectX::XMLoadFloat3(&root);
		DirectX::XMVECTOR midV = DirectX::XMLoadFloat3(&mid);
		DirectX::XMVECTOR endV = DirectX::XMLoadFloat3(&end);
		DirectX::XMVECTOR targetV = DirectX::XMLoadFloat3(&target);
		DirectX::XMVECTOR poleV = DirectX::XMLoadFloat3(&poleVector);

		// 各辺のベクトルと長さ
		DirectX::XMVECTOR ab = DirectX::XMVectorSubtract(midV, rootV);    // root→mid (上の骨)
		DirectX::XMVECTOR bc = DirectX::XMVectorSubtract(endV, midV);     // mid→end (下の骨)
		DirectX::XMVECTOR ac = DirectX::XMVectorSubtract(endV, rootV);    // root→end (現在の姿勢での対角)
		DirectX::XMVECTOR at = DirectX::XMVectorSubtract(targetV, rootV); // root→target

		float upperLen = DirectX::XMVectorGetX(DirectX::XMVector3Length(ab));
		float lowerLen = DirectX::XMVectorGetX(DirectX::XMVector3Length(bc));
		float acLen = DirectX::XMVectorGetX(DirectX::XMVector3Length(ac));
		float rawTargetLen = DirectX::XMVectorGetX(DirectX::XMVector3Length(at));

		float targetLen = ClampTargetLength(rawTargetLen, upperLen, lowerLen);

		// 曲げ平面の軸(root→target と root→poleVector が乗る平面の法線)
		DirectX::XMVECTOR bendAxis = CalcBendAxis(at, DirectX::XMVectorSubtract(poleV, rootV));

		// mid関節の角度: 現在の角度 → targetLenに対応する角度、への差分を求める
		float oldMidAngle = TriangleAngle(upperLen, lowerLen, acLen);
		float newMidAngle = TriangleAngle(upperLen, lowerLen, targetLen);
		float deltaMidAngle = newMidAngle - oldMidAngle;

		// mid関節をbendAxis周りにdeltaMidAngleだけ回転させる
		DirectX::XMVECTOR midDeltaRotation = DirectX::XMQuaternionRotationAxis(bendAxis, deltaMidAngle);
		DirectX::XMVECTOR midRotationV = DirectX::XMLoadFloat4(&midRotation);
		DirectX::XMVECTOR bentMidRotationV = DirectX::XMQuaternionMultiply(midRotationV, midDeltaRotation);

		// mid関節を回したことで、bc(mid→end)の向きも一緒に変わる
		DirectX::XMVECTOR newBc = RotateAroundAxis(bc, bendAxis, deltaMidAngle);
		DirectX::XMVECTOR newAc = DirectX::XMVectorAdd(ab, newBc); // root→新しいend の方向・距離(長さ = targetLen になっているはず)

		// newAc(曲げた後のroot→end)を、at(root→target)の方向に向ける回転を求める
		DirectX::XMVECTOR aimRotation = RotationFromToRotation(newAc, at);

		// root回転にaimRotationを上乗せする
		DirectX::XMVECTOR rootRotationV = DirectX::XMLoadFloat4(&rootRotation);
		DirectX::XMVECTOR newRootRotationV = DirectX::XMQuaternionMultiply(rootRotationV, aimRotation);

		// rootが回ればmid, endも一緒に回るので、midの回転にも同じ分を上乗せする
		DirectX::XMVECTOR finalMidRotationV = DirectX::XMQuaternionMultiply(bentMidRotationV, aimRotation);

		// 最終的なmid, endのワールド位置を求める
		DirectX::XMVECTOR finalAb = DirectX::XMVector3Rotate(ab, aimRotation);
		DirectX::XMVECTOR finalMidPositionV = DirectX::XMVectorAdd(rootV, finalAb);

		DirectX::XMVECTOR finalBc = DirectX::XMVector3Rotate(newBc, aimRotation);
		DirectX::XMVECTOR finalEndPositionV = DirectX::XMVectorAdd(finalMidPositionV, finalBc);

		Result result{};
		DirectX::XMStoreFloat3(&result.midPosition, finalMidPositionV);
		DirectX::XMStoreFloat4(&result.rootRotation, newRootRotationV);
		DirectX::XMStoreFloat4(&result.midRotation, finalMidRotationV);
		DirectX::XMStoreFloat3(&result.endPosition, finalEndPositionV);
		return result;
	}
}