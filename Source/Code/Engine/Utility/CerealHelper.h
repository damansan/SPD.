#pragma once
#include <DirectXMath.h>

// Cereal前方宣言
namespace cereal
{
    class BinaryOutputArchive;
    class BinaryInputArchive;
}

// DirectXMath構造体のserialize宣言
namespace DirectX
{
    template<class T> void serialize(T& archive, DirectX::XMFLOAT2& v);
    template<class T> void serialize(T& archive, DirectX::XMFLOAT3& v);
    template<class T> void serialize(T& archive, DirectX::XMFLOAT4& v);
    template<class T> void serialize(T& archive, DirectX::XMFLOAT4X4& m);
    template<class T> void serialize(T& archive, DirectX::XMUINT4& v);

    // 明示的インスタンス化の宣言（.cppで展開済みと伝える）
    extern template void serialize(cereal::BinaryOutputArchive&, DirectX::XMFLOAT2&);
    extern template void serialize(cereal::BinaryInputArchive&, DirectX::XMFLOAT2&);
    extern template void serialize(cereal::BinaryOutputArchive&, DirectX::XMFLOAT3&);
    extern template void serialize(cereal::BinaryInputArchive&, DirectX::XMFLOAT3&);
    extern template void serialize(cereal::BinaryOutputArchive&, DirectX::XMFLOAT4&);
    extern template void serialize(cereal::BinaryInputArchive&, DirectX::XMFLOAT4&);
    extern template void serialize(cereal::BinaryOutputArchive&, DirectX::XMFLOAT4X4&);
    extern template void serialize(cereal::BinaryInputArchive&, DirectX::XMFLOAT4X4&);
    extern template void serialize(cereal::BinaryOutputArchive&, DirectX::XMUINT4&);
    extern template void serialize(cereal::BinaryInputArchive&, DirectX::XMUINT4&); 
}