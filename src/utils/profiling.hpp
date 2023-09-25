#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>

#include "common/opengl.hpp"
#include "utils/hash.hpp"

#define CONCAT2_(a, b) a##b
#define CONCAT2(a, b)  CONCAT2_(a, b)

#define PROFILE_FUNCTION() \
    auto CONCAT2(profile_function_, __COUNTER__) = Profiler(__PRETTY_FUNCTION__, "Function Call")
#define PROFILE_SCOPE(name) \
    auto CONCAT2(profile_scope_, __COUNTER__) = Profiler(__PRETTY_FUNCTION__, name)
#define PROFILE_BLOCK(name) Profiler(__PRETTY_FUNCTION__, name, true)->*[&](void)

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

    bool operator==(const ProfilerScope& rhs) const
    {
        return (this->function == rhs.function && this->tag == rhs.tag);
    }
};

HASH_IMPL(ProfilerScope, (x.function, x.tag));

struct ProfilerMeasurement {
    uint64_t cpu_time;
    uint64_t gpu_time;

    void operator+=(const ProfilerMeasurement& rhs)
    {
        this->cpu_time += rhs.cpu_time;
        this->gpu_time += rhs.gpu_time;
    }
};

struct ProfilerQueryable {
    ProfilerScope scope;

    uint64_t cpu_start;
    uint64_t cpu_end;

    GLuint gpu_start_q;
    GLuint gpu_end_q;

    ProfilerQueryable();
    ProfilerQueryable(const char* function, const char* tag);

    ProfilerMeasurement Measure(void);
};

struct Profiler {
    ProfilerQueryable query;
    bool              ignore_exit;

    Profiler(const char* function, const char* tag, bool ignore_exit = false);
    Profiler(const Profiler&) = delete;
    Profiler(const Profiler&&);
    ~Profiler();

    void Begin(void);
    void End(void);

    void operator->*(std::function<void(void)> invoke);
};

extern std::unordered_map<ProfilerScope, ProfilerMeasurement> ProfilingMeasurements;
extern std::vector<ProfilerQueryable>                         ProfilingQueryables;

void ResetProfilingData(void);
void CollectProfilingData(void);
