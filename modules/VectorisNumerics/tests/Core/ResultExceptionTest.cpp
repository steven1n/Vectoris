#include "Vectoris/Numerics/Core/Result.h"
#include <gtest/gtest.h>
#include <memory>
#include <type_traits>
#include <utility>

namespace {
using vectoris::numerics::core::Result;
enum class Fault { none, copy_construct, move_construct, copy_assign, move_assign, convert };
struct PayloadException {};
Fault active_fault = Fault::none;

void throw_if(Fault operation) {
    if (active_fault == operation) {
        throw PayloadException{};
    }
}

struct FaultScope final {
    explicit FaultScope(Fault operation) { active_fault = operation; }
    ~FaultScope() { active_fault = Fault::none; }
    FaultScope(const FaultScope&) = delete;
    FaultScope& operator=(const FaultScope&) = delete;
    FaultScope(FaultScope&&) = delete;
    FaultScope& operator=(FaultScope&&) = delete;
};
struct Source { int id; };

template <int Tag, bool NothrowMove = true>
struct Payload final {
    int id;
    Payload() = delete;
    explicit Payload(int value) noexcept : id(value) {}
    explicit Payload(Source source) : id(source.id) { throw_if(Fault::convert); }
    Payload(const Payload& other) : id(other.id) { throw_if(Fault::copy_construct); }
    Payload(Payload&& other) noexcept(NothrowMove) : id(std::exchange(other.id, -9)) {
        if constexpr (!NothrowMove) { throw_if(Fault::move_construct); }
    }
    Payload& operator=(const Payload& other) {
        id = other.id; // Deliberately modify before throwing: only the basic guarantee.
        throw_if(Fault::copy_assign);
        return *this;
    }
    Payload& operator=(Payload&& other) noexcept(false) {
        id = std::exchange(other.id, -9);
        throw_if(Fault::move_assign);
        return *this;
    }
    ~Payload() noexcept = default;
};
using Safe = Result<Payload<0>, Payload<1>>;
using Unsafe = Result<Payload<0, false>, Payload<1, false>>;

// Copy construction alone suffices to protect cross-alternative copy assignment.
struct NothrowCopy final {
    int id;
    explicit NothrowCopy(int value) noexcept : id(value) {}
    NothrowCopy(const NothrowCopy&) noexcept = default;
    NothrowCopy(NothrowCopy&& other) noexcept(false) : id(other.id) { throw_if(Fault::move_construct); }
    NothrowCopy& operator=(const NothrowCopy&) noexcept = default;
    NothrowCopy& operator=(NothrowCopy&&) noexcept = default;
    ~NothrowCopy() = default;
};
struct Immovable final {
    int id;
    explicit Immovable(int value) noexcept : id(value) {}
    Immovable(const Immovable&) = delete;
    Immovable(Immovable&&) = delete;
    Immovable& operator=(const Immovable&) = delete;
    Immovable& operator=(Immovable&&) = delete;
    ~Immovable() = default;
};

template <typename R>
R make_result(bool value, int id) {
    if (value) { return R::success(id); }
    return R::failure(id);
}

template <typename R>
void expect_state(R& result, bool value, int id) {
    const auto& view = result;
    ASSERT_EQ(result.has_value(), value);
    EXPECT_EQ(result.IsSuccess(), value);
    EXPECT_EQ(static_cast<bool>(result), value);
    ASSERT_EQ(result.value_if() != nullptr, value);
    ASSERT_EQ(result.error_if() != nullptr, !value);
    EXPECT_EQ(view.value_if(), result.value_if());
    EXPECT_EQ(view.error_if(), result.error_if());
    if (value) {
        EXPECT_EQ(result.value().id, id);
        EXPECT_EQ(result.Value().id, id);
        EXPECT_EQ(view.value().id, id);
        EXPECT_EQ(view.Value().id, id);
        EXPECT_EQ(static_cast<R&&>(result).value().id, id);
        EXPECT_EQ(static_cast<R&&>(result).Value().id, id);
        EXPECT_EQ(static_cast<const R&&>(view).value().id, id);
        EXPECT_EQ(static_cast<const R&&>(view).Value().id, id);
    } else {
        EXPECT_EQ(result.error().id, id);
        EXPECT_EQ(view.error().id, id);
        EXPECT_EQ(static_cast<R&&>(result).error().id, id);
        EXPECT_EQ(static_cast<const R&&>(view).error().id, id);
    }
}

void check_assignment(bool before, bool after, bool move) {
    auto destination = make_result<Safe>(before, 17);
    auto source = make_result<Safe>(after, 42);
    expect_state(destination, before, 17);
    if (move) {
        destination = std::move(source);
        // Observing the active alternative of a moved-from Result is supported.
        expect_state(source, after, -9); // NOLINT(bugprone-use-after-move)
    } else {
        destination = source;
        expect_state(source, after, 42);
    }
    expect_state(destination, after, 42);
}

void check_assignment_exception(bool before, bool after, bool move) {
    auto destination = make_result<Safe>(before, 17);
    auto source = make_result<Safe>(after, 42);
    const FaultScope fault(move ? Fault::move_assign :
        (before == after ? Fault::copy_assign : Fault::copy_construct));
    if (move) {
        EXPECT_THROW(destination = std::move(source), PayloadException);
        expect_state(source, after, -9); // NOLINT(bugprone-use-after-move)
    } else {
        EXPECT_THROW(destination = source, PayloadException);
        expect_state(source, after, 42);
    }
    // Cross-state temporary construction failure preserves the original payload.
    // Same-state assignment preserves the alternative, with a modified payload.
    expect_state(destination, before, before == after ? 42 : 17);
}

void check_constructor_exception(bool value, bool move) {
    auto source = make_result<Unsafe>(value, 42);
    const FaultScope fault(move ? Fault::move_construct : Fault::copy_construct);
    if (move) {
        EXPECT_THROW(static_cast<void>(Unsafe(std::move(source))), PayloadException);
        expect_state(source, value, -9); // NOLINT(bugprone-use-after-move)
    } else {
        EXPECT_THROW(static_cast<void>(Unsafe(source)), PayloadException);
        expect_state(source, value, 42);
    }
}
} // namespace

