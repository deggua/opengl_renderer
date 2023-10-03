#include "profiling.hpp"

#include <unordered_map>

std::unordered_map<ProfilerScope, ProfilerMeasurement> ProfilingMeasurements;
std::vector<ProfilerQueryable>                         ProfilingQueryables;
bool                                                   ProfilingEnabled;

/* ProfilerScope */
ProfilerScope::ProfilerScope(const char* function, const char* tag) noexcept
{
    this->function = function;
    this->tag      = tag;
}

bool ProfilerScope::operator==(const ProfilerScope& rhs) const
{
    return (!strcmp(this->function, rhs.function) && !strcmp(this->tag, rhs.tag));
}

/* ProfilerMeasurement */
void ProfilerMeasurement::operator+=(const ProfilerMeasurement& rhs)
{
    this->cpu_time += rhs.cpu_time;
    this->gpu_time += rhs.gpu_time;
    this->hit_count += 1;
}

/* ProfilerQueryable */
ProfilerQueryable::ProfilerQueryable(const char* function, const char* tag) noexcept :
    scope(function, tag)
{
    this->gpu_start.Reserve();
    this->gpu_end.Reserve();
}

ProfilerQueryable::~ProfilerQueryable() noexcept
{
    this->gpu_start.Delete();
    this->gpu_end.Delete();
}

ProfilerQueryable::ProfilerQueryable(ProfilerQueryable&& other) noexcept :
    scope(other.scope.function, other.scope.tag)
{
    this->cpu_start = other.cpu_start;
    this->cpu_end   = other.cpu_end;

    this->gpu_start = other.gpu_start;
    this->gpu_end   = other.gpu_end;

    other.gpu_start = 0;
    other.gpu_end   = 0;
}

void ProfilerQueryable::Begin()
{
    this->cpu_start = __builtin_readcyclecounter();
    this->gpu_start.RecordTimestamp();
}

void ProfilerQueryable::End()
{
    this->cpu_end = __builtin_readcyclecounter();
    this->gpu_end.RecordTimestamp();
}

ProfilerMeasurement ProfilerQueryable::Get(void) const
{
    u64 cpu_time = this->cpu_end - this->cpu_start;
    u64 gpu_time = this->gpu_end.RetrieveValue() - this->gpu_start.RetrieveValue();

    return {cpu_time, gpu_time};
}

/* Profiler */
Profiler::Profiler(const char* function, const char* tag)
{
    if (!ProfilingEnabled) {
        this->query = INVALID_QUERY;
        return;
    }

    ProfilingQueryables.emplace_back(function, tag);
    this->query = ProfilingQueryables.size() - 1;

    ProfilingQueryables[this->query].Begin();
}

Profiler::~Profiler()
{
    if (this->query == INVALID_QUERY) {
        return;
    }

    ProfilingQueryables[this->query].End();
}

void CollectProfilingData(void)
{
    if (!ProfilingEnabled) {
        return;
    }

    for (const auto& query : ProfilingQueryables) {
        ProfilingMeasurements[query.scope] += query.Get();
    }

    ProfilingQueryables.clear();
}

void ResetProfilingData(void)
{
    if (!ProfilingEnabled) {
        return;
    }

    ProfilingMeasurements.clear();
}

void SetProfilingEnabled(bool enabled)
{
    ProfilingEnabled = enabled;
}
