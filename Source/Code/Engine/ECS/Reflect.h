#pragma once
#include "entt.hpp"
#include "Engine/ECS/Reflection.h"

using namespace entt::literals;

// 型そのものの登録を開始する
#define REFLECT_BEGIN(Type, DisplayName)                                            \
private:                                                                            \
    using ReflectSelf = Type;                                                       \
    static inline bool _reflect_begin = []                                          \
    {                                                                               \
        entt::meta_factory<Type>{}                                                  \
            .type(#Type)                                                            \
            .custom<TypeInfo>(TypeInfo{ DisplayName });                             \
        return true;                                                                \
    }();                                                                            \
public:

// FieldInfoを、位置引数＋デフォルト引数で組み立てるヘルパー
inline FieldInfo MakeFieldInfo(
    std::string label,
    float minValue = 0.0f,
    float maxValue = 0.0f,
    float dragSpeed = 0.1f,
    std::string dragDropPayload = "",
    FieldFlags flags = FieldFlags::None)
{
    return FieldInfo{ label, minValue, maxValue, dragSpeed, dragDropPayload, flags };
}

// 1フィールド分の宣言＋登録
#define REFLECT_FIELD(FieldType, FieldName, DefaultArgs, FieldInfoArgs) \
    FieldType FieldName = FieldType DefaultArgs;                        \
    static inline bool _reflect_##FieldName##_registered = []           \
    {                                                                   \
        entt::meta_factory<ReflectSelf>{}                               \
            .data<&ReflectSelf::FieldName>(#FieldName)                  \
            .custom<FieldInfo>(MakeFieldInfo FieldInfoArgs);            \
        return true;                                                    \
    }();

// enumの登録
#define REFLECT_ENUM(EnumType)                          \
namespace ReflectEnumDetail_##EnumType                  \
{                                                       \
    using ReflectEnumSelf = EnumType;                   \
    inline bool _reflect_enum_type_registered = []      \
    {                                                   \
        entt::meta_factory<EnumType>{}.type(#EnumType); \
        return true;                                    \
    }();

#define REFLECT_ENUM_VALUE(ValueName, Value, Label)         \
    inline bool _reflect_##ValueName##_registered = []      \
    {                                                       \
        entt::meta_factory<ReflectEnumSelf>{}               \
            .data<Value>(#ValueName)                        \
            .custom<EnumValueInfo>(EnumValueInfo{ Label }); \
        return true;                                        \
    }();

#define REFLECT_ENUM_END() }

#define REFLECT_END()