#include <gtest/gtest.h>
#include <string>
#include <type_traits>
#include <utility>
#include "Vectoris/Numerics/Core/Result.h"
#include "Vectoris/Numerics/Core/MathError.h"
#include "Vectoris/Numerics/Geometry/Matrix3.h"

using namespace vectoris::numerics::Core;

namespace {

    struct Tracked {
        static inline int constructions = 0;
        static inline int destructions = 0;
        static inline int copies = 0;
        static inline int moves = 0;

        static void reset() {
            constructions = 0;
            destructions = 0;
            copies = 0;
            moves = 0;
        }

        int id{0};

        explicit Tracked(int i) : id(i) {
            ++constructions;
        }

        Tracked(const Tracked& other) : id(other.id) {
            ++copies;
        }

        Tracked(Tracked&& other) noexcept : id(other.id) {
            other.id = -1;
            ++moves;
        }

        Tracked& operator=(const Tracked& other) {
            id = other.id;
            ++copies;
            return *this;
        }

        Tracked& operator=(Tracked&& other) noexcept {
            id = other.id;
            other.id = -1;
            ++moves;
            return *this;
        }

        ~Tracked() {
            ++destructions;
        }
    };

} // namespace

TEST(ResultTest, Success) {
    auto r = Result<int>::success(42);
    EXPECT_TRUE(r.has_value());
    EXPECT_TRUE(r.IsSuccess());
    EXPECT_TRUE(static_cast<bool>(r));
    EXPECT_EQ(r.value(), 42);
    EXPECT_EQ(r.Value(), 42);

    r.value() = 99;
    EXPECT_EQ(r.value(), 99);
}

TEST(ResultTest, Failure) {
    auto r = Result<int>::failure(MathError::domain_error);
    EXPECT_FALSE(r.has_value());
    EXPECT_FALSE(r.IsSuccess());
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(r.error(), MathError::domain_error);
}

TEST(ResultTest, ErrorPayload) {
    auto r1 = Result<double>::failure(MathError::singular_matrix);
    EXPECT_EQ(r1.error(), MathError::singular_matrix);
    EXPECT_STREQ(to_string(r1.error()), "singular_matrix");

    auto r2 = Result<double>::failure(MathError::zero_norm);
    EXPECT_EQ(r2.error(), MathError::zero_norm);
    EXPECT_STREQ(to_string(r2.error()), "zero_norm");

    auto r3 = Result<double>::failure(MathError::non_finite_input);
    EXPECT_EQ(r3.error(), MathError::non_finite_input);
    EXPECT_STREQ(to_string(r3.error()), "non_finite_input");

    auto r4 = Result<double>::failure(MathError::ill_conditioned);
    EXPECT_EQ(r4.error(), MathError::ill_conditioned);
    EXPECT_STREQ(to_string(r4.error()), "ill_conditioned");

    auto r5 = Result<double>::failure(MathError::normalization_failure);
    EXPECT_EQ(r5.error(), MathError::normalization_failure);
    EXPECT_STREQ(to_string(r5.error()), "normalization_failure");

    auto r6 = Result<double>::failure(MathError::non_convergence);
    EXPECT_EQ(r6.error(), MathError::non_convergence);
    EXPECT_STREQ(to_string(r6.error()), "non_convergence");

    auto r7 = Result<double>::failure(MathError::max_iterations);
    EXPECT_EQ(r7.error(), MathError::max_iterations);
    EXPECT_STREQ(to_string(r7.error()), "max_iterations");

    auto r8 = Result<double>::failure(MathError::invalid_state);
    EXPECT_EQ(r8.error(), MathError::invalid_state);
    EXPECT_STREQ(to_string(r8.error()), "invalid_state");

    auto r9 = Result<double>::failure(MathError::invalid_argument);
    EXPECT_EQ(r9.error(), MathError::invalid_argument);
    EXPECT_STREQ(to_string(r9.error()), "invalid_argument");

    auto r10 = Result<double>::failure(MathError::domain_error);
    EXPECT_EQ(r10.error(), MathError::domain_error);
    EXPECT_STREQ(to_string(r10.error()), "domain_error");

    EXPECT_STREQ(to_string(static_cast<MathError>(255)), "unknown_error");
}

