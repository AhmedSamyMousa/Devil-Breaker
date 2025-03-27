#include <iostream>
#include <random>
#include <chrono>
#include <string>

const std::string lowercaseChars = "abcdefghijklmnopqrstuvwxyz";
const std::string uppercaseChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string numberChars = "0123456789";
const std::string specialChars = "!@#$%^&*()_+-=[]{}|;:'\",.<>?/";

std::string generatePassword(int length, uint64_t seed, const std::string& chars) {
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<> dist(0, chars.size() - 1);
    std::string password;

    for (int i = 0; i < length; i++) {
        password += chars[dist(gen)];
    }
    return password;
}

int main() {
    std::string targetPassword = "!Ccngz@ueh4E";
    int length = targetPassword.length();
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    std::string possibleChars = lowercaseChars + uppercaseChars + numberChars + specialChars;
    for (uint64_t seed = now; seed > now - 10000000; seed--) {
        std::string generatedPassword = generatePassword(length, seed, possibleChars);
        std::cout << "Trying seed " << seed << " -> " << generatedPassword << std::endl;

        if (generatedPassword == targetPassword) {
            std::cout << "\nPassword cracked! Seed: " << seed << "\n";
            return 0;
        }
    }

    std::cout << "\nFailed to crack the password within the seed range.\n";
    return 1;
}
