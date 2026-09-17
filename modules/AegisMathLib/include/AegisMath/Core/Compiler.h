#pragma once

// 跨平台识别 C++20 标准 (兼容 MSVC 的特殊宏定义机制)
#if defined(_MSVC_LANG)
    #define AEGIS_CPLUSPLUS _MSVC_LANG
#else
    #define AEGIS_CPLUSPLUS __cplusplus
#endif

// 强制断言：AegisMathLib 仅支持 C++20 及以上版本
static_assert(AEGIS_CPLUSPLUS >= 202002L,
    "[AegisMath] FATAL: Compiler does not support C++20 standard.");