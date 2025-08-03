#ifndef PARSED_ID_H
#define PARSED_ID_H

#include <string>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <cassert>
#include <iomanip>
#include <sstream>

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
    
    // Enhanced validation with assertions
    bool isValid() const {
        // Dimension validation
        assert(dim >= 1 && dim <= 4);
        if (!(dim >= 1 && dim <= 4)) return false;
        
        // Digit range validations
        assert(trans_digit >= 0 && trans_digit <= 99);
        if (!(trans_digit >= 0 && trans_digit <= 99)) return false;
        
        assert(harm_digit >= 0 && harm_digit <= 99);
        if (!(harm_digit >= 0 && harm_digit <= 99)) return false;
        
        assert(fx_digit >= 0 && fx_digit <= 99);
        if (!(fx_digit >= 0 && fx_digit <= 99)) return false;
        
        assert(damp_digit >= 0 && damp_digit <= 99);
        if (!(damp_digit >= 0 && damp_digit <= 99)) return false;
        
        assert(freq_digit >= 0 && freq_digit <= 99);
        if (!(freq_digit >= 0 && freq_digit <= 99)) return false;
        
        // Prime validation
        assert(tuning_prime >= 2 && tuning_prime <= 11);
        if (!(tuning_prime >= 2 && tuning_prime <= 11)) return false;
        
        // Type validation
        assert(type == 'i' || type == 'g' || type == 'x' || type == 'm' || type == 's');
        if (!(type == 'i' || type == 'g' || type == 'x' || type == 'm' || type == 's')) return false;
        
        return true;
    }
    
    // Debug string representation
    std::string toString() const {
        return std::to_string(dim) + "." + formatAttrs() + type;
    }
    
private:
    std::string formatAttrs() const {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << trans_digit
           << std::setfill('0') << std::setw(2) << harm_digit
           << std::setfill('0') << std::setw(2) << fx_digit
           << tuning_prime
           << std::setfill('0') << std::setw(2) << damp_digit
           << std::setfill('0') << std::setw(2) << freq_digit;
        return ss.str();
    }
};

// Enhanced safe string to integer conversion with logging
inline int safeStoi(const std::string& str, int defaultValue = 50) {
    try {
        int value = std::stoi(str);
        return std::max(0, std::min(99, value)); // Cap to 0-99
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to convert '" << str << "' to integer: " 
                  << e.what() << " (using default " << defaultValue << ")" << std::endl;
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