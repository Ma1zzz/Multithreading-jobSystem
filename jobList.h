#pragma once
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <dispatch/dispatch.h>
#include <iostream>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>
#include <utility>

struct jobData {
  void (*func)();
};

class listOfDynamicListsInt {
private:
  void **ptrToPtr;

  size_t *capacity;
  size_t *currentSize;
  int *numberOfElements;
  // int *amoutOfData;

  size_t pageSize;

  int numOfLists;
  std::atomic<bool> canReedData;
  bool canAddData;

public:
  listOfDynamicListsInt(int num) {
    numOfLists = num;
    ptrToPtr = new void *[num];
    currentSize = new size_t[num];
    capacity = new size_t[num];
    numberOfElements = new int[num];
    // amoutOfData = (int *)malloc(sizeof(int) * 4);
    pageSize = sysconf(_SC_PAGESIZE);

    for (int x = 0; x < numOfLists; x++) {
      ptrToPtr[x] = mmap(nullptr, pageSize, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }

    for (int x = 0; x < num; x++) {
      numberOfElements[x] = 0;
      capacity[x] = pageSize;
      currentSize[x] = 0;
    }

    canReedData = true;
    /* for (int y = 0; y < num; y++) {
       std::cout << ptrToPtr[y] << std::endl;
       std::cout << numberOfElements[y] << std::endl;
       std::cout << currentSize[y] << std::endl;
       std::cout << capacity[y] << std::endl;
     }*/
  }

  ~listOfDynamicListsInt() {
    std::cout << "NOOOOOOO" << std::endl;
    for (int x = 0; x < numOfLists; x++) {
      munmap(ptrToPtr[x], pageSize);
    }
  }
  void resize(int index, int amount) {
    while (canAddData == false) {
      std::this_thread::yield();
    }
    canReedData = false;
    // std::cout << "REZISE " << std::endl;
    float bytesNeeded = amount * sizeof(int);
    int neededPages = static_cast<int>(std::ceil((bytesNeeded / pageSize)));

    void *newData =
        mmap(nullptr, (neededPages * pageSize), PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // kopir lige det gamle data over
    // std::cout << currentSize << std::endl;
    memcpy(newData, ptrToPtr[index], currentSize[index]);

    if (munmap(ptrToPtr[index], capacity[index]) != 0) {
      perror("munmap");
    }
    ptrToPtr[index] = newData;
    capacity[index] = neededPages * pageSize;

    // std::cout << "new ptr " << ptrToPtr[index] << std::endl;
    // std::cout << "new capacity in bytes : " << capacity[index] << std::endl;
    canReedData = true;
  }

  void add(int index, int *numToAdd) {

    if (*currentSize == *capacity) {
      // std::cout << "GOING TO REZISE " << std::endl;
      resize(index, (*numberOfElements + 500));
    }

    /*std::cout << "Current size : " << *currentSize << std::endl;
    std::cout << "capasity : " << *capacity << std::endl;*/
    /*  std::cout << "-----" << std::endl;
      std::cout << currentSize[0] << std::endl;
      std::cout << currentSize[1] << std::endl;
      std::cout << currentSize[2] << std::endl;
      std::cout << currentSize[3] << std::endl;
      std::cout << "----------" << std::endl;*/
    void *ptr = ptrToPtr[index];
    // std::cout << currentSize[index] << std::endl;
    void *newptr = (char *)ptr + currentSize[index];

    // std::cout << ptr << std::endl;
    // std::cout << newptr << std::endl;
    numberOfElements[index]++;
    // amoutOfData[index]++;
    currentSize[index] = numberOfElements[index] * 4;
    // std::cout << currentSize[index] << std::endl;
    memcpy(newptr, numToAdd, sizeof(int));
  }

  bool isAtEnd(int threadIndex, int index) {
    if (index == numberOfElements[threadIndex]) {
      // std::cout << "numberOfElements : " << numberOfElements[threadIndex]
      //         << std::endl;
      return true;
    }
    return false;
  }

  int getJobIndex(int threadIndex, int number) {
    canAddData = false;
    while (canReedData == false) {
      std::this_thread::yield();
    }

    // bool shoudBe = canReedData.load();

    // do {
    // void *ptr = ptrToPtr[threadIndex];
    void *ptr = (char *)ptrToPtr[threadIndex] + (sizeof(int) * number);
    // void *newPtr = (char *)ptr + (sizeof(int) * number);

    int returnVal = *(int *)ptr;
    //}
    // while(!canReedData.compare_exchange_strong(shoudBe, TRUE));

    canAddData = true;
    return returnVal;
  }
};

class lockClass {
private:
  std::atomic<int> *ptr;

public:
  lockClass(std::atomic<int> *x) {
    *x += 1;
    ptr = x;
  };
  ~lockClass() {
    *ptr -= 1;
    // ptr = nullptr;
  };
};

template <typename func> class myVector {
private:
  func *dataType;
  size_t amoutDataStored;
  size_t currentSize;
  size_t capacity;
  size_t pageSize;
  void *data;

  bool canReedData;
  std::atomic<int> lockVal = 0;
  // int* lockValPtr = &lockVal;

  // lockClass* obj;

  std::atomic<int> timeResized;

  std::atomic<bool> canResize = false;

  // std::atomic_flag lock = ATOMIC_FLAG_INIT;

public:
  myVector() {

    data = mmap(nullptr, 1024, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    pageSize = sysconf(_SC_PAGESIZE);
    capacity = pageSize;
    amoutDataStored = 0;
    canReedData = true;
    currentSize = amoutDataStored * sizeof(*dataType);
    std::cout << "Size " << sizeof(*dataType) << std::endl;
    std::cout << "Page size : " << pageSize << std::endl;
  }
  ~myVector() { munmap(data, pageSize); }

  void add(func input) {
    int thisDataNumber;

    /*jobda *test = (jobda *)input;

    test->job();*/

    if (currentSize == capacity) {
      resize(amoutDataStored + 500);
    }

    if (currentSize != 0) {
      int currentDataNumber = currentSize / sizeof(*dataType);
      thisDataNumber = currentDataNumber + 1;
    } else {
      thisDataNumber = 1;
    }

    currentSize += sizeof(*dataType);
    thisDataNumber--;

    void *ptr = (char *)data + (sizeof(*dataType) * (thisDataNumber));

    // new (ptr) t(std::move(input));
    std::memcpy(ptr, &input, sizeof(*dataType));
    amoutDataStored++;
  }

  void resize(int amount) {
    canReedData = false;

     /*while (!canResize) {

       std::this_thread::yield();
     }*/
    while (lockVal != 0) {
      std::this_thread::yield();
    }
    float bytesNeeded = amount * sizeof(*dataType);
    int neededPages = static_cast<int>(std::ceil((bytesNeeded / pageSize)));

    //timeResized.fetch_add(1);

    void *newData =
        mmap(nullptr, (neededPages * pageSize), PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // kopir lige det gamle data over
    // std::cout << currentSize << std::endl;
    memcpy(newData, data, currentSize);

    void *oldPtr = data;
    data = nullptr;

    if (munmap(oldPtr, capacity) != 0) {
      perror("munmap");
    }
    data = newData;
    capacity = neededPages * pageSize;
    // newData = nullptr;
    //  std::cout << "new capacity in bytes : " << capacity << std::endl;
    canReedData = true;
  }

  void *getVal(int number) {

    int bytesToShift = number * sizeof(*dataType);

    void *ptr = (char *)data + bytesToShift;
    return ptr;
  }

  jobData getJob(int number) {

    // std::cout << lockVal << std::endl;
      while (canReedData == false) {
          std::this_thread::yield();
      }

      lockClass l_LockClass(&lockVal);

  back:

      //int temp = timeResized.load();

      // canResize = false;
    //  lockClass l_LockClass(&lockVal);

    // std::cout << lockVal << std::endl;

    //int temp = timeResized.load();

    int bytesToShift = number * sizeof(*dataType);

    void *pr = (char *)data + bytesToShift;
    // void *pr = reinterpret_cast<void *>((char *)data + bytesToShift);


    return *(jobData *)pr;
  }

  bool atEnd(int index) {
    if (index == amoutDataStored) {
      return true;
    }
    return false;
  }

  void clear() {
    amoutDataStored = 0;
    currentSize = 0;
  }
};
