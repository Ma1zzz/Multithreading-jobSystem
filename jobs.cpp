#define DEBUGMODE true
#include "jobs.h"
#include "jobList.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <limits.h>
#ifdef __linux__
#include <linux/futex.h>
#elif __APPLE__
#include <dispatch/dispatch.h>
dispatch_semaphore_t *sems; //= dispatch_semaphore_create(0);
#endif
#include <mutex>
#include <queue>
#include <sys/syscall.h>
#include <thread>
#include <vector>

/*struct jobData {
  void (*func)();
};*/

static std::condition_variable threadConditio;
static std::condition_variable isWorkDone;

static bool shouldShutDown = false;

static std::mutex jobsQueueMutex;
static std::mutex mutex;

static std::queue<std::function<void()>> loopJobs;

static std::vector<std::vector<jobData>> jobThreadQueueVector;
static std::vector<std::mutex> mutexList;

// static std::atomic<int> jobCount = 0;
static int jobCount = 0;

static std::atomic<int> loopJobsCount = 0;
static std::atomic<int> loopJobsLeftToComplete = 0;

static std::vector<std::thread> threads;

static std::atomic<int> idleThreads;
// static int idleThreads;
static std::atomic<int> usableThreads;
// static int usableThreads;
//
//
// std::vector<std::vector<int>> jobNumberLists(4);

static myVector<jobData> jobDataList;

static int threadToGetJob;

static std::atomic<int> futexVal = 0;

// static void *listyThingy;

static std::atomic<int> jobsDone;

static listOfDynamicListsInt *listPtr;

static bool isWorking[4];

void reqJobs(void (*func)()) {

  jobDataList.add({func});

  if (threadToGetJob == usableThreads) {
    threadToGetJob = 0;
  }

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

  // jobNumberLists[threadToGetJob].push_back(jobCount);

  listPtr->add(threadToGetJob, &jobCount);

  // std::cout << "DØØØØ" << std::endl;
  threadToGetJob++;

  ++jobCount;
}

// static std::atomic<int> jobNumber = 0;

static void createWorkerThread(int id) {

  int jobNumber = 0;

  while (!shouldShutDown) {

    // std::cout << "jobCount : " << jobCount << std::endl;
    // idleThreads++;
  back:
    if (shouldShutDown) {
      // std::cout << "THREAD IS GOING BYE BYE" << std::endl;
      break;
    }

    if (listPtr->isAtEnd(id, jobNumber)) {
      ////std::cout << "OOOOOOOOOOOOO" << std::endl;
#ifdef __linux__
      syscall(SYS_futex, &futexVal, FUTEX_WAIT, 0);
#elif __APPLE__

      // idleThreads.fetch_add(1);
      isWorking[id] = false;
      dispatch_semaphore_wait(sems[id], DISPATCH_TIME_FOREVER);
      // idleThreads.fetch_sub(1);
#endif
      goto back;
    }

    // dispatch_semaphore_wait(sems[id], DISPATCH_TIME_FOREVER);
    //  std::cout << "job number : " << jobNumber << std::endl;

    //--jobCount;
    // std::cout << getJobQueue().size() << std::endl;

    // idleThreads--;
  bb:

    // void* ptr = jobDataList.getJob(listPtr->getJobIndex(id, jobNumber));
    // void *ptr = jobDataList.getVal(listPtr->getJobIndex(id, jobNumber));
    // jobData jobdata =
    //  *(jobData *)jobDataList.getJob(listPtr->getJobIndex(id, jobNumber));
    // jobData jobdata = *(jobData *)ptr;
    jobData jobdata = jobDataList.getJob(listPtr->getJobIndex(id, jobNumber));
    // std::cout << ptr << std::endl;
    auto job = jobdata.func;

    if (job == nullptr) {
      std::cerr << "job == null not god" << std::endl;
      goto bb;
    }
    job();

    jobNumber++;
#if DEBUGMODE
    // jobsDone.fetch_add(1);
#endif
    // std::cout << "amount of jobs : " << jobCount << std::endl;
    // std::cout << "job is done JOB NumBer : " << jobsDone.load() << std::endl;
  }
  // std::cout << "THREAD WANNA DIEEE" << std::endl;
}

