#pragma once

#include "hamming.h"

#include <fstream>
#include <cstdint>
#include <vector>
#include <cstring>

const size_t kMaxFileNameSize = 256;
const size_t kEncodedBytes = 2;
const int kOne = 1;


struct FileMeta {
    char file_name[kMaxFileNameSize];
    uint64_t original_size;
    uint64_t encoded_size;
    uint64_t data_offset;
    uint64_t next_meta_offset;
};


struct Header {
    char archive_name[kMaxFileNameSize];
    uint64_t encoding_k;
    uint64_t files_count;
    uint64_t last_offset;
};


#pragma pack(pop)

const size_t kMetaSize = sizeof(FileMeta);
const size_t kEncodedMetaSize = kMetaSize * kEncodedBytes;

const size_t kHeaderSize = sizeof(Header);
const size_t kEncodedHeaderSize = kHeaderSize * kEncodedBytes;

class DataCoder {
    private:
        int encoding_k_;
        HammingCoder hamming_;

    public:
        explicit DataCoder(int data_bytes);

        FileMeta EncodeFile(std::ifstream& input_file, std::ofstream& output_file) const;

        void DecodeFile(std::ifstream& input_file, std::ofstream& output_file, uint32_t start_size) const;
};

class MetaCoder {
    private:
        HammingCoder hamming_;

        std::vector<char> SerializeMeta(const FileMeta& meta) const;
        std::vector<char> SerializeHeader(const Header& header) const;

        FileMeta DeserializeMeta(const std::vector<char>& data) const;
        Header DeserializeHeader(const std::vector<char>& data) const;

    public:
        MetaCoder();

        void EncodeMeta(std::ofstream& output, const FileMeta& meta) const;
        void EncodeHeader(std::ofstream& output, const Header& header) const;

        FileMeta DecodeMeta(std::ifstream& input) const;
        Header DecodeHeader(std::ifstream& input) const;
};