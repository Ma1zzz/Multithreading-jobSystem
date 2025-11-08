#define DEBUGMODE false
#include "jobs.h"
#include "jobList.h"
#include "thread"
#include <algorithm>
#include <atomic>
#include <iostream>
#ifdef __linux__
#include <linux/futex.h>
#elif __APPLE__
#include <dispatch/dispatch.h>
static dispatch_semaphore_t *sems;
#endif

#include <thread>
#include <vector>

namespace jobSystem {

static bool shouldShutDown = false;

static std::vector<std::thread> threads;

static std::atomic<int> usableThreads;
static int threadToGetJob;

static std::atomic<int> futexVal = 0;

static std::atomic<int> threadsStarted;

static std::atomic<int> jobsDone;

static listOfDynamicListsInt *jobListsPtr;

static bool *isWorking;

static std::atomic<int> **jobNumbersPtr;

void reqJob(void (*func)()) {

  if (threadToGetJob == usableThreads) {
    threadToGetJob = 0;
  }
  jobListsPtr->add(threadToGetJob, {func});

  // futexVal++;
#ifdef __linux__

  syscall(SYS_futex, &futexVal, FUTEX_WAIT, 1);
#elif __APPLE__
  // std::cout << "WOke A Thread : " << std::endl;
  if (isWorking[threadToGetJob] == false) {

    // std::cout << "WOke A Thread : " << std::endl;
    if (sems[threadToGetJob] == nullptr) {
      std::cout << "SOmething wrent wront " << std::endl;
    }
    isWorking[threadToGetJob] = true;
    dispatch_semaphore_signal(sems[threadToGetJob]);
  }
#endif

  // std::cout << "DØØØØ" << std::endl;

  threadToGetJob++;
  //++jobCount;
}

static bool steelWork(int id, bool isMain = false) {

  for (int i = 0; i < usableThreads; ++i) {
    if (id == i && !isMain)
      continue;
    if (threadsStarted != usableThreads)
      continue;

    std::atomic<int> *ptr = jobNumbersPtr[i];

    // std::cout << ptr << std::endl;
    if (!jobListsPtr->isAtEnd(i, ptr)) {

      jobData jobdata = jobListsPtr->getJob(i, ptr);
      // auto job = jobdata.func;

      if (jobdata.func == nullptr) {
        // std::cout << *ptr << std::endl;
        // std::cout << "oh no " << std::endl;
        return false;
      }
      // std::cout << "did things" << std::endl;
      jobdata.func();
      return true;
    }
  }

  return false;
}

static void createWorkerThread(int id) {

  std::atomic<int> jobNumber = 0;

  jobNumbersPtr[id] = &jobNumber;

  threadsStarted.fetch_add(1);

  while (!shouldShutDown) {

  // std::cout << "jobCount : " << jobCount << std::endl;
  // idleThreads++;
  back:
    if (shouldShutDown) {
      // std::cout << "THREAD IS GOING BYE BYE" << std::endl;
      break;
    }

    if (jobListsPtr->isAtEnd(id, &jobNumber)) {
      ////std::cout << "OOOOOOOOOOOOO" << std::endl;
#ifdef __linux__
      syscall(SYS_futex, &futexVal, FUTEX_WAIT, 0);
#elif __APPLE__

      //  idleThreads.fetch_add(1);
      if (steelWork(id)) {
        goto back;
      }

      isWorking[id] = false;
      dispatch_semaphore_wait(sems[id], DISPATCH_TIME_FOREVER);
      // idleThreads.fetch_sub(1);
#endif
      goto back;
    }

    jobData jobdata = jobListsPtr->getJob(id, &jobNumber);
    // auto job = jobdata.func;

    if (jobdata.func != nullptr) {
      jobdata.func();
    }
    // jobNumber++;

    // std::cout << jobNumber << std::endl;

#if DEBUGMODE
    // jobsDone.fetch_add(1);
#endif
    // std::cout << "amount of jobs : " << jobCount << std::endl;
    // std::cout << "job is done JOB NumBer : " << jobsDone.load() << std::endl;
  }
  // std::cout << "THREAD WANNA DIEEE" << std::endl;
}

void init(int totalThreads) {
  // usableThreads = std::thread::hardware_concurrency();
  // usableThreads = 3; // for testing
  usableThreads = totalThreads - 1; // -1 for the main thread
  if (usableThreads == 1) {
    throw std::runtime_error("Not enough threads for multithreading.");
  }

#if DEBUGMODE
  std::cout << " Threads program will use : " << usableThreads << std::endl;
#endif

  threads.resize(usableThreads);
  // freeThreads.resize(usableThreads, true);
  shouldShutDown = false;

  listOfDynamicListsInt *dl = new listOfDynamicListsInt(usableThreads);
  jobListsPtr = dl;

  // jobListsPtr = new listOfDynamicListsInt(usableThreads);

  isWorking = new bool[usableThreads];

#ifdef __APPLE__
  sems = new dispatch_semaphore_t[usableThreads];
  for (int i = 0; i < usableThreads; i++) {
    sems[i] = dispatch_semaphore_create(0);
  }
#endif

  jobNumbersPtr = new std::atomic<int> *[usableThreads];
  for (int i = 0; i < threads.size(); i++) {

    threads[i] = std::thread(createWorkerThread, i);

    std::cout << "thread : " << " started" << std::endl;
  }
}

void waitAllJobs() {

#if DEBUGMODE
  std::cout << "SATRTED WAIT " << std::endl;
#endif
retry:

  for (int x = 0; x < 100; x++) {
    std::this_thread::yield();
  }

  for (int z = 0; z < usableThreads; z++) {
    if (isWorking[z]) {
      steelWork(99, true); // id does not matter here
      goto retry;
    }
  }

#if DEBUGMODE
  std::cout << "DONE JOBS : " << jobsDone << std::endl;
#endif
}

void shutdown() {

  shouldShutDown = true;

  for (int y = 0; y < usableThreads; y++) {

    dispatch_semaphore_signal(sems[y]);
  }
#ifdef __linux__
  syscall(SYS_futex, &futexVal, FUTEX_WAKE, INT_MAX);
#elif __APPLE__
  // std::cout << "Shuting down" << std::endl;
#endif

  for (int i = 0; i < threads.size(); i++) {
    if (threads[i].joinable()) {
      threads[i].join();
    }
  }
  delete jobListsPtr;

  delete[] isWorking;
  delete[] jobNumbersPtr;

#ifdef __APPLE__
  delete[] sems;
#endif
}

void clear() {
  for (int x = 0; x < usableThreads; x++) {
    *jobNumbersPtr[x] = 0;
  }
  jobListsPtr->clear();
}
} // namespace jobSystem
