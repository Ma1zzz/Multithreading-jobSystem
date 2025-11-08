#pragma once
#include <atomic>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

struct jobData {
  void (*func)();
};

class listOfDynamicListsInt {
private:
    std::atomic<void *> *ptrToPtr;

    //func *dataType;
    size_t *capacity;
    size_t *currentSize;
    std::atomic<int> *numberOfElements;

    size_t pageSize;

    std::vector<size_t> ptrSizes;
    std::vector<void *> oldPtrs;

    int numOfLists;

    std::mutex *mutexs;
public:
  listOfDynamicListsInt(int num);

  ~listOfDynamicListsInt();

  void resize(int index, int pagesToAdd);

  void add(int index, jobData input);

  bool isAtEnd(int threadIndex, std::atomic<int> *number);

  jobData getJob(int index, std::atomic<int> *number);

  void clear();
};
