#include <functional>

template<class T, class... Ts>
size_t HashCombine_FNV1A(size_t state, T first, Ts... pack) noexcept
{
    constexpr size_t FNV_prime = 0x00000100000001B3;
    if constexpr (sizeof...(pack) > 0) {
        return HashCombine_FNV1A((state * FNV_prime) ^ std::hash<T>()(first), pack...);
    } else {
        return (state * FNV_prime) ^ std::hash<T>()(first);
    }
}

template<class... Ts>
size_t HashCombine(Ts... pack) noexcept
{
    constexpr size_t FNV_basis = 0xcbf29ce484222325;
    return HashCombine_FNV1A(FNV_basis, pack...);
}

#define HASH_IMPL(type, x_members)                      \
    template<>                                          \
    struct std::hash<type> {                            \
        size_t operator()(const type& x) const noexcept \
        {                                               \
            return HashCombine x_members;               \
        }                                               \
    }
