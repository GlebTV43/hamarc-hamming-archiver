#include "coder.h"
#include "hamming.h"

#include <fstream>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstring>

// DataCoder

DataCoder::DataCoder(int encoding_k) : encoding_k_(encoding_k), hamming_(encoding_k) {}

FileMeta DataCoder::EncodeFile(std::ifstream& input_file, std::ofstream& output_file) const {
    
    FileMeta meta;

    int encoded_size = (hamming_.GetN() + kByte - 1) / kByte;

    std::vector<char> input(encoding_k_, 0);
    std::vector<char> encoded(encoded_size, 0);

    while (input_file.read(input.data(), encoding_k_)) {
        meta.original_size += encoding_k_;
        encoded = hamming_.EncodeBlock(input);
        output_file.write(encoded.data(), encoded_size);
        meta.encoded_size += encoded_size;
    }

    int dop_bytes = input_file.gcount();

    if (dop_bytes) {
        meta.original_size += dop_bytes;
        for (int i = dop_bytes; i < encoding_k_; ++i) {
            input[i] = 0;
        }

        encoded = hamming_.EncodeBlock(input);
        output_file.write(encoded.data(), encoded_size);
        meta.encoded_size += encoded_size;
    }

    return meta;
}

void DataCoder::DecodeFile(std::istream& input_file, std::ostream& output_file, uint32_t start_size) const {
    
    int encoded_size = (hamming_.GetN() + kByte - 1) / kByte;
    
    std::vector<char> input(encoded_size);
    uint32_t new_bytes = 0;
    
    while (input_file.read(input.data(), encoded_size) && new_bytes < start_size) {
 
        std::vector<char> decoded_block = hamming_.DecodeBlock(input);

        size_t bytes_to_write = encoding_k_;
        if (new_bytes + encoding_k_ > start_size) {
            bytes_to_write = start_size - new_bytes;
        }
        
        output_file.write(decoded_block.data(), bytes_to_write);
        new_bytes += bytes_to_write;
        
    }
}

// MetaCoder

MetaCoder::MetaCoder() = default;

std::vector<char> MetaCoder::SerializeMeta(const FileMeta& meta) const {
    std::vector<char> result(kMetaSize);
    std::memcpy(result.data(), &meta, kMetaSize);
    return result;
}

FileMeta MetaCoder::DeserializeMeta(const std::vector<char>& meta) const {
    FileMeta result;
    std::memcpy(&result, meta.data(), kMetaSize);
    return result;
}

std::vector<char> MetaCoder::SerializeHeader(const Header& header) const {
    std::vector<char> result(kHeaderSize);
    std::memcpy(result.data(), &header, kHeaderSize);
    return result;
}

Header MetaCoder::DeserializeHeader(const std::vector<char>& header) const {
    Header result;
    std::memcpy(&result, header.data(), kHeaderSize);
    return result;
}

void MetaCoder::EncodeMeta(std::ofstream& output, const FileMeta& meta) const {
    std::vector<char>meta_vec = SerializeMeta(meta);

    std::vector<char> byte(kOne);
    std::vector<char> encoded;

    for (int index = 0; index < kMetaSize; ++index) {
        byte[0] = meta_vec[index];
        encoded = hamming_.EncodeBlock(byte);
        output.write(encoded.data(), kEncodedBytes);
    }
}

FileMeta MetaCoder::DecodeMeta(std::ifstream& input) const {
    std::vector<char> decoded;
    std::vector<char> encoded(kEncodedBytes);

    for (int index = 0; index < kMetaSize; ++index) {
        input.read(encoded.data(), kEncodedBytes);

        decoded.push_back(hamming_.DecodeBlock(encoded)[0]);
    }

    return DeserializeMeta(decoded);
}

void MetaCoder::EncodeHeader(std::ofstream& output, const Header& header) const {
    std::vector<char>header_vec = SerializeHeader(header);

    std::vector<char> byte(kOne);
    std::vector<char> encoded;

    for (int index = 0; index < kHeaderSize; ++index) {
        byte[0] = header_vec[index];
        encoded = hamming_.EncodeBlock(byte);
        output.write(encoded.data(), kEncodedBytes);
    }
}

Header MetaCoder::DecodeHeader(std::ifstream& input) const {
    std::vector<char> decoded;
    std::vector<char> encoded(kEncodedBytes);

    for (int index = 0; index < kHeaderSize; ++index) {
        input.read(encoded.data(), kEncodedBytes);

        decoded.push_back(hamming_.DecodeBlock(encoded)[0]);
    }

    return DeserializeHeader(decoded);
}
