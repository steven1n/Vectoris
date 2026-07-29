#pragma once
#include "../Core/Concepts.h"
#include <compare>

namespace AegisMath {

    // Quantity 核心类，封锁了所有危险的隐式行为
    template <Concepts::FloatingPoint T, typename UnitTag>
    class Quantity {
    public:
        using ValueType = T;
        using TagType = UnitTag;

        // 【禁止】隐式转换，必须显式构造
        explicit constexpr Quantity(T val = static_cast<T>(0)) noexcept : m_value(val) {}

        // 【安全】获取原始数值
        [[nodiscard]] constexpr T value() const noexcept { return m_value; }

        // 同单位运算 (零开销)
        constexpr Quantity operator+(const Quantity& rhs) const noexcept {
            return Quantity(m_value + rhs.m_value);
        }
        constexpr Quantity operator-(const Quantity& rhs) const noexcept {
            return Quantity(m_value - rhs.m_value);
        }
        constexpr Quantity& operator+=(const Quantity& rhs) noexcept {
            m_value += rhs.m_value; return *this;
        }
        constexpr Quantity& operator-=(const Quantity& rhs) noexcept {
            m_value -= rhs.m_value; return *this;
        }
        
        // 标量乘除 (如 Meter * 2.0)
        constexpr Quantity operator*(T scalar) const noexcept { return Quantity(m_value * scalar); }
        constexpr Quantity operator/(T scalar) const noexcept { return Quantity(m_value / scalar); }

        // 取负
        constexpr Quantity operator-() const noexcept { return Quantity(-m_value); }

        // C++20 飞船运算符，支持所有比较 (<, >, ==, != 等)
        constexpr auto operator<=>(const Quantity&) const = default;

    private:
        T m_value;
    };

    // 标量左乘支持 (如 2.0 * Meter)
    template <Concepts::FloatingPoint T, typename UnitTag>
    constexpr Quantity<T, UnitTag> operator*(T scalar, const Quantity<T, UnitTag>& q) noexcept {
        return Quantity<T, UnitTag>(scalar * q.value());
    }

    // =========================================================================
    // 安全单位转换器 (Unit Cast)
    // 强制要求：仅允许在同一个维度 (Dimension) 内进行转换。
    // =========================================================================
    template <typename ToTag, typename T, typename FromTag>
    requires std::same_as<typename ToTag::Dim, typename FromTag::Dim>
    [[nodiscard]] constexpr Quantity<T, ToTag> unit_cast(const Quantity<T, FromTag>& q) noexcept {
        if constexpr (std::is_same_v<ToTag, FromTag>) {
            return q;
        } else {
            // 编译期折叠系数计算，零运行时开销
            constexpr T factor = static_cast<T>(FromTag::scale / ToTag::scale);
            return Quantity<T, ToTag>(q.value() * factor);
        }
    }

} // namespace AegisMath