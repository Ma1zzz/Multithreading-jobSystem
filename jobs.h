#pragma once
#include <functional>

void reqJobs(void (*func)());

void parallelLoop(int start, int end, std::function<void(int)> code,
                  int jobsToCreate = 4, bool wait = true);

void initJobsSystem();

void doJobs();

void waitAllJobs();

void shutdownJobsSystem();
