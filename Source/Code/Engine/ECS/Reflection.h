#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "entt.hpp"

// 値の型
enum class ValueType
{
	Int,
	Float,
	Bool,
	Float2,
	Float3,
	Float4,

	Unsupported, // 条件で読めない型(strong,enumなど)
};

// フィールドの扱いを表すフラグ
enum class FieldFlags : uint32_t
{
	None = 0,
	VisibleOnly = 1 << 0, // エディタで表示のみ
	Transient = 1 << 1,   // 保存しない
};
inline FieldFlags operator|(FieldFlags a, FieldFlags b)
{
	return static_cast<FieldFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline bool HasFlag(FieldFlags value, FieldFlags flag)
{
	return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}

// フィールド1つに添付する付加情報
struct FieldInfo
{
	std::string label;      // エディタ上の表示名
	float minValue = 0.0f;  // 下限値
	float maxValue = 0.0f;  // 上限値
	float dragSpeed = 0.1f; // ドラッグ時の変化量
	std::string dragDropPayload;
	FieldFlags flags = FieldFlags::None;
};

// コンポーネント1つに添付する付加情報
struct TypeInfo
{
	std::string category;
};

// enumの、1つの値に紐づく表示名
struct EnumValueInfo
{
	std::string label;
};


// フィールド1件分の情報(条件の評価やGetter選択UIで使う)
struct FieldEntry
{
	entt::meta_type componentType; // 属するコンポーネントの型
	entt::id_type fieldId = 0;     // entt::meta上のフィールドID
	entt::meta_type nestedType;    // ネストしている場合の型
	entt::id_type subFieldId = 0;  // ネストしている場合のフィールドID
	std::string componentName;     // コンポーネントの型名
	std::string category;          // カテゴリ
	std::string fieldLabel;        // フィールドの表示名
	std::string fieldName;         // フィールドのコード上の名前

	ValueType type = ValueType::Float;
};


class Reflection
{
public:
	// 登録済みのフィールド一覧を取得する
	static const std::vector<FieldEntry>& GetAllFields();

	// コード上の名前でフィールドを探す
	static const FieldEntry* FindField(const std::string& componentName, const std::string& fieldName);
	// 表示名でフィールドを探す
	static const FieldEntry* FindFieldByLabel(const std::string& componentName, const std::string& label);

	// Float2/3/4型の、成分数を返す(それ以外は0)
	static int GetComponentCount(ValueType type);
};