TEST(ResultAssignmentTest, ValueToValueCopy) { check_assignment(true, true, false); }
TEST(ResultAssignmentTest, ValueToErrorCopy) { check_assignment(true, false, false); }
TEST(ResultAssignmentTest, ErrorToValueCopy) { check_assignment(false, true, false); }
TEST(ResultAssignmentTest, ErrorToErrorCopy) { check_assignment(false, false, false); }
TEST(ResultAssignmentTest, ValueToValueMove) { check_assignment(true, true, true); }
TEST(ResultAssignmentTest, ValueToErrorMove) { check_assignment(true, false, true); }
TEST(ResultAssignmentTest, ErrorToValueMove) { check_assignment(false, true, true); }
TEST(ResultAssignmentTest, ErrorToErrorMove) { check_assignment(false, false, true); }

TEST(ResultExceptionTest, ValueToValueCopy) { check_assignment_exception(true, true, false); }
TEST(ResultExceptionTest, ValueToErrorCopy) { check_assignment_exception(true, false, false); }
TEST(ResultExceptionTest, ErrorToValueCopy) { check_assignment_exception(false, true, false); }
TEST(ResultExceptionTest, ErrorToErrorCopy) { check_assignment_exception(false, false, false); }
TEST(ResultExceptionTest, ValueToValueMove) { check_assignment_exception(true, true, true); }
TEST(ResultExceptionTest, ErrorToErrorMove) { check_assignment_exception(false, false, true); }
TEST(ResultExceptionTest, CopyConstructValue) { check_constructor_exception(true, false); }
TEST(ResultExceptionTest, CopyConstructError) { check_constructor_exception(false, false); }
TEST(ResultExceptionTest, MoveConstructValue) { check_constructor_exception(true, true); }
TEST(ResultExceptionTest, MoveConstructError) { check_constructor_exception(false, true); }

TEST(ResultExceptionTest, ConversionFactoriesAndConstructors) {
    const Source source{42};
    const FaultScope fault(Fault::convert);
    EXPECT_THROW(static_cast<void>(Safe::success(source)), PayloadException);
    EXPECT_THROW(static_cast<void>(Safe::failure(source)), PayloadException);
    EXPECT_THROW(static_cast<void>((Result<Payload<0>, int>(source))), PayloadException);
    EXPECT_THROW(static_cast<void>((Result<int, Payload<1>>(source))), PayloadException);
    EXPECT_THROW(static_cast<void>(Safe(std::in_place, source)), PayloadException);
}

TEST(ResultExceptionTest, ValueOrCopyPreservesValue) {
    auto result = Safe::success(42);
    const auto& view = result;
    const FaultScope fault(Fault::copy_construct);
    EXPECT_THROW(static_cast<void>(view.value_or(Source{19})), PayloadException);
    expect_state(result, true, 42);
}

TEST(ResultExceptionTest, ValueOrMovePreservesAlternative) {
    auto result = Unsafe::success(42);
    const FaultScope fault(Fault::move_construct);
    EXPECT_THROW(static_cast<void>(std::move(result).value_or(Source{19})), PayloadException);
    expect_state(result, true, -9); // NOLINT(bugprone-use-after-move)
}