TEST(ResultTest, ConstexprSuccess) {
    constexpr auto r = Result<int>::success(42);
    static_assert(r.has_value());
    static_assert(r.IsSuccess());
    static_assert(r.value() == 42);
    static_assert(r.Value() == 42);
    static_assert(r.value_if() != nullptr);
    static_assert(*r.value_if() == 42);
    static_assert(r.error_if() == nullptr);
    EXPECT_EQ(r.value(), 42);
}

TEST(ResultTest, ConstexprFailure) {
    constexpr auto r = Result<int>::failure(MathError::domain_error);
    static_assert(!r.has_value());
    static_assert(!r.IsSuccess());
    static_assert(r.error() == MathError::domain_error);
    static_assert(r.value_if() == nullptr);
    static_assert(r.error_if() != nullptr);
    static_assert(*r.error_if() == MathError::domain_error);
    EXPECT_EQ(r.error(), MathError::domain_error);
}

TEST(ResultTest, Copy) {
    auto r1 = Result<int>::success(123);
    Result<int> r2 = r1;
    EXPECT_TRUE(r2.has_value());
    EXPECT_EQ(r2.value(), 123);

    auto f1 = Result<int>::failure(MathError::ill_conditioned);
    Result<int> f2 = f1;
    EXPECT_FALSE(f2.has_value());
    EXPECT_EQ(f2.error(), MathError::ill_conditioned);
}

TEST(ResultTest, Move) {
    auto r1 = Result<std::string>::success("hello");
    Result<std::string> r2 = std::move(r1);
    EXPECT_TRUE(r2.has_value());
    EXPECT_EQ(r2.value(), "hello");

    auto f1 = Result<std::string>::failure(MathError::invalid_argument);
    Result<std::string> f2 = std::move(f1);
    EXPECT_FALSE(f2.has_value());
    EXPECT_EQ(f2.error(), MathError::invalid_argument);
}

TEST(ResultTest, NonTrivialLifetime) {
    Tracked::reset();
    {
        Result<Tracked> r = Result<Tracked>::success(Tracked(10));
        EXPECT_TRUE(r.has_value());
        EXPECT_EQ(r.value().id, 10);
    }
    // Tracked(10) constructed, moved into Result, temporary destructed, Result destructed
    EXPECT_EQ(Tracked::constructions, 1);
    EXPECT_EQ(Tracked::moves, 1);
    EXPECT_EQ(Tracked::destructions, 2);

    Tracked::reset();
    {
        Result<Tracked> r_err = Result<Tracked>::failure(MathError::zero_norm);
        EXPECT_FALSE(r_err.has_value());
        EXPECT_EQ(r_err.error(), MathError::zero_norm);
    }
    // No Tracked instances should be constructed or destructed in failure state
    EXPECT_EQ(Tracked::constructions, 0);
    EXPECT_EQ(Tracked::destructions, 0);
}

TEST(ResultTest, ValueOr) {
    auto r_ok = Result<int>::success(10);
    EXPECT_EQ(r_ok.value_or(20), 10);

    auto r_fail = Result<int>::failure(MathError::domain_error);
    EXPECT_EQ(r_fail.value_or(20), 20);
}

TEST(ResultTest, SizeAndAlignmentInspection) {
    // Phase 11: 记录常用数值类型包装下的布局与内存开销
    EXPECT_EQ(sizeof(Result<double>), 16);
    EXPECT_EQ(alignof(Result<double>), 8);

    EXPECT_EQ(sizeof(Result<vectoris::numerics::Geometry::Matrix3<double>>), 80);
    EXPECT_EQ(alignof(Result<vectoris::numerics::Geometry::Matrix3<double>>), 8);
}

