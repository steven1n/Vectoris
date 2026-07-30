#!/usr/bin/env python3
import os

def align_files():
    dynamics_dir = "include/AegisMath/Dynamics"
    if not os.path.exists(dynamics_dir):
        print(f"错误: 找不到目录 {dynamics_dir}")
        return

    # 递归收集 Dynamics 下所有真实存在的 .h 文件名及其相对路径
    file_map = {}
    for root, dirs, files in os.walk(dynamics_dir):
        for file in files:
            if file.endswith(".h"):
                # 例如 StateTypes.h 或 Detail/DynamicsABI.h
                rel_path = os.path.relpath(os.path.join(root, file), dynamics_dir)
                file_map[file] = rel_path.replace(os.sep, "/")

    print("检测到的 Dynamics 头文件映射:")
    for name, path in file_map.items():
        print(f"  {name} -> {path}")

    # 扫描 include 和 tests 目录下的所有源码，修正 #include "AegisMath/Dynamics/..."
    search_dirs = ["include", "tests"]
    for s_dir in search_dirs:
        if not os.path.exists(s_dir):
            continue
        for root, dirs, files in os.walk(s_dir):
            for file in files:
                if file.endswith((".h", ".cpp")):
                    file_path = os.path.join(root, file)
                    with open(file_path, "r", encoding="utf-8") as f:
                        content = f.read()

                    new_content = content
                    # 针对每一个文件名，如果被错误引用了，修正为正确的基于 AegisMath/Dynamics/ 的绝对相对路径
                    for fname, rpath in file_map.items():
                        # 匹配诸如 #include "AegisMath/Dynamics/StateTypes.h" 或 #include "StateTypes.h"
                        wrong_patterns = [
                            f'#include "AegisMath/Dynamics/{fname}"',
                            f'#include "{fname}"',
                        ]
                        correct_include = f'#include "AegisMath/Dynamics/{rpath}"'

                        for wp in wrong_patterns:
                            # 避免重复替换已经正确的
                            if wp != correct_include and wp in new_content:
                                new_content = new_content.replace(wp, correct_include)

                    if new_content != content:
                        with open(file_path, "w", encoding="utf-8") as f:
                            f.write(new_content)
                        print(f"[已对齐] {file_path}")

    print("\n[完成] 头文件包含路径已与实际物理文件结构完全对齐！")

if __name__ == "__main__":
    align_files()
