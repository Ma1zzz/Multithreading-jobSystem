# Cpp multithreading JobSystem

C++ job system designed to make multithreading less of a pain in the ass

## Performance

Benchmarked on Apple M1 with 3 worker threads and the main thread<br>
Tests performed with empty functions to measure pure overhead

| Jobs | Time      | Overhead per Job |
|------|-----------|------------------|
| 50,000 | 3-4ms     | 60-80ns          |
| 10,000 | 0.6-0.7ms | 60-70ns          |

## Limitations

**macOS only** (for now): Linux/Windows support in progress<br>
**No job dependencies**: Jobs can't depend on each other<br>
**No priorities**: Jobs are executed in the order they are added<br>

## Working On

**Cross-platform support**<br>
**Job dependencies**<br>
**Priority queues**<br>
**Job groups**<br>


## How to use

```cpp
#include "jobs.h"

void functionName() {
    // Do something
}

int main() {
    // Enter the number of total threads 
    jobSystem::init(4) // so this program will use 4 threads. 3 worker and one main
    
    // Submit a job
    jobSystem::reqJob(functionName);
    
    
    jobSystem::waitAllJobs()// Will wait till jobs are done. 
    // the main thread will help finished the remaining jobs
    
    jobSystem::clear() // this will clear the jobs queue
    // and free used memory
    // if you programs runs in a while loop use this at the end of it before the next literation
    // waitAllJobs must be called before this no no jobs can be running
    
    jobSystem::shutdown() // just shuts it down. put at end of program before it closes
   
}