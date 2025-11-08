#pragma once

namespace jobSystem {
void reqJob(void (*func)());

void init(int totalThreads);

void waitAllJobs();

void shutdown();

void clear();
} // namespace jobSystem
