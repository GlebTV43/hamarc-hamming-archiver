#include "hamming.h"
#include <iostream>
#include <vector>

HammingCoder::HammingCoder(int bytes) : k_(bytes * kByte) {
    // std::cerr << "DEBUG HammingCoder: bytes=" << bytes << ", k_=" << k_ << std::endl;
    
    if (bytes <= 0) {
        std::cerr << ("bytes must be positive");
    }
    
    CalculateParameters();
    // std::cerr << "DEBUG: k_=" << k_ << ", r_=" << r_ << ", n_=" << n_ << std::endl;
}

void HammingCoder::CalculateParameters() {
    r_ = 1;
    while ((1 << r_) < (k_ + r_ + 1)) {
        r_++;
    }
    n_ = k_ + r_ + 1;
}

bool HammingCoder::IsPowerOfTwo(int x) const {
    return (x & (x - 1)) == 0;
}

bool HammingCoder::Covering(int parity_bit, int data_bit) const {
    return (data_bit & parity_bit) != 0;
}

bool HammingCoder::CalculateOverallParity(const std::vector<char>& block) const {
    bool parity = false;
    
    for (int i = 0; i < n_ - 1; ++i) {
        parity ^= GetBitFromBlock(block, i);
    }
    
    return parity;
}

bool HammingCoder::GetBitFromBlock(const std::vector<char>& block, int index) const {
    int byte = index / kByte;
    int bit = index % kByte;
    
    if (byte < 0 || byte >= static_cast<int>(block.size())) {
        std::cerr << "WARNING GetBitFromBlock: index " << index 
                  << " out of bounds (0-" << block.size() * kByte - 1 << ")" << std::endl;
        return false;
    }
    
    return (block[byte] >> (7 - bit)) & 1;
}

void HammingCoder::SetBitInBlock(std::vector<char>& block, int index, bool value) const {
    int byte = index / kByte;
    int bit = index % kByte;
    
    if (byte < 0 || byte >= static_cast<int>(block.size())) {
        std::cerr << "WARNING SetBitInBlock: index " << index 
                  << " out of bounds (0-" << block.size() * kByte - 1 << ")" << std::endl;
        return;
    }
    
    unsigned char mask = 0x80 >> bit;
    
    if (value) {
        block[byte] |= mask;
    } else {
        block[byte] &= ~mask;
    }
}

std::vector<char> HammingCoder::EncodeFillDataBitsInHamming(const std::vector<char>& block) const {
    int output_bytes = (n_ + kByte - 1) / kByte;
    std::vector<char> encoded(output_bytes, 0);
    
    int data_bit_index = 0;
    
    for (int pos = 1; pos <= n_ - 1 && data_bit_index < k_; ++pos) {
        if (!IsPowerOfTwo(pos)) {
            bool bit_value = GetBitFromBlock(block, data_bit_index);
            SetBitInBlock(encoded, pos - 1, bit_value);
            data_bit_index++;
        }
    }
    
    // std::cerr << "DEBUG EncodeFillDataBitsInHamming: placed " << data_bit_index << " data bits (k_=" << k_ << ")" << std::endl;
    
    return encoded;
}

std::vector<char> HammingCoder::EncodeBlock(const std::vector<char>& data) const {
    int expected_input_bytes = (k_ + kByte - 1) / kByte;
    if (static_cast<int>(data.size()) != expected_input_bytes) {
        std::cerr << "ERROR EncodeBlock: Expected " << expected_input_bytes 
                  << " bytes for " << k_ << " bits, got " << data.size() << std::endl;
        throw std::invalid_argument("Wrong input size");
    }
    
    std::vector<char> encoded = EncodeFillDataBitsInHamming(data);
    
    for (int parity_pos = 1; parity_pos <= n_ - 1; parity_pos <<= 1) {
        if (!IsPowerOfTwo(parity_pos)) continue;
        
        bool parity = false;
        
        for (int bit_pos = 1; bit_pos <= n_ - 1; ++bit_pos) {
            if (bit_pos == parity_pos) continue;
            
            if (Covering(parity_pos, bit_pos)) {
                bool value = GetBitFromBlock(encoded, bit_pos - 1);
                parity ^= value;
            }
        }
        
        SetBitInBlock(encoded, parity_pos - 1, parity);
    }
    
    bool overall_parity = CalculateOverallParity(encoded);
    SetBitInBlock(encoded, n_ - 1, overall_parity);
    
    return encoded;
}

std::vector<char> HammingCoder::DecodeFillDataBitsFromHamming(const std::vector<char>& block) const {
    int output_bytes = (k_ + kByte - 1) / kByte;
    std::vector<char> decoded(output_bytes, 0);
    
    int data_bit_index = 0;

    for (int pos = 1; pos <= n_ - 1 && data_bit_index < k_; ++pos) {
        if (!IsPowerOfTwo(pos)) {
            bool value = GetBitFromBlock(block, pos - 1);
            SetBitInBlock(decoded, data_bit_index, value);
            data_bit_index++;
        }
    }
    
    return decoded;
}

int HammingCoder::CalculateSyndrome(const std::vector<char>& block) const {
    int syndrome = 0;
    
    for (int parity_pos = 1; parity_pos <= n_ - 1; parity_pos <<= 1) {
        if (!IsPowerOfTwo(parity_pos)) continue;
        
        bool calculated_parity = false;
        
        for (int bit_pos = 1; bit_pos <= n_ - 1; ++bit_pos) {
            if (bit_pos == parity_pos) continue;
            
            if (Covering(parity_pos, bit_pos)) {
                bool value = GetBitFromBlock(block, bit_pos - 1);
                calculated_parity ^= value;
            }
        }
        
        bool actual_parity = GetBitFromBlock(block, parity_pos - 1);
        
        if (calculated_parity != actual_parity) {
            syndrome |= parity_pos;
        }
    }
    
    return syndrome;
}

std::vector<char> HammingCoder::Fix(const std::vector<char>& block, int error_pos) const {
    std::vector<char> fixed = block;
    
    if (error_pos > 0 && error_pos <= n_) {
        bool current_value = GetBitFromBlock(fixed, error_pos - 1);
        SetBitInBlock(fixed, error_pos - 1, !current_value);
    }
    
    return fixed;
}

std::vector<char> HammingCoder::DecodeBlock(const std::vector<char>& block) const {
    bool overall_parity_from_block = GetBitFromBlock(block, n_ - 1);
    bool calculated_overall_parity = CalculateOverallParity(block);
    bool overall_parity_correct = (overall_parity_from_block == calculated_overall_parity);
    
    int syndrome = CalculateSyndrome(block);
    
    if (syndrome != 0) {
        if (overall_parity_correct) {
            //std::cerr << "Double error detected (syndrome=" << syndrome << ", overall parity OK)" << std::endl;
            return DecodeFillDataBitsFromHamming(block);
        } else {
            std::vector<char> fixed = Fix(block, syndrome);
            
            int fixed_syndrome = CalculateSyndrome(fixed);
            bool fixed_overall_parity = (GetBitFromBlock(fixed, n_ - 1) == CalculateOverallParity(fixed));
            
            if (fixed_syndrome == 0 && fixed_overall_parity) {
                //std::cerr << "Single error corrected at position " << syndrome << std::endl;
                return DecodeFillDataBitsFromHamming(fixed);
            } else {
                //std::cerr << "Correction failed" << std::endl;
                return DecodeFillDataBitsFromHamming(block);
            }
        }
    } else {
        if (!overall_parity_correct) {
            //std::cerr << "Error in overall parity bit only, ignoring" << std::endl;
        }
        return DecodeFillDataBitsFromHamming(block);
    }
}