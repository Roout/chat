#ifndef LOCAL_UTILITY_HPP
#define LOCAL_UTILITY_HPP

#include <cstddef> // std::size_t
#include <chrono>

namespace Utils {
    /**
     * Work with enumerations: 
     */
    template<class Enum>
    constexpr std::size_t EnumCast(Enum value) noexcept {
        return static_cast<std::size_t>(value);
    }

    template<class Enum>
    constexpr std::size_t EnumSize() noexcept {
        return EnumCast(Enum::COUNT);
    } 

    template<class Enum>
    constexpr Enum EnumCast(std::size_t value) noexcept {
        return static_cast<Enum>(value);
    }

    template<class ...Args>
    constexpr std::size_t CreateMask(Args&& ... args) noexcept {
        return (static_cast<std::size_t>(args) | ...);
    }

    /**
     * Generate timestamp. It's time elapsed since epoch in milliseconds 
     */
    inline long long GetTimestamp() noexcept {
        const auto now = std::chrono::high_resolution_clock::now();
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        );
        return ms.count();
    }
}

#endif // LOCAL_UTILITY_HPP