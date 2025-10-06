#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define TEST_SIZE_MB 4
#define TEST_SIZE_BYTES (TEST_SIZE_MB * 1024 * 1024)

// Function declarations for system calls
uint64 rdcycle(void);
uint64 rdtime(void);
uint64 rdinstret(void);

int main(int argc, char *argv[]) {
  char *ptr;
  uint64 start_cycles, end_cycles;
  uint64 start_time, end_time;
  uint64 start_instret, end_instret;
  
  printf("Superpage Performance Benchmark\n");
  printf("===============================\n");
  printf("Testing allocation and access patterns for %d MB\n\n", TEST_SIZE_MB);
  
  // Measure allocation performance
  printf("1. Testing memory allocation performance...\n");
  printf("  Attempting to allocate %d MB (%d bytes)...\n", TEST_SIZE_MB, TEST_SIZE_BYTES);
  
  start_cycles = rdcycle();
  start_time = rdtime();
  start_instret = rdinstret();
  
  printf("  Calling sbrk...\n");
  ptr = sbrk(TEST_SIZE_BYTES);
  printf("  sbrk returned: %p\n", ptr);
  
  if (ptr == (char*)-1) {
    printf("Memory allocation failed!\n");
    exit(1);
  }
  
  printf("  Allocation successful, measuring end time...\n");
  end_cycles = rdcycle();
  end_time = rdtime();
  end_instret = rdinstret();
  
  printf("Allocation Results:\n");
  printf("  CPU Cycles: %ld\n", end_cycles - start_cycles);
  printf("  Time: %ld\n", end_time - start_time);
  printf("  Instructions Retired: %ld\n", end_instret - start_instret);
  printf("  Allocated at address: %p\n", ptr);
  printf("  Size: %d MB (%d bytes)\n\n", TEST_SIZE_MB, TEST_SIZE_BYTES);
  
  // Measure sequential write performance
  printf("2. Testing sequential write performance...\n");
  start_cycles = rdcycle();
  start_time = rdtime();
  start_instret = rdinstret();
  
  for (int i = 0; i < TEST_SIZE_BYTES; i += 1024) {
    ptr[i] = (char)(i & 0xFF);
  }
  
  end_cycles = rdcycle();
  end_time = rdtime();
  end_instret = rdinstret();
  
  printf("Sequential Write Results:\n");
  printf("  CPU Cycles: %ld\n", end_cycles - start_cycles);
  printf("  Time: %ld\n", end_time - start_time);
  printf("  Instructions Retired: %ld\n", end_instret - start_instret);
  printf("  Throughput: ~%.2f MB/s\n\n", 
         (double)TEST_SIZE_MB / ((double)(end_time - start_time) / 1000000.0));
  
  // Measure random access performance
  printf("3. Testing random access performance...\n");
  start_cycles = rdcycle();
  start_time = rdtime();
  start_instret = rdinstret();
  
  for (int i = 0; i < 10000; i++) {
    int offset = (i * 12345) % TEST_SIZE_BYTES;
    ptr[offset] = (char)(i & 0xFF);
  }
  
  end_cycles = rdcycle();
  end_time = rdtime();
  end_instret = rdinstret();
  
  printf("Random Access Results:\n");
  printf("  CPU Cycles: %ld\n", end_cycles - start_cycles);
  printf("  Time: %ld\n", end_time - start_time);
  printf("  Instructions Retired: %ld\n", end_instret - start_instret);
  printf("  Random accesses: 10,000\n\n");
  
  // Verify memory integrity
  printf("4. Verifying memory integrity...\n");
  int errors = 0;
  for (int i = 0; i < TEST_SIZE_BYTES; i += 1024) {
    if (ptr[i] != (char)(i & 0xFF)) {
      errors++;
      if (errors <= 5) {
        printf("  Error at offset %d: expected %d, got %d\n", 
               i, (i & 0xFF), ptr[i]);
      }
    }
  }
  
  if (errors == 0) {
    printf("  Memory integrity check passed!\n");
  } else {
    printf("  Memory integrity check failed: %d errors found\n", errors);
  }
  
  printf("\nSuperpage benchmark completed successfully!\n");
  exit(0);
}
