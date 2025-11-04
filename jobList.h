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

template <typename func> class listOfDynamicListsInt {
private:
  // void ** ptrToPtr;

  std::atomic<void *> *ptrToPtr;

  func *dataType;
  size_t *capacity;
  size_t *currentSize;
  std::atomic<int> *numberOfElements;
  int *jobNumber;

  size_t pageSize;

  std::vector<size_t> ptrSizes;
  std::vector<void*> oldPtrs;

  int numOfLists;

public:
  listOfDynamicListsInt(int num) {
    numOfLists = num;
    ptrToPtr = new std::atomic<void *>[num];
    currentSize = new size_t[num];
    capacity = new size_t[num];
    numberOfElements = new std::atomic<int>[num];
    jobNumber = new int[num];
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
  }

  ~listOfDynamicListsInt() {
    for (int x = 0; x < numOfLists; x++) {
      munmap(ptrToPtr[x], capacity[x]);
    }
    delete[] ptrToPtr;
    delete[] currentSize;
    delete[] capacity;
    delete[] numberOfElements;
    delete[] jobNumber;
  }


  void resize(int index, int amount) {
    // std::cout << "REZISE " << std::endl;

    float bytesNeeded = amount * sizeof(*dataType);
    int neededPages = static_cast<int>(std::ceil((bytesNeeded / pageSize)));

    size_t oldSize = currentSize[index];
    void *oldPtr = ptrToPtr[index].load();
    //void *oldPtr = ptrToPtr[index].exchange(nullptr);

    // void* oldPtr = ptrToPtr[index].load(std::memory_order_acquire);

    void *newData =
        mmap(nullptr, (neededPages * pageSize), PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);


    memcpy(newData, oldPtr, currentSize[index]);



    ptrToPtr[index].store(newData, std::memory_order_release);
    capacity[index] = neededPages * pageSize;

    oldPtrs.push_back(oldPtr);
    ptrSizes.push_back(oldSize);


  }

  void add(int index, func input) {

    if (currentSize[index] == capacity[index]) {
      // std::cout << "GOING TO REZISE " << std::endl;
      resize(index, (numberOfElements[index] + pageSize));
    }

    void *ptr = ptrToPtr[index].load(std::memory_order_acquire);
    // std::cout << currentSize[index] << std::endl;
    void *newptr = (char *)ptr + currentSize[index];

    // std::cout << currentSize[index] << std::endl;
    memcpy(newptr, &input, sizeof(*dataType));

    numberOfElements[index]++;
    currentSize[index] = numberOfElements[index] * sizeof(*dataType);
  }

  bool isAtEnd(int threadIndex, int &number) {
    if (number == numberOfElements[threadIndex]) {
      // std::cout << "numberOfElements : " << numberOfElements[threadIndex]
      //         << std::endl;
      return true;
    }
    return false;
  }

  func getJob(int index, int &number) {


    int bytesToShift = number * sizeof(func);


     void* raw_ptr = ptrToPtr[index].load(std::memory_order_acquire);

     char *byte_ptr = static_cast<char *>(raw_ptr);

    byte_ptr += bytesToShift;

    func tr = *(func *)byte_ptr;


    jobData test = tr;

    if (test.func == nullptr) {
      std::cerr << "SHOUD NOt SE THIS " << std::endl;
    }


    return tr;
  }

  void clear() {

    for (int z = 0; z < numOfLists; z++) {
      if (munmap(ptrToPtr[z], capacity[z]) != 0) {
        std::cerr << "clear unmap no working" << std::endl;
      }
      void *newData = mmap(nullptr, pageSize, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      ptrToPtr[z] = newData;

      capacity[z] = pageSize;
      currentSize[z] = 0;
      numberOfElements[z] = 0;
      jobNumber[z] = 0;
    }

      while (!oldPtrs.empty())
      {
          if (munmap(oldPtrs.front(), ptrSizes.front()) != 0) {
              perror("munmap");
          }
          ptrSizes.erase(ptrSizes.begin());
          oldPtrs.erase(oldPtrs.begin());
      }

  }
};