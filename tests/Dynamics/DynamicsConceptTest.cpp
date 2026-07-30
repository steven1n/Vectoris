#include <gtest/gtest.h>
#include <string>
#include "AegisMath/Dynamics/Concepts.h"

using namespace AegisMath::Dynamics;

TEST(DynamicsConceptTest, ScalarConstraints) {
    static_assert(DynamicsScalar<float>, "Float must satisfy DynamicsScalar.");
    static_assert(DynamicsScalar<double>, "Double must satisfy DynamicsScalar.");
    static_assert(!DynamicsScalar<std::string>, "std::string must be rejected.");
    static_assert(!DynamicsScalar<int*>, "Pointer types must be rejected.");
    SUCCEED();
}