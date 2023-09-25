#include "profiling.hpp"

#include <unordered_map>

std::unordered_map<ProfilerScope, ProfilerMeasurement> ProfilingMeasurements;
std::vector<ProfilerQueryable>                         ProfilingQueryables;

ProfilerQueryable::ProfilerQueryable()
{
    this->scope       = {NULL, NULL};
    this->cpu_start   = 0;
    this->cpu_end     = 0;
    this->gpu_start_q = 0;
    this->gpu_end_q   = 0;
}

ProfilerQueryable::ProfilerQueryable(const char* function, const char* tag)
{
    this->scope       = {function, tag};
    this->cpu_start   = 0;
    this->cpu_end     = 0;
    this->gpu_start_q = 0;
    this->gpu_end_q   = 0;
}

ProfilerMeasurement ProfilerQueryable::Measure(void)
{
    uint64_t cpu_time = this->cpu_end - this->cpu_start;

    GLuint64 gpu_end, gpu_start;
    GL(glGetQueryObjectui64v(this->gpu_end_q, GL_QUERY_RESULT, &gpu_end));
    GL(glGetQueryObjectui64v(this->gpu_start_q, GL_QUERY_RESULT, &gpu_start));
    uint64_t gpu_time = gpu_end - gpu_start;

    GL(glDeleteQueries(1, &this->gpu_end_q));
    GL(glDeleteQueries(1, &this->gpu_start_q));

    return {cpu_time, gpu_time};
}

Profiler::Profiler(const char* function, const char* tag, bool ignore_exit)
{
    this->query       = ProfilerQueryable(function, tag);
    this->ignore_exit = ignore_exit;

    GL(glGenQueries(1, &this->query.gpu_start_q));
    GL(glGenQueries(1, &this->query.gpu_end_q));

    this->Begin();
}

Profiler::~Profiler()
{
    if (this->ignore_exit) {
        return;
    }

    this->End();

    ProfilingQueryables.push_back(this->query);
}

void Profiler::Begin(void)
{
    this->query.cpu_start = __builtin_readcyclecounter();
    GL(glQueryCounter(this->query.gpu_start_q, GL_TIMESTAMP));
}

void Profiler::End(void)
{
    this->query.cpu_end = __builtin_readcyclecounter();
    GL(glQueryCounter(this->query.gpu_end_q, GL_TIMESTAMP));
}

void Profiler::operator->*(std::function<void(void)> invoke)
{
    this->ignore_exit = true;

    this->Begin();
    invoke();
    this->End();

    ProfilingQueryables.push_back(this->query);
}

void CollectProfilingData(void)
{
    for (auto& qable : ProfilingQueryables) {
        ProfilingMeasurements[qable.scope] += qable.Measure();
    }

    ProfilingQueryables.clear();
}

void ResetProfilingData(void)
{
    ProfilingMeasurements.clear();
}
