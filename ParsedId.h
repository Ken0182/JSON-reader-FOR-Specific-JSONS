#ifndef PARSED_ID_H
#define PARSED_ID_H

#include <string>
#include <algorithm>
#include <numeric>
#include <cmath>

// 4Z ID Structure - shared across all systems
struct ParsedId {
    int dim = 3;
    int trans_digit = 50;
    int harm_digit = 50;
    int fx_digit = 20;
    int tuning_prime = 7;
    int damp_digit = 50;
    int freq_digit = 50;
    char type = 'g';
    
    // Validation helper
    bool isValid() const {
        return (dim >= 1 && dim <= 4) &&
               (trans_digit >= 0 && trans_digit <= 99) &&
               (harm_digit >= 0 && harm_digit <= 99) &&
               (fx_digit >= 0 && fx_digit <= 99) &&
               (tuning_prime >= 2 && tuning_prime <= 11) &&
               (damp_digit >= 0 && damp_digit <= 99) &&
               (freq_digit >= 0 && freq_digit <= 99) &&
               (type == 'i' || type == 'g' || type == 'x' || type == 'm' || type == 's');
    }
};

// Shared utility functions
inline int safeStoi(const std::string& str, int defaultValue = 50) {
    try {
        int value = std::stoi(str);
        return std::max(0, std::min(99, value)); // Cap to 0-99
    } catch (const std::exception&) {
        return defaultValue;
    }
}

// GCD using std::gcd from <numeric>
inline int calculateGcd(int a, int b) {
    return std::gcd(a, b);
}

// Validate and fix tuning prime
inline int validateTuningPrime(int prime) {
    if (prime < 2 || prime > 11) return 7; // Default NA
    // Valid primes: 2, 3, 5, 7, 11
    if (prime == 2 || prime == 3 || prime == 5 || prime == 7 || prime == 11) {
        return prime;
    }
    return 7; // Default if not a valid prime
}

#endif // PARSED_ID_H