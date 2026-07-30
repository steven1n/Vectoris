import os
from pathlib import Path

def fix_rotation_invariant():
    """修复 Detail/RotationInvariant.h 中的旧 Traits 路径"""
    filepath = Path("include/AegisMath/Geometry/Detail/RotationInvariant.h")

    if not filepath.exists():
        print(f"⚠️ 找不到文件: {filepath}")
        return False

    content = filepath.read_text(encoding="utf-8")
    # Geometry/Detail 到 Core 的相对路径是 ../../Core/
    target_str = '#include "../../Traits/NumericTraits.h"'
    replacement_str = '#include "../../Core/NumericTraits.h"'

    if target_str in content:
        content = content.replace(target_str, replacement_str)
        filepath.write_text(content, encoding="utf-8")
        print(f"✅ 成功修复: {filepath} (修正为 ../../Core/NumericTraits.h)")
        return True
    else:
        print(f"ℹ️ 无需修改: {filepath} 中未找到旧路径。")
        return False

def fix_units_test():
    """修复 UnitsTest.cpp 缺少 Kilometer 定义的问题"""
    filepath = Path("tests/Units/UnitsTest.cpp")

    if not filepath.exists():
        print(f"⚠️ 找不到文件: {filepath}")
        return False

    with open(filepath, "r", encoding="utf-8") as f:
        lines = f.readlines()

    # 检查是否已经存在
    has_include = any("Length.h" in line for line in lines)
    has_using = any("using namespace AegisMath::Units;" in line for line in lines)

    if not has_include or not has_using:
        # 寻找最后一个 #include 的位置
        last_include_idx = -1
        for i, line in enumerate(lines):
            if line.startswith("#include"):
                last_include_idx = i

        insert_idx = last_include_idx + 1 if last_include_idx != -1 else 0

        if not has_using:
            lines.insert(insert_idx, 'using namespace AegisMath::Units;\n')
        if not has_include:
            lines.insert(insert_idx, '#include "AegisMath/Units/Length.h"\n')

        with open(filepath, "w", encoding="utf-8") as f:
            f.writelines(lines)

        print(f"✅ 成功修复: {filepath} (强制补充了 Length.h 和 namespace)")
        return True
    else:
        print(f"ℹ️ 无需修改: {filepath} 已包含 Units 相关头文件。")
        return False

if __name__ == "__main__":
    print("🚀 开始终极一键修复 AegisMathLib 编译错误...\n")

    if not Path("include/AegisMath").exists():
        print("❌ 错误：请在项目根目录 (AegisMathLib) 下运行此脚本！")
    else:
        fix_rotation_invariant()
        fix_units_test()

    print("\n🎉 修复执行完毕！请重新运行构建命令：")
    print("   cmake --build /Users/akiyama/CLionProjects/AegisMathLib/cmake-build-debug --target AegisMathLib_Tests -j 14")