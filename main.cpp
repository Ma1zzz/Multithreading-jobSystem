#include "atomic"
#include "chrono"
#include "jobs.h"
#include "printf.h"
#include <cmath>
#include <iostream>
#include <thread>
#include <unistd.h>

int testVarible;

void PrimeCalculation3() {
  int count = 0;
  for (int n = 2; n < 500000; n++) {
    bool isPrime = true;
    for (int i = 2; i <= sqrt(n); i++) {
      if (n % i == 0) {
        isPrime = false;
        break;
      }
    }
    if (isPrime)
      count++;
  }
  std::cout << "Found " << count << " primes\n";
}

std::atomic<int> jobN;
void PrimeCalculation4() {
  int count = 0;
  for (int n = 2; n < 500; n++) {
    bool isPrime = true;
    for (int i = 2; i <= sqrt(n); i++) {
      if (n % i == 0) {
        isPrime = false;
        break;
      }
    }
    if (isPrime)
      count++;
  }
  // jobN++;
  //  std::cout << "job Number : " << jobN << std::endl;
  //  std::cout << "Found " << count << " primes\n";
}

void noCodeTest() {}

// void test() { DEFINE_JOB(PrimeCalculation, nullptr, 1) }

void printNumber(int number) {
  // std::cout << number << std::endl;
  ;
}

//  DEFINE_JOB(printNumber, nullptr, 1, testVarible)

// DEFINE_JOB(PrimeCalculation, printHello, 1)
// DEFINE_JOB(PrimeCalculation2, nullptr, 1)

int main() {
  initJobsSystem();
  auto start = std::chrono::high_resolution_clock::now();

  // DEFINE_JOB(PrimeCalculation4, nullptr, 0);
  //  auto start = std::chrono::high_resolution_clock::now();
  doJobs();

  auto rgestart = std::chrono::high_resolution_clock::now();
  for (int x = 0; x < 50000; x++) {

    // reqJobs(PrimeCalculation4);
    reqJobs(noCodeTest);
    //                    std::cout << x << std::endl;
    //                   noCodeTest();
    //  PrimeCalculation4();
  }
  std::cout << "done adding jobs" << std::endl;
  auto rqe = std::chrono::high_resolution_clock::now();
  auto dur = duration_cast<std::chrono::microseconds>(rqe - rgestart);
  std::cout << "microseconds : " << dur.count() << std::endl;

  //  sleep(1);
  //  PrimeCalculation4();

  // PrimeCalculation4();
  // sleep(1);
  waitAllJobs();
  //  std::cout << "er noget her til " << std::endl;

  //  shutdownJobsSystem();
  // std::cout << "number : " << count << std::endl;
  auto stop = std::chrono::high_resolution_clock::now();
  auto duration = duration_cast<std::chrono::milliseconds>(stop - start);
  std::cout << "milliseconds : " << duration.count() << std::endl;

  shutdownJobsSystem();
  std::cout << jobN << std::endl;
  return 0;
}
