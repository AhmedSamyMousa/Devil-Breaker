#include <iostream>
#include <random>
#include <chrono>
#include <string>
#include <thread>

// Character sets
const std::string lowercaseChars = "abcdefghijklmnopqrstuvwxyz";
const std::string uppercaseChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string numberChars = "0123456789";
const std::string specialChars = "!@#$%^&*()_+-=[]{}|;:'\",.<>?/";

// Function to generate a password given a specific seed (CPU mode)
std::string generatePassword(int length, uint64_t seed, const std::string& chars) {
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<> dist(0, chars.size() - 1);
    std::string password;

    for (int i = 0; i < length; i++) {
        password += chars[dist(gen)];
    }
    return password;
}

// Brute-force using CPU
void crackPasswordCPU(const std::string& targetPassword, uint64_t startSeed, uint64_t attempts) {
    int length = targetPassword.length();
    std::string possibleChars = lowercaseChars + uppercaseChars + numberChars + specialChars;

    for (uint64_t seed = startSeed; seed > startSeed - attempts; seed--) {
        std::string generatedPassword = generatePassword(length, seed, possibleChars);
        std::cout << "Trying: " << generatedPassword << " (Seed: " << seed << ")\n";

        if (generatedPassword == targetPassword) {
            std::cout << "\nPassword cracked! Seed: " << seed << "\n";
            return;
        }
    }
    std::cout << "\nFailed to crack the password in the given seed range.\n";
}

#ifdef __CUDACC__
#include <cuda_runtime.h>
#include <curand_kernel.h>

__constant__ char possibleChars[94] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:'\",.<>?/";

// CUDA Kernel
__global__ void crackPasswordKernel(char* target, int length, uint64_t startSeed, uint64_t attempts, int* found, uint64_t* foundSeed) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    uint64_t seed = startSeed - idx;

    if (seed < startSeed - attempts) return;

    curandState state;
    curand_init(seed, 0, 0, &state);

    char generated[20];
    for (int i = 0; i < length; i++) {
        generated[i] = possibleChars[curand(&state) % 94];
    }

    bool match = true;
    for (int i = 0; i < length; i++) {
        if (generated[i] != target[i]) {
            match = false;
            break;
        }
    }

    if (match) {
        *found = 1;
        *foundSeed = seed;
    }
}

// Launch CUDA
void crackPasswordGPU_CUDA(const std::string& targetPassword, uint64_t startSeed, uint64_t attempts) {
    int length = targetPassword.length();
    char* d_target;
    int* d_found;
    uint64_t* d_foundSeed;
    cudaMalloc(&d_target, length);
    cudaMalloc(&d_found, sizeof(int));
    cudaMalloc(&d_foundSeed, sizeof(uint64_t));

    cudaMemcpy(d_target, targetPassword.c_str(), length, cudaMemcpyHostToDevice);
    int found = 0;
    uint64_t foundSeed = 0;
    cudaMemcpy(d_found, &found, sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_foundSeed, &foundSeed, sizeof(uint64_t), cudaMemcpyHostToDevice);

    int threadsPerBlock = 256;
    int numBlocks = (attempts + threadsPerBlock - 1) / threadsPerBlock;

    crackPasswordKernel << <numBlocks, threadsPerBlock >> > (d_target, length, startSeed, attempts, d_found, d_foundSeed);

    cudaMemcpy(&found, d_found, sizeof(int), cudaMemcpyDeviceToHost);
    cudaMemcpy(&foundSeed, d_foundSeed, sizeof(uint64_t), cudaMemcpyDeviceToHost);

    cudaFree(d_target);
    cudaFree(d_found);
    cudaFree(d_foundSeed);

    if (found) {
        std::cout << "\nPassword cracked! Seed: " << foundSeed << "\n";
    }
    else {
        std::cout << "\nFailed to crack the password in the given seed range.\n";
    }
}
#endif

#ifdef __APPLE__
// OpenCL for AMD/NVIDIA GPUs
#include <CL/cl.h>

// OpenCL kernel source
const char* openclKernelSource = R"CLC(
    __constant char possibleChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:'\",.<>?/";
    __kernel void crackPassword(__global char* target, int length, ulong startSeed, ulong attempts, __global int* found, __global ulong* foundSeed) {
        int idx = get_global_id(0);
        ulong seed = startSeed - idx;

        if (seed < startSeed - attempts) return;

        uint state = seed;
        char generated[20];
        for (int i = 0; i < length; i++) {
            generated[i] = possibleChars[state % 94];
            state = state * 1664525 + 1013904223;
        }

        bool match = true;
        for (int i = 0; i < length; i++) {
            if (generated[i] != target[i]) {
                match = false;
                break;
            }
        }

        if (match) {
            *found = 1;
            *foundSeed = seed;
        }
    }
)CLC";

// OpenCL wrapper function (Not fully implemented)
void crackPasswordGPU_OpenCL(const std::string& targetPassword, uint64_t startSeed, uint64_t attempts) {
    std::cout << "\nOpenCL support is not fully implemented yet.\n";
}
#endif

int main() {
    std::string targetPassword = "P";
    uint64_t now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    uint64_t attempts = 10000000;

    std::cout << "Select mode:\n";
    std::cout << "1. CPU (Slow)\n";
#ifdef __CUDACC__
    std::cout << "2. GPU (NVIDIA CUDA - Fast)\n";
#endif
#ifdef __APPLE__
    std::cout << "3. GPU (AMD/NVIDIA OpenCL - Fast)\n";
#endif
    int choice;
    std::cin >> choice;

    if (choice == 1) {
        crackPasswordCPU(targetPassword, now, attempts);
    }
#ifdef __CUDACC__
    else if (choice == 2) {
        crackPasswordGPU_CUDA(targetPassword, now, attempts);
    }
#endif
#ifdef __APPLE__
    else if (choice == 3) {
        crackPasswordGPU_OpenCL(targetPassword, now, attempts);
    }
#endif
    else {
        std::cout << "Invalid choice!\n";
    }

    return 0;
}