TEST(ResultExceptionTest, ValueOrFallbackPreservesError) {
    auto result = Safe::failure(42);
    const auto& view = result;
    const FaultScope fault(Fault::convert);
    EXPECT_THROW(static_cast<void>(view.value_or(Source{19})), PayloadException);
    EXPECT_THROW(static_cast<void>(std::move(result).value_or(Source{19})), PayloadException);
    expect_state(result, false, 42); // NOLINT(bugprone-use-after-move)
}

TEST(ResultConstraintTest, ScalarSpecialMembers) {
    using R = Result<int, long>;
    static_assert(std::is_copy_constructible_v<R> && std::is_move_constructible_v<R>);
    static_assert(std::is_copy_assignable_v<R> && std::is_move_assignable_v<R>);
    static_assert(std::is_nothrow_copy_constructible_v<R> && std::is_nothrow_move_constructible_v<R>);
    static_assert(std::is_nothrow_copy_assignable_v<R> && std::is_nothrow_move_assignable_v<R>);
    static_assert(std::is_trivially_copyable_v<R>);
    constexpr auto result = [] {
        auto target = R::failure(7);
        const auto source = R::success(9);
        target = source;
        target = R::failure(11);
        return target;
    }();
    static_assert(!result.has_value() && result.error() == 11L);
    EXPECT_EQ(result.error(), 11L);
}

TEST(ResultConstraintTest, NothrowMoveProtectsThrowingCopy) {
    static_assert(std::is_copy_constructible_v<Safe> && std::is_move_constructible_v<Safe>);
    static_assert(std::is_copy_assignable_v<Safe> && std::is_move_assignable_v<Safe>);
    static_assert(!std::is_nothrow_copy_constructible_v<Safe> && std::is_nothrow_move_constructible_v<Safe>);
    static_assert(!std::is_nothrow_copy_assignable_v<Safe> && !std::is_nothrow_move_assignable_v<Safe>);
    const auto source = Safe::failure(42);
    auto target = source;
    expect_state(target, false, 42);
}

TEST(ResultConstraintTest, UnsafeValueAssignmentsRejected) {
    using R = Result<Payload<0, false>, int>;
    static_assert(std::is_copy_constructible_v<R> && std::is_move_constructible_v<R>);
    static_assert(!std::is_copy_assignable_v<R> && !std::is_move_assignable_v<R>);
    static_assert(!std::is_nothrow_move_constructible_v<R> && !std::is_nothrow_move_assignable_v<R>);
    auto result = R::success(42);
    EXPECT_EQ(result.value().id, 42);
}

TEST(ResultConstraintTest, UnsafeErrorAssignmentsRejected) {
    using R = Result<int, Payload<1, false>>;
    static_assert(std::is_copy_constructible_v<R> && std::is_move_constructible_v<R>);
    static_assert(!std::is_copy_assignable_v<R> && !std::is_move_assignable_v<R>);
    static_assert(!std::is_nothrow_move_constructible_v<R> && !std::is_nothrow_move_assignable_v<R>);
    auto result = R::failure(42);
    EXPECT_EQ(result.error().id, 42);
}

TEST(ResultConstraintTest, NothrowCopyAllowsCopyButRejectsMoveAssignment) {
    using V = Result<NothrowCopy, int>;
    using E = Result<int, NothrowCopy>;
    static_assert(std::is_copy_assignable_v<V> && !std::is_move_assignable_v<V>);
    static_assert(std::is_copy_assignable_v<E> && !std::is_move_assignable_v<E>);
    static_assert(!std::is_nothrow_move_constructible_v<V> && !std::is_nothrow_move_assignable_v<V>);
    auto target_v = V::failure(17);
    const auto source_v = V::success(42);
    auto target_e = E::success(17);
    const auto source_e = E::failure(42);
    const FaultScope fault(Fault::move_construct); // Copy must not call throwing move.
    target_v = source_v;
    target_e = source_e;
    EXPECT_EQ(target_v.value().id, 42);
    EXPECT_EQ(target_e.error().id, 42);
}

TEST(ResultConstraintTest, MoveOnlyValueAndError) {
    using V = std::unique_ptr<int>;
    using E = std::unique_ptr<long>;
    using R = Result<V, E>;
    static_assert(!std::is_copy_constructible_v<R> && !std::is_copy_assignable_v<R>);
    static_assert(std::is_nothrow_move_constructible_v<R> && std::is_nothrow_move_assignable_v<R>);
    auto value = R::success(std::make_unique<int>(42));
    auto error = R::failure(std::make_unique<long>(17));
    auto moved = std::move(value);
    EXPECT_EQ(*moved.value(), 42);
    moved = std::move(error);
    EXPECT_EQ(*moved.error(), 17L);
    // Result promises an active alternative after moving its payload out.
    // These observations test that contract; no moved-from unique_ptr is dereferenced.
    EXPECT_NE(value.value_if(), nullptr); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    EXPECT_EQ(value.error_if(), nullptr);
    EXPECT_NE(error.error_if(), nullptr); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    EXPECT_EQ(error.value_if(), nullptr);
}

