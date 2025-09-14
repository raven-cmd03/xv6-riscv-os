#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MATRIX_SIZE 10
#define NUM_PROCESSES 4

// Function to initialize a matrix with values
void init_matrix(int matrix[MATRIX_SIZE][MATRIX_SIZE], int value) {
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            matrix[i][j] = value + i * MATRIX_SIZE + j;
        }
    }
}

// Function to print a matrix
void print_matrix(int matrix[MATRIX_SIZE][MATRIX_SIZE], const char* name) {
    printf("%s:\n", name);
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            printf("%4d ", matrix[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

// Function to multiply a portion of matrices
void multiply_portion(int start_row, int end_row, 
                     int matrix_a[MATRIX_SIZE][MATRIX_SIZE],
                     int matrix_b[MATRIX_SIZE][MATRIX_SIZE],
                     int result[MATRIX_SIZE][MATRIX_SIZE]) {
    for (int i = start_row; i < end_row; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            result[i][j] = 0;
            for (int k = 0; k < MATRIX_SIZE; k++) {
                result[i][j] += matrix_a[i][k] * matrix_b[k][j];
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int matrix_a[MATRIX_SIZE][MATRIX_SIZE];
    int matrix_b[MATRIX_SIZE][MATRIX_SIZE];
    int result[MATRIX_SIZE][MATRIX_SIZE];
    int pipes[NUM_PROCESSES][2]; // Array of pipes for communication
    int pids[NUM_PROCESSES];
    
    printf("=== Distributed Matrix Multiplication ===\n");
    printf("Matrix size: %dx%d\n", MATRIX_SIZE, MATRIX_SIZE);
    printf("Number of processes: %d\n\n", NUM_PROCESSES);
    
    // Initialize matrices
    init_matrix(matrix_a, 1);
    init_matrix(matrix_b, 2);
    
    // Print input matrices
    print_matrix(matrix_a, "Matrix A");
    print_matrix(matrix_b, "Matrix B");
    
    // Create pipes for communication
    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (pipe(pipes[i]) < 0) {
            printf("Error: Failed to create pipe %d\n", i);
            exit(1);
        }
    }
    
    // Calculate rows per process
    int rows_per_process = MATRIX_SIZE / NUM_PROCESSES;
    int remaining_rows = MATRIX_SIZE % NUM_PROCESSES;
    
    // Fork processes for parallel computation
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pids[i] = fork();
        
        if (pids[i] < 0) {
            printf("Error: Fork failed for process %d\n", i);
            exit(1);
        }
        else if (pids[i] == 0) {
            // Child process
            close(pipes[i][0]); // Close read end
            
            // Calculate start and end rows for this process
            int start_row = i * rows_per_process;
            int end_row = start_row + rows_per_process;
            
            // Last process gets any remaining rows
            if (i == NUM_PROCESSES - 1) {
                end_row += remaining_rows;
            }
            
            // Create partial result matrix
            int partial_result[MATRIX_SIZE][MATRIX_SIZE];
            
            // Initialize partial result
            for (int row = 0; row < MATRIX_SIZE; row++) {
                for (int col = 0; col < MATRIX_SIZE; col++) {
                    partial_result[row][col] = 0;
                }
            }
            
            // Perform multiplication for assigned rows
            multiply_portion(start_row, end_row, matrix_a, matrix_b, partial_result);
            
            // Send partial result through pipe
            if (write(pipes[i][1], partial_result, sizeof(partial_result)) < 0) {
                printf("Error: Failed to write to pipe %d\n", i);
                exit(1);
            }
            
            close(pipes[i][1]); // Close write end
            exit(0);
        }
        else {
            // Parent process
            close(pipes[i][1]); // Close write end
        }
    }
    
    // Parent process: collect results from all children
    printf("Collecting results from child processes...\n");
    
    // Initialize result matrix
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            result[i][j] = 0;
        }
    }
    
    // Read partial results from each child process
    for (int i = 0; i < NUM_PROCESSES; i++) {
        int partial_result[MATRIX_SIZE][MATRIX_SIZE];
        
        if (read(pipes[i][0], partial_result, sizeof(partial_result)) < 0) {
            printf("Error: Failed to read from pipe %d\n", i);
            exit(1);
        }
        
        // Add partial result to final result
        for (int row = 0; row < MATRIX_SIZE; row++) {
            for (int col = 0; col < MATRIX_SIZE; col++) {
                result[row][col] += partial_result[row][col];
            }
        }
        
        close(pipes[i][0]); // Close read end
    }
    
    // Wait for all child processes to complete
    for (int i = 0; i < NUM_PROCESSES; i++) {
        int status;
        wait(&status);
        printf("Child process %d completed with status: %d\n", i, status);
    }
    
    // Print final result
    print_matrix(result, "Result Matrix (A × B)");
    
    // Verify with a simple calculation for first few elements
    printf("Verification (first 3x3 elements):\n");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            int expected = 0;
            for (int k = 0; k < MATRIX_SIZE; k++) {
                expected += matrix_a[i][k] * matrix_b[k][j];
            }
            printf("result[%d][%d] = %d (expected: %d) %s\n", 
                   i, j, result[i][j], expected, 
                   (result[i][j] == expected) ? "✓" : "✗");
        }
    }
    
    printf("\n=== Matrix multiplication completed successfully! ===\n");
    exit(0);
}
