#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

// 1. 编译隔离性检查：包含 Core 与 Geometry 核心头文件，验证无 Dynamics 依赖即可通过编译
#include "Vectoris/Numerics/Core/Constants.h"
#include "Vectoris/Numerics/Core/NumericTraits.h"
#include "Vectoris/Numerics/Geometry/FrameTags.h"
#include "Vectoris/Numerics/Geometry/Vector3.h"
#include "Vectoris/Numerics/Geometry/Point3.h"
#include "Vectoris/Numerics/Geometry/Matrix3.h"
#include "Vectoris/Numerics/Geometry/Quaternion.h"
#include "Vectoris/Numerics/Geometry/RotationMatrix3.h"

namespace fs = std::filesystem;

// Project root is injected by CMake via -DVECTORIS_PROJECT_ROOT="..."
#ifndef VECTORIS_PROJECT_ROOT
#error "VECTORIS_PROJECT_ROOT must be defined by CMake"
#endif

static fs::path numerics_include_root() {
    return fs::path(VECTORIS_PROJECT_ROOT) / "modules" / "VectorisNumerics" / "include" / "Vectoris" / "Numerics";
}

TEST(ArchitectureLayeringTest, LowerLayersMustNotIncludeDynamics) {
    const std::vector<fs::path> search_paths = {
        numerics_include_root() / "Core",
        numerics_include_root() / "Units",
        numerics_include_root() / "Geometry"
    };

    const std::regex illegal_include_pattern(R"(#\s*include\s*["<].*Dynamics.*[">])");
    std::vector<std::string> violations;

    for (const auto& dir : search_paths) {
        ASSERT_TRUE(fs::exists(dir)) << "Directory does not exist: " << dir;
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file() && (entry.path().extension() == ".h" || entry.path().extension() == ".hpp")) {
                std::ifstream file(entry.path());
                ASSERT_TRUE(file.is_open()) << "Failed to open header: " << entry.path();

                std::string line;
                int line_num = 0;
                while (std::getline(file, line)) {
                    ++line_num;
                    if (std::regex_search(line, illegal_include_pattern)) {
                        violations.push_back(entry.path().filename().string() + ":" + std::to_string(line_num) + ": " + line);
                    }
                }
            }
        }
    }

    EXPECT_TRUE(violations.empty()) << "Architecture violation: Lower layers include Dynamics headers:\n"
                                     << [&]() {
                                         std::string msg;
                                         for (const auto& v : violations) {
                                             msg += "  " + v + "\n";
                                         }
                                         return msg;
                                     }();
}

TEST(ArchitectureLayeringTest, NoLegacyDuplicateUnitsSystem) {
    const fs::path include_root = numerics_include_root();
    const fs::path legacy_unit_h = include_root / "Units" / "Unit.h";

    // 1. Ensure the legacy Unit.h file is completely removed
    EXPECT_FALSE(fs::exists(legacy_unit_h)) << "Legacy duplicate header Unit.h must not exist: " << legacy_unit_h;

    // 2. Ensure no header declares a duplicate Quantity directly in namespace vectoris::numerics
    const fs::path units_dir = include_root / "Units";
    ASSERT_TRUE(fs::exists(units_dir));

    const std::regex illegal_root_quantity_pattern(R"(namespace\s+vectoris::numerics\s*\{\s*template.*class\s+Quantity)");
    std::vector<std::string> violations;

    for (const auto& entry : fs::recursive_directory_iterator(units_dir)) {
        if (entry.is_regular_file() && (entry.path().extension() == ".h" || entry.path().extension() == ".hpp")) {
            std::ifstream file(entry.path());
            ASSERT_TRUE(file.is_open());

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (std::regex_search(content, illegal_root_quantity_pattern)) {
                violations.push_back(entry.path().filename().string());
            }
        }
    }

    EXPECT_TRUE(violations.empty()) << "Architecture violation: Found duplicate Quantity in namespace vectoris::numerics in Units headers";
}

TEST(ArchitectureLayeringTest, VectorisNumericsMustNotContainOrIncludeDynamics) {
    const fs::path numerics_root = numerics_include_root();
    const fs::path legacy_dynamics_dir = numerics_root / "Dynamics";

    // 1. Ensure the legacy Dynamics folder is completely absent from include/Vectoris/Numerics/
    EXPECT_FALSE(fs::exists(legacy_dynamics_dir))
        << "Domain Dynamics folder must not exist inside pure VectorisNumerics: " << legacy_dynamics_dir;

    // 2. Scan all VectorisNumerics headers to ensure zero mentions of VectorisDynamics or Dynamics includes
    const std::regex illegal_dynamics_pattern(R"(#\s*include\s*["<].*(Dynamics|VectorisDynamics|AegisDynamics).*[">])");
    std::vector<std::string> violations;

    for (const auto& entry : fs::recursive_directory_iterator(numerics_root)) {
        if (entry.is_regular_file() && (entry.path().extension() == ".h" || entry.path().extension() == ".hpp")) {
            std::ifstream file(entry.path());
            ASSERT_TRUE(file.is_open()) << "Failed to open header: " << entry.path();

            std::string line;
            int line_num = 0;
            while (std::getline(file, line)) {
                ++line_num;
                if (std::regex_search(line, illegal_dynamics_pattern)) {
                    violations.push_back(entry.path().filename().string() + ":" + std::to_string(line_num) + ": " + line);
                }
            }
        }
    }

    EXPECT_TRUE(violations.empty()) << "Architecture violation: VectorisNumerics headers include Dynamics:\n"
                                     << [&]() {
                                         std::string msg;
                                         for (const auto& v : violations) {
                                             msg += "  " + v + "\n";
                                         }
                                         return msg;
                                     }();
}
