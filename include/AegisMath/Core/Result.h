#pragma once
#include "MathError.h"
#include <cassert>
#include <variant>
#include <utility>
#include <type_traits>

namespace AegisMath::Core {

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

        // 数据提取 (必须先验证 has_value())
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

        // 兼容性接口
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

        // 错误提取 (必须在 !has_value() 状态下调用)
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

} // namespace AegisMath::Core