TEST(ResultTest, ValueIfSuccess) {
    auto r = Result<int>::success(100);
    int* ptr = r.value_if();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, 100);

    const auto& cr = r;
    const int* cptr = cr.value_if();
    ASSERT_NE(cptr, nullptr);
    EXPECT_EQ(*cptr, 100);

    // 修改值
    *ptr = 200;
    EXPECT_EQ(r.value(), 200);
}

TEST(ResultTest, ValueIfFailure) {
    auto r = Result<int>::failure(MathError::zero_norm);
    EXPECT_EQ(r.value_if(), nullptr);

    const auto& cr = r;
    EXPECT_EQ(cr.value_if(), nullptr);
}

TEST(ResultTest, ErrorIfFailure) {
    auto r = Result<int>::failure(MathError::singular_matrix);
    MathError* err_ptr = r.error_if();
    ASSERT_NE(err_ptr, nullptr);
    EXPECT_EQ(*err_ptr, MathError::singular_matrix);

    const auto& cr = r;
    const MathError* cerr_ptr = cr.error_if();
    ASSERT_NE(cerr_ptr, nullptr);
    EXPECT_EQ(*cerr_ptr, MathError::singular_matrix);
}

TEST(ResultTest, ErrorIfSuccess) {
    auto r = Result<int>::success(42);
    EXPECT_EQ(r.error_if(), nullptr);

    const auto& cr = r;
    EXPECT_EQ(cr.error_if(), nullptr);
}

#include "Vectoris/Numerics/Geometry/Quaternion.h"

TEST(ResultTest, RvalueAndRefQualifiedAccessors) {
    // Value() &
    auto r_ok = Result<int>::success(100);
    EXPECT_EQ(r_ok.Value(), 100);
    r_ok.Value() = 105;
    EXPECT_EQ(r_ok.value(), 105);

    // const T& Value() const &
    const auto r_const_ref = Result<int>::success(200);
    EXPECT_EQ(r_const_ref.Value(), 200);

    // const T&& Value() const &&
    const auto r_const = Result<std::string>::success("const_str");
    EXPECT_EQ(static_cast<const Result<std::string>&&>(r_const).Value(), "const_str");

    // T&& Value() &&
    auto r_str = Result<std::string>::success("move_str");
    EXPECT_EQ(std::move(r_str).Value(), "move_str");

    // T&& value() &&
    auto r_str2 = Result<std::string>::success("move_val");
    EXPECT_EQ(std::move(r_str2).value(), "move_val");

    // value_or with rvalue Result
    auto r_val_or_ok = Result<std::string>::success("hello");
    EXPECT_EQ(std::move(r_val_or_ok).value_or("default"), "hello");

    auto r_val_or_fail = Result<std::string>::failure(MathError::domain_error);
    EXPECT_EQ(std::move(r_val_or_fail).value_or("default"), "default");

    // error() &&
    auto r_fail = Result<std::string>::failure(MathError::ill_conditioned);
    EXPECT_EQ(std::move(r_fail).error(), MathError::ill_conditioned);

    // const error() const &&
    const auto r_fail_const = Result<std::string>::failure(MathError::max_iterations);
    EXPECT_EQ(static_cast<const Result<std::string>&&>(r_fail_const).error(), MathError::max_iterations);

    // Quaternion Result instantiation coverage
    struct ResultFrameA {};
    struct ResultFrameB {};
    using QuatT = vectoris::numerics::Geometry::Quaternion<double, ResultFrameA, ResultFrameB>;
    auto q_res = Result<QuatT>::failure(MathError::invalid_state);
    EXPECT_EQ(q_res.error(), MathError::invalid_state);
    EXPECT_EQ(static_cast<Result<QuatT>&&>(q_res).error(), MathError::invalid_state);

    auto q_val = QuatT::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    auto q_ok = Result<QuatT>::success(q_val);
    EXPECT_TRUE(q_ok.has_value());
    EXPECT_DOUBLE_EQ(q_ok.value().w, 1.0);
    EXPECT_DOUBLE_EQ(q_ok.Value().w, 1.0);
    const auto& q_ok_const = q_ok;
    EXPECT_DOUBLE_EQ(q_ok_const.value().w, 1.0);
    EXPECT_DOUBLE_EQ(q_ok_const.Value().w, 1.0);
    EXPECT_DOUBLE_EQ(static_cast<Result<QuatT>&&>(q_ok).Value().w, 1.0);
}

