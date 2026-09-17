#pragma once

// 跨平台识别 C++20 标准 (兼容 MSVC 的特殊宏定义机制)
#if defined(_MSVC_LANG)
    #define VECTORIS_CPLUSPLUS _MSVC_LANG
#else
    #define VECTORIS_CPLUSPLUS __cplusplus
#endif

// 强制断言：Vectoris 仅支持 C++20 及以上版本
static_assert(VECTORIS_CPLUSPLUS >= 202002L,
    "[VectorisNumerics] FATAL: Compiler does not support C++20 standard.");