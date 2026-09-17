#pragma once
#include <cstdint>

namespace vectoris::numerics::Core {

    // 强类型数学错误码枚举 (遵从 ISO C++20 与 Engineering Standard v1.0 Section 46)
    enum class MathError : std::uint8_t {
        invalid_argument = 1,
        domain_error,
        non_finite_input,
        singular_matrix,
        ill_conditioned,
        zero_norm,
        normalization_failure,
        non_convergence,
        max_iterations,
        invalid_state
    };

    [[nodiscard]] constexpr const char* to_string(MathError err) noexcept {
        switch (err) {
            case MathError::invalid_argument:      return "invalid_argument";
            case MathError::domain_error:          return "domain_error";
            case MathError::non_finite_input:      return "non_finite_input";
            case MathError::singular_matrix:       return "singular_matrix";
            case MathError::ill_conditioned:       return "ill_conditioned";
            case MathError::zero_norm:             return "zero_norm";
            case MathError::normalization_failure: return "normalization_failure";
            case MathError::non_convergence:       return "non_convergence";
            case MathError::max_iterations:        return "max_iterations";
            case MathError::invalid_state:         return "invalid_state";
        }
        return "unknown_error";
    }

} // namespace vectoris::numerics::Core