#if !defined(NDEBUG)
TEST(ResultDeathTest, ValueCalledOnErrorResultAborts) {
    auto r = Result<int>::failure(MathError::domain_error);
    EXPECT_DEATH(static_cast<void>(r.value()), ".*Vectoris Precondition Violation.*");
    EXPECT_DEATH(static_cast<void>(r.Value()), ".*Vectoris Precondition Violation.*");
    const auto& cr = r;
    EXPECT_DEATH(static_cast<void>(cr.value()), ".*Vectoris Precondition Violation.*");
    EXPECT_DEATH(static_cast<void>(cr.Value()), ".*Vectoris Precondition Violation.*");
}

TEST(ResultDeathTest, ErrorCalledOnSuccessResultAborts) {
    auto r = Result<int>::success(42);
    EXPECT_DEATH(static_cast<void>(r.error()), ".*Vectoris Precondition Violation.*");
    const auto& cr = r;
    EXPECT_DEATH(static_cast<void>(cr.error()), ".*Vectoris Precondition Violation.*");
}
#endif

TEST(ResultFactoryTest, FailureAlwaysSelectsErrorAlternative) {
    auto result = Result<int, long>::failure(3);
    ASSERT_FALSE(result.has_value());
    ASSERT_NE(result.error_if(), nullptr);
    EXPECT_EQ(result.value_if(), nullptr);
    EXPECT_EQ(result.error(), 3L);
    EXPECT_EQ(result.value_or(99), 99);
}

TEST(ResultFactoryTest, SuccessAlwaysSelectsValueAlternative) {
    auto result = Result<double, int>::success(3);
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result.value_if(), nullptr);
    EXPECT_EQ(result.error_if(), nullptr);
    EXPECT_DOUBLE_EQ(result.value(), 3.0);
    EXPECT_DOUBLE_EQ(result.value_or(99.0), 3.0);
}
namespace {
struct DualNumericSource {
    constexpr operator int() const noexcept { return 11; }
    constexpr operator long() const noexcept { return 29L; }
};
struct CategorySource {
    int id;
    explicit CategorySource(int value) noexcept : id(value) {}
    CategorySource(const CategorySource&) = default;
    CategorySource& operator=(const CategorySource&) = default;
    CategorySource(CategorySource&& other) noexcept : id(std::exchange(other.id, -1)) {}
    CategorySource& operator=(CategorySource&& other) noexcept {
        id = std::exchange(other.id, -1);
        return *this;
    }
    ~CategorySource() = default;
};
template <int Tag> struct CategoryPayload {
    int id;
    int category;
    CategoryPayload() = delete;
    explicit CategoryPayload(CategorySource& source) noexcept : id(source.id), category(1) {}
    explicit CategoryPayload(const CategorySource& source) noexcept(false) : id(source.id), category(2) {}
    explicit CategoryPayload(CategorySource&& source) noexcept
        : id(CategorySource(std::move(source)).id), category(3) {}
};
template <int Tag> struct MoveOnlyPayload {
    int id;
    MoveOnlyPayload() = delete;
    explicit MoveOnlyPayload(int value) noexcept : id(value) {}
    MoveOnlyPayload(const MoveOnlyPayload&) = delete;
    MoveOnlyPayload& operator=(const MoveOnlyPayload&) = delete;
    MoveOnlyPayload(MoveOnlyPayload&& other) noexcept : id(std::exchange(other.id, -1)) {}
    MoveOnlyPayload& operator=(MoveOnlyPayload&& other) noexcept {
        id = std::exchange(other.id, -1);
        return *this;
    }
    ~MoveOnlyPayload() = default;
};
struct FactoryConstructionError {};
struct NoThrowPayload {
    int id;
    explicit NoThrowPayload(int value) noexcept : id(value) {}
};
struct ThrowingPayload {
    explicit ThrowingPayload(int) { throw FactoryConstructionError{}; }
};
struct IndexAwarePayload {
    int category{0};
    IndexAwarePayload() noexcept = default;
    explicit IndexAwarePayload(std::in_place_index_t<0>) noexcept : category(9) {}
};
template <typename R, typename A> concept CanSucceed = requires(A&& value) {
    R::success(std::forward<A>(value));
};
template <typename R, typename A> concept CanFail = requires(A&& value) {
    R::failure(std::forward<A>(value));
};
} // namespace

