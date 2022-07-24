#pragma once

#include "SRCommon.h"

namespace SR
{
#define JOB_SYSTEM_NULL_HANDLE 0
#define JOB_SYSTEM_JOB_ENTRY_POINT(func) ((void(*)(void*))func)

using JobSystemAtomicCounterHandle = uint32;

enum
{
    JOB_SYSTEM_MAX_NUM_WORKER_THREADS = 128,
    JOB_SYSTEM_MAX_NUM_FIBERS = 256,
    JOB_SYSTEM_MAX_NUM_JOBS = 4096,
};

using JobFunc = void(*)(void*);
struct JobDecl
{
    JobFunc jobFunc;
    void* data;
};

namespace JobSystem
{

extern void Init(uint32 numWorkerThreads, uint32 numFibers, uint32 fiberStackSize);
extern void Shutdown();
extern bool IsInitialized();
extern JobSystemAtomicCounterHandle RunJobs(JobDecl* jobDecls, uint32 numJobs);
extern void WaitForCounter(JobSystemAtomicCounterHandle counterHandle, uint32 condition);
extern void WaitForCounterAndFree(JobSystemAtomicCounterHandle counterHandle, uint32 condition);
extern void WaitForCounterAndFreeWithoutFiber(JobSystemAtomicCounterHandle counterHandle);

};

}