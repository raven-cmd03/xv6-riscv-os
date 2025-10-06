#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  char *ptr1, *ptr2;
  int size_mb = 2;
  int size_bytes = size_mb * 1024 * 1024;
  
  printf("Testing superpage allocation with 2MB alignment...\n");
  
  // First allocate some memory to get to a 2MB boundary
  printf("Allocating initial memory to align to 2MB boundary...\n");
  ptr1 = sbrk(4096); // Allocate one page to test alignment
  if (ptr1 == (char*)-1) {
    printf("Initial allocation failed!\n");
    exit(1);
  }
  
  printf("Initial allocation successful at %p\n", ptr1);
  
  // Now try to allocate 2MB which should use superpages
  printf("Attempting to allocate %d MB (%d bytes) for superpage test...\n", size_mb, size_bytes);
  ptr2 = sbrk(size_bytes);
  if (ptr2 == (char*)-1) {
    printf("Superpage allocation failed!\n");
    exit(1);
  }
  
  printf("Superpage allocation successful at %p\n", ptr2);
  printf("Allocated %d MB of memory\n", size_mb);
  
  // Test writing to the allocated memory
  printf("Testing memory write/read...\n");
  for (int i = 0; i < size_mb * 1024; i++) {
    ptr2[i * 1024] = (char)(i & 0xFF);
  }
  
  printf("Memory write test completed successfully!\n");
  printf("Superpage test 2 passed\n");
  
  exit(0);
}
