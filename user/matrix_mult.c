#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Function to perform matrix multiplication C = A * B
void matrix_multiply(int **A, int **B, int **C, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            C[i][j] = 0;
            for (int k = 0; k < size; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

// Function to allocate a 2D matrix
int** allocate_matrix(int size) {
    int **matrix = (int**)malloc(size * sizeof(int*));
    if (!matrix) {
        printf("Failed to allocate matrix rows\n");
        exit(1);
    }
    
    for (int i = 0; i < size; i++) {
        matrix[i] = (int*)malloc(size * sizeof(int));
        if (!matrix[i]) {
            printf("Failed to allocate matrix column %d\n", i);
            exit(1);
        }
    }
    
    return matrix;
}

// Function to initialize matrix with random-like values
void initialize_matrix(int **matrix, int size, int seed) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            // Simple pseudo-random number generation
            matrix[i][j] = (i * size + j + seed) % 100;
        }
    }
}

// Function to free a 2D matrix
void free_matrix(int **matrix, int size) {
    for (int i = 0; i < size; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

int main(int argc, char *argv[]) {
    int sizes[] = {50, 100, 150, 200, 250, 300, 350, 400};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    printf("Matrix Multiplication Performance Test\n");
    printf("=====================================\n");
    printf("Testing different matrix sizes using expanded RAM\n");
    printf("Measuring cycles, time, and instructions retired\n\n");
    
    for (int s = 0; s < num_sizes; s++) {
        int size = sizes[s];
        printf("Testing matrix size: %d x %d\n", size, size);
        
        // Allocate matrices
        int **A = allocate_matrix(size);
        int **B = allocate_matrix(size);
        int **C = allocate_matrix(size);
        
        // Initialize matrices
        initialize_matrix(A, size, 1);
        initialize_matrix(B, size, 2);
        
        // Measure performance
        uint64 start_cycles = rdcycle();
        uint64 start_time = rdtime();
        uint64 start_instret = rdinstret();
        
        // Perform matrix multiplication
        matrix_multiply(A, B, C, size);
        
        // Measure performance again
        uint64 end_cycles = rdcycle();
        uint64 end_time = rdtime();
        uint64 end_instret = rdinstret();
        
        // Calculate differences
        uint64 cycles_used = end_cycles - start_cycles;
        uint64 time_used = end_time - start_time;
        uint64 instret_used = end_instret - start_instret;
        
        printf("Results:\n");
        printf("  CPU Cycles: %ld\n", cycles_used);
        printf("  Time: %ld\n", time_used);
        printf("  Instructions Retired: %ld\n", instret_used);
        printf("  Matrix size: %d x %d\n", size, size);
        printf("  Total elements: %d\n", size * size);
        printf("  Memory used: ~%ld MB\n", (size * size * 3 * sizeof(int)) / (1024 * 1024));
        printf("\n");
        
        // Free matrices
        free_matrix(A, size);
        free_matrix(B, size);
        free_matrix(C, size);
    }
    
    printf("Matrix multiplication performance test completed!\n");
    exit(0);
}
