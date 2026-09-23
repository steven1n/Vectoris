#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string_view>

#include "Vectoris/Numerics/Geometry/Matrix3.h"
#include "Vectoris/Numerics/Geometry/Vector3.h"

namespace {
struct BenchFrame final {};
using Matrix = vectoris::numerics::geometry::Matrix3<double>;
using Vector = vectoris::numerics::geometry::Vector3<double, BenchFrame>;
constexpr std::uint64_t kSeed = 0x6d617472697833ULL;
constexpr std::uint64_t kIterations = 200'000;
constexpr int kSamples = 9;
volatile double benchmark_sink = 0.0;

struct Corpus final {
    std::array<Matrix, 64> lhs{};
    std::array<Matrix, 64> rhs{};
    std::array<Vector, 64> vectors{};
};

std::uint64_t next_state(std::uint64_t& state) noexcept {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    return state;
}

double bounded_value(std::uint64_t& state) noexcept {
    const auto integer = static_cast<std::int32_t>((next_state(state) >> 32U) % 2001U) - 1000;
    return static_cast<double>(integer) / 4000.0;
}

Corpus make_corpus() noexcept {
    Corpus corpus{};
    volatile std::uint64_t seed = kSeed;
    std::uint64_t state = seed;
    for (std::size_t sample = 0; sample < corpus.lhs.size(); ++sample) {
        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t column = 0; column < 3; ++column) {
                corpus.lhs[sample](row, column) = bounded_value(state);
                corpus.rhs[sample](row, column) = bounded_value(state);
            }
            corpus.lhs[sample](row, row) += 3.0;
        }
        corpus.vectors[sample] = Vector{
            bounded_value(state), bounded_value(state), bounded_value(state)};
    }
    return corpus;
}

double sum_elements(const Matrix& matrix) noexcept {
    double sum = 0.0;
    for (std::size_t index = 0; index < 9; ++index) {
        sum += matrix.m[index];
    }
    return sum;
}

bool close_enough(double actual, double expected) noexcept {
    return std::abs(actual - expected) <= 1.0e-12;
}

bool verify_benchmark_oracles() noexcept {
    const Matrix matrix{4.0, 1.0, 0.0, 1.0, 5.0, 1.0, 0.0, 1.0, 6.0};
    const Matrix diagonal{2.0, 0.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 4.0};
    const auto product = matrix * diagonal;
    const Matrix expected_product{8.0, 3.0, 0.0, 2.0, 15.0, 4.0, 0.0, 3.0, 24.0};
    if (!product.AlmostEqual(expected_product, 1.0e-12, 1.0e-12)) {
        return false;
    }
    const auto vector_result = matrix * Vector{1.0, -2.0, 3.0};
    if (!close_enough(vector_result.x, 2.0) || !close_enough(vector_result.y, -6.0) ||
        !close_enough(vector_result.z, 16.0)) {
        return false;
    }
    if (!close_enough(matrix.det(), 110.0)) {
        return false;
    }
    Matrix inverse{};
    if (!matrix.TryInverse(inverse) || !(matrix * inverse).AlmostEqual(Matrix::Identity(), 1.0e-12, 1.0e-12)) {
        return false;
    }
    return close_enough(matrix(2, 1), 1.0) && close_enough(matrix.m[7], 1.0);
}

template <typename Operation>
void measure(std::string_view name, const Corpus& corpus, Operation operation) {
    std::array<double, kSamples> nanoseconds_per_operation{};
    double warmup_checksum = 0.0;
    for (std::uint64_t iteration = 0; iteration < 10'000U; ++iteration) {
        const std::size_t index = static_cast<std::size_t>(iteration % corpus.lhs.size());
        warmup_checksum += operation(corpus, index, iteration);
    }
    benchmark_sink = benchmark_sink + warmup_checksum;
    for (int sample = 0; sample < kSamples; ++sample) {
        double checksum = 0.0;
        const auto start = std::chrono::steady_clock::now();
        for (std::uint64_t iteration = 0; iteration < kIterations; ++iteration) {
            const std::size_t index = static_cast<std::size_t>(iteration % corpus.lhs.size());
            checksum += operation(corpus, index, iteration);
        }
        const auto stop = std::chrono::steady_clock::now();
        benchmark_sink = benchmark_sink + checksum;
        const auto elapsed = std::chrono::duration<double, std::nano>(stop - start).count();
        nanoseconds_per_operation[static_cast<std::size_t>(sample)] =
            elapsed / static_cast<double>(kIterations);
    }
    std::sort(nanoseconds_per_operation.begin(), nanoseconds_per_operation.end());
    std::cout << name << ",double," << kIterations << ',' << kSamples << ','
              << std::fixed << std::setprecision(3)
              << nanoseconds_per_operation[static_cast<std::size_t>(kSamples / 2)] << '\n';
}
} // namespace

int main() {
    if (!verify_benchmark_oracles()) {
        std::cerr << "Matrix3 benchmark correctness oracle failed\n";
        return 1;
    }
    const Corpus corpus = make_corpus();
    std::cout << "operation,type,iterations_per_sample,samples,median_ns_per_operation\n";
    measure("matrix_matrix", corpus, [](const Corpus& data, std::size_t i, std::uint64_t) {
        return sum_elements(data.lhs[i] * data.rhs[(i + 1U) % data.rhs.size()]);
    });
    measure("matrix_vector", corpus, [](const Corpus& data, std::size_t i, std::uint64_t) {
        const auto result = data.lhs[i] * data.vectors[(i + 1U) % data.vectors.size()];
        return result.x + result.y + result.z;
    });
    measure("determinant", corpus, [](const Corpus& data, std::size_t i, std::uint64_t) {
        return data.lhs[i].det();
    });
    measure("try_inverse", corpus, [](const Corpus& data, std::size_t i, std::uint64_t) {
        Matrix inverse{};
        if (!data.lhs[i].TryInverse(inverse)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return sum_elements(inverse);
    });
    measure("element_access", corpus, [](const Corpus& data, std::size_t i, std::uint64_t iteration) {
        const std::size_t row = static_cast<std::size_t>(iteration % 3U);
        const std::size_t column = static_cast<std::size_t>((iteration / 3U) % 3U);
        return data.lhs[i](row, column);
    });
    if (!std::isfinite(benchmark_sink)) {
        std::cerr << "Matrix3 benchmark checksum is non-finite\n";
        return 2;
    }
    std::cerr << "checksum=" << benchmark_sink << " seed=" << kSeed << '\n';
    return 0;
}