TEST(ResultFactoryTest, DualConvertibleSourceAndCheckedPointers) {
    using R = Result<int, long>;
    static_assert(std::is_convertible_v<DualNumericSource, int>);
    static_assert(std::is_convertible_v<DualNumericSource, long>);
    const auto check = [](auto&& value_source, auto&& error_source) {
        auto ok = R::success(std::forward<decltype(value_source)>(value_source));
        auto error = R::failure(std::forward<decltype(error_source)>(error_source));
        ASSERT_TRUE(ok.has_value());
        ASSERT_FALSE(error.has_value());
        ASSERT_NE(ok.value_if(), nullptr);
        ASSERT_NE(error.error_if(), nullptr);
        EXPECT_EQ(*ok.value_if(), 11);
        EXPECT_EQ(*error.error_if(), 29L);
        EXPECT_EQ(ok.error_if(), nullptr);
        EXPECT_EQ(error.value_if(), nullptr);
        const auto& const_ok = ok;
        const auto& const_error = error;
        EXPECT_EQ(const_ok.value_if(), ok.value_if());
        EXPECT_EQ(const_ok.error_if(), nullptr);
        EXPECT_EQ(const_error.error_if(), error.error_if());
        EXPECT_EQ(const_error.value_if(), nullptr);
        EXPECT_TRUE(static_cast<bool>(ok));
        EXPECT_FALSE(static_cast<bool>(error));
    };
    DualNumericSource value_source;
    DualNumericSource error_source;
    const DualNumericSource const_value_source;
    const DualNumericSource const_error_source;
    check(value_source, error_source);
    check(const_value_source, const_error_source);
    check(DualNumericSource{}, DualNumericSource{});
}

TEST(ResultFactoryTest, ForwardingCategoryAndConditionalNoexcept) {
    using R = Result<CategoryPayload<0>, CategoryPayload<1>>;
    static_assert(!std::is_default_constructible_v<CategoryPayload<0>>);
    static_assert(!std::is_default_constructible_v<CategoryPayload<1>>);
    static_assert(noexcept(R::success(std::declval<CategorySource&>())));
    static_assert(!noexcept(R::success(std::declval<const CategorySource&>())));
    static_assert(noexcept(R::success(std::declval<CategorySource&&>())));
    static_assert(noexcept(R::failure(std::declval<CategorySource&>())));
    static_assert(!noexcept(R::failure(std::declval<const CategorySource&>())));
    static_assert(noexcept(R::failure(std::declval<CategorySource&&>())));
    const auto check = [](auto&& value_source, auto&& error_source, int expected_category) {
        auto ok = R::success(std::forward<decltype(value_source)>(value_source));
        auto error = R::failure(std::forward<decltype(error_source)>(error_source));
        ASSERT_TRUE(ok.has_value());
        ASSERT_FALSE(error.has_value());
        EXPECT_EQ(ok.value().category, expected_category);
        EXPECT_EQ(error.error().category, expected_category);
        EXPECT_EQ(ok.value().id, 42);
        EXPECT_EQ(error.error().id, 42);
    };
    CategorySource value_source{42};
    CategorySource error_source{42};
    const CategorySource const_value_source{42};
    const CategorySource const_error_source{42};
    check(value_source, error_source, 1);
    check(const_value_source, const_error_source, 2);
    check(CategorySource{42}, CategorySource{42}, 3);
}

