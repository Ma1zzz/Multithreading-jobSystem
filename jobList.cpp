#pragma once
#include "jobList.h"
#include <atomic>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

listOfDynamicListsInt::listOfDynamicListsInt(int num) {
  numOfLists = num;
  ptrToPtr = new std::atomic<void *>[num];
  currentSize = new size_t[num];
  capacity = new size_t[num];
  numberOfElements = new std::atomic<int>[num];
  pageSize = sysconf(_SC_PAGESIZE);
  mutexs = new std::mutex[num];

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

listOfDynamicListsInt::~listOfDynamicListsInt() {
  for (int x = 0; x < numOfLists; x++) {
    munmap(ptrToPtr[x], capacity[x]);
  }
  delete[] ptrToPtr;
  delete[] currentSize;
  delete[] capacity;
  delete[] numberOfElements;

  delete[] mutexs;
}

void listOfDynamicListsInt::resize(int index, int pagesToAdd) {
  // std::cout << "REZISE " << std::endl;

  /* float bytesNeeded = amount * sizeof(*dataType);
   int neededPages = static_cast<int>(std::ceil((bytesNeeded / pageSize)));*/

  int neededPages = (capacity[index] / pageSize) + pagesToAdd;

  size_t oldSize = currentSize[index];
  void *oldPtr = ptrToPtr[index].load();

  void *newData =
      mmap(nullptr, (neededPages * pageSize), PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  memcpy(newData, oldPtr, currentSize[index]);

  ptrToPtr[index].store(newData, std::memory_order_release);
  capacity[index] = neededPages * pageSize;

  oldPtrs.push_back(oldPtr);
  ptrSizes.push_back(oldSize);
}

void listOfDynamicListsInt::add(int index, jobData input) {

  if (currentSize[index] == capacity[index]) {
    // std::cout << "GOING TO REZISE " << std::endl;
    resize(index, 2);
    // std::cout << pageSize << std::endl;
    // std::cout << capacity[index] << std::endl;
  }

  void *ptr = ptrToPtr[index].load(std::memory_order_acquire);
  // std::cout << currentSize[index] << std::endl;
  void *newptr = (char *)ptr + currentSize[index];

  // std::cout << currentSize[index] << std::endl;
  memcpy(newptr, &input, sizeof(jobData));

  numberOfElements[index]++;
  currentSize[index] = numberOfElements[index] * sizeof(jobData);
  ;
}

bool listOfDynamicListsInt::isAtEnd(int threadIndex, std::atomic<int> *number) {
  if (*number == numberOfElements[threadIndex]) {

    return true;
  }
  return false;
}

jobData listOfDynamicListsInt::getJob(int index, std::atomic<int> *number) {

  mutexs[index].lock();

  if (number->load() >= numberOfElements[index].load()) {
    jobData sendBack;
    sendBack.func = nullptr;

    mutexs[index].unlock();
    return sendBack;
  }

  void *raw_ptr = ptrToPtr[index].load(std::memory_order_acquire);

  char *byte_ptr = static_cast<char *>(raw_ptr);

  byte_ptr += number->load() * sizeof(jobData);
  ;

  jobData dataToSendBack = *(jobData *)byte_ptr;

  number->fetch_add(1);

  mutexs[index].unlock();

  return dataToSendBack;
}

void listOfDynamicListsInt::clear() {

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
  }

  while (!oldPtrs.empty()) {
    if (munmap(oldPtrs.front(), ptrSizes.front()) != 0) {
      perror("munmap");
    }
    ptrSizes.erase(ptrSizes.begin());
    oldPtrs.erase(oldPtrs.begin());
  }
}
