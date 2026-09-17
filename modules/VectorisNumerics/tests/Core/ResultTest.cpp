#include <gtest/gtest.h>
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
    EXPECT_EQ(std::move(r_const).Value(), "const_str");

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
    auto r_fail = Result<int>::failure(MathError::ill_conditioned);
    EXPECT_EQ(std::move(r_fail).error(), MathError::ill_conditioned);

    // const error() const &&
    const auto r_fail_const = Result<int>::failure(MathError::max_iterations);
    EXPECT_EQ(std::move(r_fail_const).error(), MathError::max_iterations);

    // Quaternion Result instantiation coverage
    struct ResultFrameA {};
    struct ResultFrameB {};
    using QuatT = vectoris::numerics::Geometry::Quaternion<double, ResultFrameA, ResultFrameB>;
    auto q_res = Result<QuatT>::failure(MathError::invalid_state);
    EXPECT_EQ(q_res.error(), MathError::invalid_state);
    EXPECT_EQ(std::move(q_res).error(), MathError::invalid_state);

    auto q_val = QuatT::TryCreate(1.0, 0.0, 0.0, 0.0).Value();
    auto q_ok = Result<QuatT>::success(q_val);
    EXPECT_TRUE(q_ok.has_value());
    EXPECT_DOUBLE_EQ(q_ok.value().w, 1.0);
    EXPECT_DOUBLE_EQ(q_ok.Value().w, 1.0);
    const auto& q_ok_const = q_ok;
    EXPECT_DOUBLE_EQ(q_ok_const.value().w, 1.0);
    EXPECT_DOUBLE_EQ(q_ok_const.Value().w, 1.0);
    EXPECT_DOUBLE_EQ(std::move(q_ok).Value().w, 1.0);
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