TEST(ResultFactoryTest, NoexceptTracksSelectedAlternativeAndPropagatesConstructionException) {
    using ValueNoThrow = Result<NoThrowPayload, ThrowingPayload>;
    using ErrorNoThrow = Result<ThrowingPayload, NoThrowPayload>;
    static_assert(noexcept(ValueNoThrow::success(7)));
    static_assert(!noexcept(ValueNoThrow::failure(7)));
    static_assert(!noexcept(ErrorNoThrow::success(7)));
    static_assert(noexcept(ErrorNoThrow::failure(7)));
    auto ok = ValueNoThrow::success(7);
    ASSERT_TRUE(ok.has_value());
    EXPECT_EQ(ok.value().id, 7);
    auto error = ErrorNoThrow::failure(7);
    ASSERT_FALSE(error.has_value());
    EXPECT_EQ(error.error().id, 7);
    // Construction fails before a Result exists. This does not test or repair VRT-13 assignment.
    EXPECT_THROW(static_cast<void>(ValueNoThrow::failure(7)), FactoryConstructionError);
    EXPECT_THROW(static_cast<void>(ErrorNoThrow::success(7)), FactoryConstructionError);
}

TEST(ResultFactoryTest, MoveOnlyNonDefaultValueAndError) {
    using V = MoveOnlyPayload<0>;
    using E = MoveOnlyPayload<1>;
    using R = Result<V, E>;
    static_assert(!std::is_default_constructible_v<V> && !std::is_default_constructible_v<E>);
    static_assert(!std::is_copy_constructible_v<R> && !std::is_copy_assignable_v<R>);
    static_assert(std::is_nothrow_move_constructible_v<R> && std::is_nothrow_move_assignable_v<R>);
    static_assert(CanSucceed<R, V> && !CanSucceed<R, V&> && !CanSucceed<R, const V&>);
    static_assert(CanFail<R, E> && !CanFail<R, E&> && !CanFail<R, const E&>);
    auto source_value = R::success(V{7});
    auto value = std::move(source_value);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value().id, 7);
    auto source_error = R::failure(E{9});
    auto error = std::move(source_error);
    ASSERT_FALSE(error.has_value());
    EXPECT_EQ(error.error().id, 9);
    value = std::move(error); // cross-alternative move assignment
    ASSERT_FALSE(value.has_value());
    EXPECT_EQ(value.error().id, 9);
    value = R::success(11); // direct construction of non-default, move-only T from int
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value().id, 11);
    auto extracted = std::move(value).value_or(V{99});
    EXPECT_EQ(extracted.id, 11);
    auto failed = R::failure(13); // direct construction of non-default, move-only E from int
    auto fallback = std::move(failed).value_or(V{99});
    EXPECT_EQ(fallback.id, 99);
}

