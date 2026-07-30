#pragma once
#include <new>

namespace AegisMath::Core {

    // 专为无异常/无堆分配环境设计的 Result 范式
    template <typename T>
    struct Result final {
    private:
        bool success_;
        alignas(T) unsigned char storage_[sizeof(T)];

    public:
        // 失败状态构造
        constexpr Result() noexcept : success_(false) {}

        // 成功状态构造
        template <typename... Args>
        constexpr explicit Result(Args&&... args) noexcept : success_(true) {
            new (storage_) T(static_cast<Args&&>(args)...);
        }

        constexpr bool IsSuccess() const noexcept { return success_; }
        
        // 提取数据 (调用方需确保 IsSuccess() 为 true)
        constexpr const T& Value() const noexcept {
            return *reinterpret_cast<const T*>(storage_);
        }
    };

} // namespace AegisMath::Core