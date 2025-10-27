#pragma once

void reqJobs(void (*func)());

void initJobsSystem();

void doJobs();

void waitAllJobs();

void shutdownJobsSystem();