TEST(ResultConstraintTest, ImmovableNonDefaultConstruction) {
    using V = Result<Immovable, int>;
    using E = Result<int, Immovable>;
    static_assert(!std::is_default_constructible_v<Immovable>);
    static_assert(!std::is_copy_constructible_v<V> && !std::is_move_constructible_v<V>);
    static_assert(!std::is_copy_assignable_v<V> && !std::is_move_assignable_v<V>);
    static_assert(!std::is_copy_constructible_v<E> && !std::is_move_constructible_v<E>);
    static_assert(!std::is_copy_assignable_v<E> && !std::is_move_assignable_v<E>);
    auto value = V::success(42);
    auto error = E::failure(17);
    EXPECT_EQ(value.value().id, 42);
    EXPECT_EQ(error.error().id, 17);
}

TEST(ResultConstraintTest, ExceptionSpecifications) {
    static_assert(noexcept(Safe::success(42)) && noexcept(Safe::failure(42)));
    static_assert(!noexcept(Safe::success(Source{42})) && !noexcept(Safe::failure(Source{42})));
    static_assert(noexcept(Safe(std::in_place, 42)) && !noexcept(Safe(std::in_place, Source{42})));
    static_assert(!std::is_nothrow_constructible_v<Safe, const Payload<0>&>);
    static_assert(std::is_nothrow_constructible_v<Safe, Payload<0>&&>);
    static_assert(!std::is_nothrow_constructible_v<Safe, const Payload<1>&>);
    static_assert(std::is_nothrow_constructible_v<Safe, Payload<1>&&>);
    static_assert(!std::is_nothrow_copy_constructible_v<Unsafe> && !std::is_nothrow_move_constructible_v<Unsafe>);
    static_assert(!std::is_copy_assignable_v<Unsafe> && !std::is_move_assignable_v<Unsafe>);
    static_assert(noexcept(std::declval<const Safe&>().has_value()));
    static_assert(noexcept(std::declval<const Safe&>().IsSuccess()));
    static_assert(noexcept(static_cast<bool>(std::declval<const Safe&>())));
    static_assert(noexcept(std::declval<Safe&>().value()) && noexcept(std::declval<Safe&>().Value()));
    static_assert(noexcept(std::declval<Safe&>().error()));
    static_assert(noexcept(std::declval<Safe&>().value_if()) && noexcept(std::declval<Safe&>().error_if()));
    static_assert(!noexcept(std::declval<Safe&>().value_or(Source{42})));
    EXPECT_TRUE(Safe::success(42).has_value());
}

TEST(ResultForwardingTest, MutableLvaluePayloads) {
    Payload<0> value{42};
    Payload<1> error{17};
    auto success = Safe::success(value);
    auto failure = Safe::failure(error);
    expect_state(success, true, 42);
    expect_state(failure, false, 17);
    EXPECT_EQ(value.id, 42);
    EXPECT_EQ(error.id, 17);
}

TEST(ResultForwardingTest, ConstLvaluePayloads) {
    const Payload<0> value{42};
    const Payload<1> error{17};
    auto success = Safe::success(value);
    auto failure = Safe::failure(error);
    expect_state(success, true, 42);
    expect_state(failure, false, 17);
}

TEST(ResultForwardingTest, RvaluePayloads) {
    auto success = Safe::success(Payload<0>{42});
    auto failure = Safe::failure(Payload<1>{17});
    expect_state(success, true, 42);
    expect_state(failure, false, 17);
}

TEST(ResultForwardingTest, ConversionPayloads) {
    auto success = Safe::success(Source{42});
    auto failure = Safe::failure(Source{17});
    expect_state(success, true, 42);
    expect_state(failure, false, 17);
}

TEST(ResultExceptionTest, FactoryCopyPayloadsPropagate) {
    const Payload<0> value{42};
    const Payload<1> error{17};
    const FaultScope fault(Fault::copy_construct);
    EXPECT_THROW(static_cast<void>(Safe::success(value)), PayloadException);
    EXPECT_THROW(static_cast<void>(Safe::failure(error)), PayloadException);
    EXPECT_EQ(value.id, 42);
    EXPECT_EQ(error.id, 17);
}

TEST(ResultExceptionTest, FactoryMovePayloadsPropagate) {
    Payload<0, false> value{42};
    Payload<1, false> error{17};
    const FaultScope fault(Fault::move_construct);
    EXPECT_THROW(static_cast<void>(Unsafe::success(std::move(value))), PayloadException);
    EXPECT_THROW(static_cast<void>(Unsafe::failure(std::move(error))), PayloadException);
    EXPECT_EQ(value.id, -9); // NOLINT(bugprone-use-after-move)
    EXPECT_EQ(error.id, -9); // NOLINT(bugprone-use-after-move)
}
