#pragma once

#include "SoftwareRasterizerApp.h"
#include "JobSystem.h"
#include "Logging.h"

#define HE_JOB_SYSTEM_NUM_FIBIERS 128
#define HE_JOB_SYSTEM_FIBER_STACK_SIZE (HE_JOB_SYSTEM_NUM_FIBIERS * 1024)

uint32_t GetNumberOfProcessors()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
}

int main(int argc, char** argv)
{
	SR::LogSystemInit();
	SR_LOG_INFO("Init log system.");
	uint32_t numProcessors = GetNumberOfProcessors();
	SR::JobSystem::Init(numProcessors, HE_JOB_SYSTEM_NUM_FIBIERS, HE_JOB_SYSTEM_FIBER_STACK_SIZE);
	SR_LOG_INFO("Init job system, num processors {}, num fibiers: {}, fiber stack size: {}.", numProcessors, HE_JOB_SYSTEM_NUM_FIBIERS, HE_JOB_SYSTEM_FIBER_STACK_SIZE);
	int exit = SoftwareRasterizerMain();
	SR::JobSystem::Shutdown();
	SR_LOG_INFO("Exit job system");
	return exit;
}
