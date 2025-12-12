#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

const std::size_t kByte = 8;

class HammingCoder {
private:
    int k_;  // data bits
    int r_;  // par bits
    int n_;  // n + k + 1
    
    void CalculateParameters();
    
    bool IsPowerOfTwo(int x) const;
    bool Covering(int parity_bit, int data_bit) const;
    
    std::vector<char> EncodeFillDataBitsInHamming(const std::vector<char>& block) const;
    std::vector<char> DecodeFillDataBitsFromHamming(const std::vector<char>& block) const;
    
    int CalculateSyndrome(const std::vector<char>& block) const;
    std::vector<char> Fix(const std::vector<char>& block, int index) const;
    bool CalculateOverallParity(const std::vector<char>& block) const;
    
    bool GetBitFromBlock(const std::vector<char>& block, int index) const;
    void SetBitInBlock(std::vector<char>& block, int index, bool value) const;
    
public:
    HammingCoder(int bytes = 1);
    
    int GetN() const { return n_; }
    
    std::vector<char> EncodeBlock(const std::vector<char>& data) const;
    std::vector<char> DecodeBlock(const std::vector<char>& encoded) const;
};