void initJobsSystem() {
  // usableThreads = std::thread::hardware_concurrency();
  // jobDataList.resize(50000);
  usableThreads = 6; // for testing
  std::cout << usableThreads << std::endl;
  if (usableThreads == 1) {
    throw std::runtime_error("Not enough threads for multithreading.");
  }
#if DEBUGMODE
  std::cout << " total threads: " << usableThreads << std::endl;
#endif

  usableThreads -= 1;

  if (usableThreads > 4)
    usableThreads -= 1;

#if DEBUGMODE
  std::cout << " Threads program will use : " << usableThreads << std::endl;
#endif

  threads.resize(usableThreads);
  // freeThreads.resize(usableThreads, true);
  shouldShutDown = false;

  listOfDynamicListsInt *dl = new listOfDynamicListsInt(usableThreads);
  listPtr = dl;

  // dispatch_semaphore_t sem = dispatch_semaphore_create(0);

  sems = new dispatch_semaphore_t[usableThreads];
  for (int i = 0; i < usableThreads; i++) {
    sems[i] = dispatch_semaphore_create(0);
  }

  // listOfDynamicListsInt give_me_a_name(usableThreads);
  // listyThingy = give_me_a_name;
  /* for (int i = 0; i < threads.size(); i++) {
     threads[i] = std::thread(createWorkerThread, i);

     std::vector<jobData> data;
     jobThreadQueueVector.push_back(data);

     std::cout << "thread : " << " started" << std::endl;
   }*/
}

static void submitloopJob(std::function<void()> func) {
  std::unique_lock<std::mutex> lock(jobsQueueMutex);
  loopJobs.push(func);
  lock.unlock();
  // threadCondition.notify_one();
}

/*void parallelLoop(int stat, int end, std::function<void(int)> code, // old
logic needs to be fixed at some point int jobsToCreate, bool wait) { int count =
end - stat; int remainder = count % jobsToCreate; count -= remainder; int
loopChunk = (count / (jobsToCreate)); loopJobsCount += jobsToCreate;
  // loopJobsLeftToComplete += jobsToCreate;

  for (int job = 0; job < jobsToCreate; job++) {
    int countStart = loopChunk * job + stat;
    int countEnd = loopChunk + countStart;

    if (job == jobsToCreate - 1)
      countEnd += remainder;

    submitloopJob([code, countStart, countEnd]() {
      for (int i = countStart; i < countEnd; i++) {
        code(i);
      }
    });
  }

  if (wait) {
    waitForLoopJobs();
  }
}*/

void doJobs() {

  for (int i = 0; i < threads.size(); i++) {
    threads[i] = std::thread(createWorkerThread, i);

    std::vector<jobData> data;
    jobThreadQueueVector.push_back(data);

    std::cout << "thread : " << " started" << std::endl;
  }
  // probaly gonna remove this
}

void waitAllJobs() {

  std::cout << "SATRTED WAIT " << std::endl;
bb:

  for (int x = 0; x < 100; x++) {
    std::this_thread::yield();
  }

  for (int z = 0; z < usableThreads; z++) {
    if (isWorking[z]) {
      goto bb;
    }
  }

  /*int threadsLeft = usableThreads - idleThreads;
  if (threadsLeft == 0) {
    std::cout << "threads are done" << std::endl;
    // return;
  } else {
    while (threadsLeft != 0) {
      threadsLeft = usableThreads - idleThreads;
      // std::cout << "watint :" << std::endl;
    }
  }*/
  std::cout << "DONE JOBS : " << jobsDone << std::endl;
}

void shutdownJobsSystem() {

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
}
