#include <cstdint>
#include <cstring>
#include <functional>
#include <string_view>
#include <unordered_map>

#include "common/opengl.hpp"
#include "gfx/opengl.hpp"
#include "utils/hash.hpp"

#define CONCAT2_(a, b) a##b
#define CONCAT2(a, b)  CONCAT2_(a, b)

#define PROFILE_FUNCTION() \
    auto CONCAT2(profile_function_, __COUNTER__) = Profiler(__PRETTY_FUNCTION__, "Function Call")
#define PROFILE_SCOPE(name) \
    auto CONCAT2(profile_scope_, __COUNTER__) = Profiler(__PRETTY_FUNCTION__, name)
#define PROFILE_BLOCK(name) Profiler(__PRETTY_FUNCTION__, name)->*[&]

template<>
struct std::hash<const char*> {
    size_t operator()(const char* x) const noexcept
    {
        constexpr size_t FNV_prime = 0x00000100000001B3;
        constexpr size_t FNV_basis = 0xcbf29ce484222325;

        size_t hash = FNV_basis;
        for (size_t ii = 0; x[ii] != '\0'; ii++) {
            hash *= FNV_prime;
            hash ^= x[ii];
        }

        return hash;
    }
};

struct ProfilerScope {
    const char* function;
    const char* tag;

    ProfilerScope() = delete;
    ProfilerScope(const char* function, const char* tag) noexcept;

    bool operator==(const ProfilerScope& rhs) const;
};

HASH_IMPL(ProfilerScope, (x.function, x.tag));

struct ProfilerMeasurement {
    // TODO: should eventually convert these to the same units
    // gpu_time is ns, cpu_time is cycles (?)
    u64 cpu_time;
    u64 gpu_time;

    void operator+=(const ProfilerMeasurement& rhs);
};

struct ProfilerQueryable {
    ProfilerScope scope;

    u64 cpu_start;
    u64 cpu_end;

    Query gpu_start;
    Query gpu_end;

    ProfilerQueryable(const char* function, const char* tag) noexcept;
    ~ProfilerQueryable() noexcept;

    ProfilerQueryable(const ProfilerQueryable&) = delete;
    ProfilerQueryable(ProfilerQueryable&& other) noexcept;

    void                Begin();
    void                End();
    ProfilerMeasurement Get() const;
};

struct Profiler {
    static constexpr usize INVALID_QUERY = ~(usize)0;

    usize query;

    Profiler(const char* function, const char* tag);
    ~Profiler();

    template<class F>
    void operator->*(F invoke)
    {
        invoke();
    }
};

extern std::unordered_map<ProfilerScope, ProfilerMeasurement> ProfilingMeasurements;
extern std::vector<ProfilerQueryable>                         ProfilingQueryables;
extern bool                                                   ProfilingEnabled;

void ResetProfilingData(void);
void CollectProfilingData(void);
void SetProfilingEnabled(bool enabled);
