#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

// 1. 编译隔离性检查：包含 Core 与 Geometry 核心头文件，验证无 Dynamics 依赖即可通过编译
#include "AegisMath/Core/Constants.h"
#include "AegisMath/Core/NumericTraits.h"
#include "AegisMath/Geometry/FrameTags.h"
#include "AegisMath/Geometry/Vector3.h"
#include "AegisMath/Geometry/Point3.h"
#include "AegisMath/Geometry/Matrix3.h"
#include "AegisMath/Geometry/Quaternion.h"
#include "AegisMath/Geometry/RotationMatrix3.h"

namespace fs = std::filesystem;

TEST(ArchitectureLayeringTest, LowerLayersMustNotIncludeDynamics) {
    // 根目录或构建上下文中的 include 目录
    const std::vector<fs::path> search_paths = {
        fs::path(__FILE__).parent_path().parent_path().parent_path() / "include" / "AegisMath" / "Core",
        fs::path(__FILE__).parent_path().parent_path().parent_path() / "include" / "AegisMath" / "Units",
        fs::path(__FILE__).parent_path().parent_path().parent_path() / "include" / "AegisMath" / "Geometry"
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
