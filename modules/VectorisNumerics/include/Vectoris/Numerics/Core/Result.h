#pragma once
#include "MathError.h"
#include <cassert>
#include <variant>
#include <utility>
#include <type_traits>

namespace vectoris::numerics::Core {

    // 专为无异常/无堆分配环境设计的强类型 Result 范式
    // 遵从 ISO C++20 与 Engineering Standard v1.0 Section 45
    template <typename T, typename E = MathError>
    class Result final {
        static_assert(!std::is_same_v<T, E>, "Result value type T and error type E cannot be the same type.");

    private:
        std::variant<T, E> storage_;

    public:
        using value_type = T;
        using error_type = E;

        // 禁止无参数默认构造，强制每个 Result 必须明确携带有效值或具体错误原因
        constexpr Result() = delete;

        // 值构造
        template <typename U = T>
        requires (!std::is_same_v<std::remove_cvref_t<U>, Result> &&
                  !std::is_same_v<std::remove_cvref_t<U>, E> &&
                  std::is_constructible_v<T, U>)
        constexpr Result(U&& val)
            noexcept(std::is_nothrow_constructible_v<T, U>)
            : storage_(std::in_place_index<0>, std::forward<U>(val)) {}

        // 原位就地构造
        template <typename... Args>
        requires std::is_constructible_v<T, Args...>
        constexpr explicit Result(std::in_place_t, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args...>)
            : storage_(std::in_place_index<0>, std::forward<Args>(args)...) {}

        // 错误构造
        template <typename Err = E>
        requires (!std::is_same_v<std::remove_cvref_t<Err>, Result> &&
                  !std::is_same_v<std::remove_cvref_t<Err>, T> &&
                  std::is_constructible_v<E, Err>)
        constexpr explicit Result(Err&& err)
            noexcept(std::is_nothrow_constructible_v<E, Err>)
            : storage_(std::in_place_index<1>, std::forward<Err>(err)) {}

        // 显式工厂
        template <typename U = T>
        static constexpr Result success(U&& val)
            noexcept(std::is_nothrow_constructible_v<T, U>) {
            return Result(std::forward<U>(val));
        }

        template <typename... Args>
        requires std::is_constructible_v<T, Args...>
        static constexpr Result success(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args...>) {
            return Result(std::in_place, std::forward<Args>(args)...);
        }

        template <typename Err = E>
        static constexpr Result failure(Err&& err)
            noexcept(std::is_nothrow_constructible_v<E, Err>) {
            return Result(std::forward<Err>(err));
        }

        // 状态查询
        [[nodiscard]] constexpr bool has_value() const noexcept {
            return storage_.index() == 0;
        }

        [[nodiscard]] constexpr bool IsSuccess() const noexcept {
            return has_value();
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept {
            return has_value();
        }

        // --- 契约式数据提取 (Contract-based Accessors) ---
        // @pre has_value() == true.
        // 诊断构建 (Debug): 断言校验前置条件，违规时终止并报错。
        // 发布构建 (Release / NDEBUG): 断言失效，违规调用视为违反编程契约 (Contract Violation)，引发未定义行为。
        // 若需完全防御性访问，请使用 value_if() 或 value_or()。
        [[nodiscard]] constexpr const T& value() const & noexcept {
            assert(has_value() && "AegisMath Precondition Violation: Result::value() called on failed Result");
            return *std::get_if<0>(&storage_);
        }

        [[nodiscard]] constexpr T& value() & noexcept {
            assert(has_value() && "AegisMath Precondition Violation: Result::value() called on failed Result");
            return *std::get_if<0>(&storage_);
        }

        [[nodiscard]] constexpr const T&& value() const && noexcept {
            assert(has_value() && "AegisMath Precondition Violation: Result::value() called on failed Result");
            return std::move(*std::get_if<0>(&storage_));
        }

        [[nodiscard]] constexpr T&& value() && noexcept {
            assert(has_value() && "AegisMath Precondition Violation: Result::value() called on failed Result");
            return std::move(*std::get_if<0>(&storage_));
        }

        // 兼容性接口 (同 value() 契约)
        [[nodiscard]] constexpr const T& Value() const & noexcept {
            return value();
        }

        [[nodiscard]] constexpr T& Value() & noexcept {
            return value();
        }

        [[nodiscard]] constexpr const T&& Value() const && noexcept {
            return std::move(value());
        }

        [[nodiscard]] constexpr T&& Value() && noexcept {
            return std::move(value());
        }

        // --- 防御性指针提取接口 (Checked Accessors, 零异常、零未定义行为) ---

        /// @brief 若当前为成功状态则返回指向 T 的常量指针，失败则返回 nullptr
        [[nodiscard]] constexpr const T* value_if() const noexcept {
            return std::get_if<0>(&storage_);
        }

        /// @brief 若当前为成功状态则返回指向 T 的指针，失败则返回 nullptr
        [[nodiscard]] constexpr T* value_if() noexcept {
            return std::get_if<0>(&storage_);
        }

        /// @brief 若当前为失败状态则返回指向 E 的常量指针，成功则返回 nullptr
        [[nodiscard]] constexpr const E* error_if() const noexcept {
            return std::get_if<1>(&storage_);
        }

        /// @brief 若当前为失败状态则返回指向 E 的指针，成功则返回 nullptr
        [[nodiscard]] constexpr E* error_if() noexcept {
            return std::get_if<1>(&storage_);
        }

        template <typename U>
        [[nodiscard]] constexpr T value_or(U&& default_value) const & {
            if (has_value()) {
                return value();
            }
            return static_cast<T>(std::forward<U>(default_value));
        }

        template <typename U>
        [[nodiscard]] constexpr T value_or(U&& default_value) && {
            if (has_value()) {
                return std::move(value());
            }
            return static_cast<T>(std::forward<U>(default_value));
        }

        // --- 契约式错误提取 (Contract-based Error Accessors) ---
        // @pre !has_value() == true.
        // 诊断构建 (Debug): 断言校验前置条件，违规时终止并报错。
        // 发布构建 (Release / NDEBUG): 断言失效，违规调用视为违反编程契约 (Contract Violation)，引发未定义行为。
        // 若需完全防御性访问，请使用 error_if()。
        [[nodiscard]] constexpr const E& error() const & noexcept {
            assert(!has_value() && "AegisMath Precondition Violation: Result::error() called on successful Result");
            return *std::get_if<1>(&storage_);
        }

        [[nodiscard]] constexpr E& error() & noexcept {
            assert(!has_value() && "AegisMath Precondition Violation: Result::error() called on successful Result");
            return *std::get_if<1>(&storage_);
        }

        [[nodiscard]] constexpr const E&& error() const && noexcept {
            assert(!has_value() && "AegisMath Precondition Violation: Result::error() called on successful Result");
            return std::move(*std::get_if<1>(&storage_));
        }

        [[nodiscard]] constexpr E&& error() && noexcept {
            assert(!has_value() && "AegisMath Precondition Violation: Result::error() called on successful Result");
            return std::move(*std::get_if<1>(&storage_));
        }
    };

} // namespace vectoris::numerics::Core