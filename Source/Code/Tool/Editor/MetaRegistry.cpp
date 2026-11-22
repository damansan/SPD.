#include "MetaRegistry.h"
#include <DirectXMath.h>
#include <filesystem>
#include <imgui.h>
#include <vector>
#include <utility>

using namespace entt::literals;

// 登録済みメタ情報からInspectorUIを自動描画
void MetaRegistry::DrawMetaFields(entt::meta_type metaType, entt::meta_any instance)
{
	if (!metaType) return;

	for (auto&& [id, metaData] : metaType.data())
	{
		ImGui::PushID(static_cast<int>(id));

		FieldInfo* fieldInfo = metaData.custom();
		std::string label = fieldInfo ? fieldInfo->label : std::string(metaData.name());

		entt::meta_any value = metaData.get(instance);

		if (float* f = value.try_cast<float>())
		{
			if (fieldInfo && HasFlag(fieldInfo->flags, FieldFlags::VisibleOnly)) 
			{
				ImGui::Text("%s: %.2f", label.c_str(), *f);
			}
			else
			{
				float minV = fieldInfo ? fieldInfo->minValue : 0.0f;
				float maxV = fieldInfo ? fieldInfo->maxValue : 0.0f;
				float speed = fieldInfo ? fieldInfo->dragSpeed : 0.1f;

				if (ImGui::DragFloat(label.c_str(), f, speed, minV, maxV))
				{
					metaData.set(instance, *f);
				}
			}
		}
		else if (DirectX::XMFLOAT3* v = value.try_cast<DirectX::XMFLOAT3>())
		{
			float speed = fieldInfo ? fieldInfo->dragSpeed : 0.1f;

			if (ImGui::DragFloat3(label.c_str(), &v->x, speed))
			{
				metaData.set(instance, *v);
			}
		}
		else if (bool* b = value.try_cast<bool>())
		{
			if (fieldInfo && HasFlag(fieldInfo->flags, FieldFlags::VisibleOnly)) 
			{
				ImGui::Text("%s: %s", label.c_str(), *b ? "true" : "false");
			}
			else
			{
				if (ImGui::Checkbox(label.c_str(), b))
				{
					metaData.set(instance, *b);
				}
			}
		}
		else if (std::string* s = value.try_cast<std::string>())
		{
			if (fieldInfo && !fieldInfo->dragDropPayload.empty())
			{
				std::string displayName = s->empty()
					? "None"
					: std::filesystem::path(*s).stem().string();

				ImGui::InputText(label.c_str(), displayName.data(),
					displayName.size() + 1, ImGuiInputTextFlags_ReadOnly);

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(fieldInfo->dragDropPayload.c_str()))
					{
						std::string newPath(static_cast<const char*>(payload->Data));
						metaData.set(instance, newPath);
					}
					ImGui::EndDragDropTarget();
				}
			}
			else if (fieldInfo && HasFlag(fieldInfo->flags, FieldFlags::VisibleOnly))
			{
				ImGui::Text("%s: %s", label.c_str(), s->c_str());
			}
			else
			{
				char buf[256];
				strncpy_s(buf, s->c_str(), sizeof(buf) - 1);
				if (ImGui::InputText(label.c_str(), buf, sizeof(buf)))
				{
					metaData.set(instance, std::string(buf));
				}
			}
		}
		else if (value.type().is_enum())
		{
			entt::meta_type enumType = value.type();

			// enumの各値(REFLECT_ENUM_VALUEで登録した定数)を、ラベル付きで列挙する
			std::vector<std::pair<std::string, entt::meta_any>> options;
			std::string currentLabel = "(Unknown)";

			for (auto&& [enumId, enumData] : enumType.data())
			{
				EnumValueInfo* enumInfo = enumData.custom();
				std::string optLabel = enumInfo ? enumInfo->label : std::string(enumData.name());

				entt::meta_any optValue = enumData.get({});
				options.push_back({ optLabel, optValue });

				if (optValue == value)
				{
					currentLabel = optLabel;
				}
			}

			if (ImGui::BeginCombo(label.c_str(), currentLabel.c_str()))
			{
				for (auto& [optLabel, optValue] : options)
				{
					bool selected = (optLabel == currentLabel);
					if (ImGui::Selectable(optLabel.c_str(), selected))
					{
						metaData.set(instance, optValue);
					}
					if (selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		else if (value.type().data().begin() != value.type().data().end())
		{
			// REFLECTされた構造体：中身を同じ関数で描画する
			if (ImGui::TreeNode(label.c_str()))
			{
				DrawMetaFields(value.type(), value.as_ref());
				ImGui::TreePop();

				// valueはコピーなので、編集結果を元のインスタンスへ書き戻す
				metaData.set(instance, value);
			}
		}
		else
		{
			ImGui::TextDisabled("%s: (unsupported type)", label.c_str());
		}

		ImGui::PopID();
	}
}