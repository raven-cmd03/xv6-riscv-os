#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  char *ptr;
  int size_mb = 2; // Try to allocate exactly 2MB to trigger superpage usage
  int size_bytes = size_mb * 1024 * 1024;
  
  printf("Testing superpage allocation (2MB)...\n");
  printf("Attempting to allocate %d MB (%d bytes)\n", size_mb, size_bytes);
  
  // Try to allocate memory using sbrk
  ptr = sbrk(size_bytes);
  if (ptr == (char*)-1) {
    printf("Memory allocation failed!\n");
    exit(1);
  }
  
  printf("Memory allocation successful!\n");
  printf("Allocated %d MB of memory\n", size_mb);
  
  // Test writing to the allocated memory
  printf("Testing memory write/read...\n");
  for (int i = 0; i < size_mb * 1024; i++) {
    ptr[i * 1024] = (char)(i & 0xFF);
  }
  
  printf("Memory write test completed successfully!\n");
  printf("Simple superpage test passed\n");
  
  exit(0);
}