TEST(ResultFactoryTest, CopyAndAssignmentKeepTheChosenState) {
    using R = Result<int, long>;
    const auto ok = R::success(4L);
    const auto error = R::failure(8);
    auto value_copy = ok;
    auto error_copy = error;
    ASSERT_TRUE(value_copy.has_value());
    ASSERT_FALSE(error_copy.has_value());
    EXPECT_EQ(value_copy.value(), 4);
    EXPECT_EQ(error_copy.error(), 8L);
    value_copy = error;
    error_copy = ok;
    ASSERT_FALSE(value_copy.has_value());
    ASSERT_TRUE(error_copy.has_value());
    EXPECT_EQ(value_copy.error(), 8L);
    EXPECT_EQ(error_copy.value(), 4);
    auto next_value = error_copy;
    auto next_error = value_copy;
    ASSERT_TRUE(next_value.has_value());
    ASSERT_FALSE(next_error.has_value());
    next_value = R::failure(12);
    next_error = R::success(16L);
    ASSERT_FALSE(next_value.has_value());
    ASSERT_TRUE(next_error.has_value());
    EXPECT_EQ(next_value.error(), 12L);
    EXPECT_EQ(next_error.value(), 16);
    next_value = R::failure(20); // same-alternative assignment
    next_error = R::success(24L);
    EXPECT_EQ(next_value.error(), 20L);
    EXPECT_EQ(next_error.value(), 24);
}

TEST(ResultFactoryTest, ConstexprVariadicAndDirectConstructionCompatibility) {
    constexpr auto ok = Result<double, int>::success(3);
    constexpr auto error = Result<int, long>::failure(3);
    static_assert(ok.has_value() && ok.error_if() == nullptr);
    static_assert(!error.has_value() && error.value_if() == nullptr);
    static_assert(*error.error_if() == 3L);
    EXPECT_DOUBLE_EQ(ok.value(), 3.0);
    EXPECT_EQ(error.error(), 3L);
    auto empty = Result<int, long>::success();
    ASSERT_TRUE(empty.has_value());
    EXPECT_EQ(empty.value(), 0);
    auto text = Result<std::string, int>::success(std::size_t{3}, 'x');
    ASSERT_TRUE(text.has_value());
    EXPECT_EQ(text.value(), "xxx");
    Result<std::string, int> direct_in_place(std::in_place, std::size_t{2}, 'y');
    ASSERT_TRUE(direct_in_place.has_value());
    EXPECT_EQ(direct_in_place.value(), "yy");
    Result<int, long> direct_value(5);
    Result<int, long> direct_error(6L);
    ASSERT_TRUE(direct_value.has_value());
    ASSERT_FALSE(direct_error.has_value());
    EXPECT_EQ(direct_value.value(), 5);
    EXPECT_EQ(direct_error.error(), 6L);
    auto explicit_template = Result<int, long>::success<int>(7);
    auto explicit_failure = Result<int, long>::failure<long>(8L);
    ASSERT_TRUE(explicit_template.has_value());
    ASSERT_FALSE(explicit_failure.has_value());
    EXPECT_EQ(explicit_template.value(), 7);
    EXPECT_EQ(explicit_failure.error(), 8L);
}

TEST(ResultFactoryTest, FactoryConstraintsAndIndexTokensAsPayload) {
    using R = Result<IndexAwarePayload, long>;
    auto default_value = R::success();
    auto token_value = R::success(std::in_place_index<0>);
    ASSERT_TRUE(default_value.has_value());
    ASSERT_TRUE(token_value.has_value());
    EXPECT_EQ(default_value.value().category, 0);
    EXPECT_EQ(token_value.value().category, 9);
    R direct_token(std::in_place_index<0>); // existing direct constructor still treats the token as payload
    ASSERT_TRUE(direct_token.has_value());
    EXPECT_EQ(direct_token.value().category, 9);
    static_assert(!CanSucceed<Result<int, long>, std::string>);
    static_assert(!CanFail<Result<int, long>, std::string>);
    static_assert(CanSucceed<Result<NoThrowPayload, long>, int>);
    static_assert(!CanFail<Result<NoThrowPayload, long>, NoThrowPayload>);
    static_assert(!CanSucceed<Result<int, NoThrowPayload>, NoThrowPayload>);
    static_assert(CanFail<Result<int, NoThrowPayload>, int>);
    static_assert(!std::is_default_constructible_v<Result<int, long>>);
}
