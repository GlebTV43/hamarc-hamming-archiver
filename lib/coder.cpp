#include "coder.h"
#include "hamming.h"

#include <fstream>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstring>

// DataCoder

DataCoder::DataCoder(int encoding_k) : encoding_k_(encoding_k), hamming_(encoding_k) {}

FileMeta DataCoder::EncodeFile(std::istream& input_file, std::ostream& output_file) const {
    
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
    std::vector<char> meta_bytes(kMetaSize, 0);
    int index = 0;

    for (int i = sizeof(meta.original_size) - 1; i >= 0; --i) {
        meta_bytes[index] = (meta.original_size >> (i * 8)) & 0xFF;
        ++index;
    }
    for (int i = sizeof(meta.encoded_size) - 1; i >= 0; --i) {
        meta_bytes[index] = (meta.encoded_size >> (i * 8)) & 0xFF;
        ++index;
    }
    for (int i = sizeof(meta.data_offset) - 1; i >= 0; --i) {
        meta_bytes[index] = (meta.data_offset >> (i * 8)) & 0xFF;
        ++index;
    }
    for (int i = sizeof(meta.next_meta_offset) - 1; i >= 0; --i) {
        meta_bytes[index] = (meta.next_meta_offset >> (i * 8)) & 0xFF;
        ++index;
    }
    for (int i = 0; i < sizeof(meta.file_name); ++i) {
        meta_bytes[index] = meta.file_name[i];
        ++index;
    }
    return meta_bytes;
}

FileMeta MetaCoder::DeserializeMeta(const std::vector<char>& bytes) const {
    FileMeta meta;
    int index = 0;
    uint8_t new_bytes = bytes[0];
    
    meta.original_size = 0;
    for (int i = sizeof(meta.original_size) - 1; i >= 0; --i) {
        meta.original_size |= (static_cast<uint64_t>(static_cast<uint8_t>(bytes[index])) << (i * 8));
        ++index;
    }
    meta.encoded_size = 0;
    for (int i = sizeof(meta.encoded_size) - 1; i >= 0; --i) {
        meta.encoded_size |= (static_cast<uint64_t>(static_cast<uint8_t>(bytes[index])) << (i * 8));
        ++index;
    }
    meta.data_offset = 0;
    for (int i = sizeof(meta.data_offset) - 1; i >= 0; --i) {
        meta.data_offset |= (static_cast<uint64_t>(static_cast<uint8_t>(bytes[index])) << (i * 8));
        ++index;
    }
    meta.next_meta_offset = 0;
    for (int i = sizeof(meta.next_meta_offset) - 1; i >= 0; --i) {
        meta.next_meta_offset |= (static_cast<uint64_t>(static_cast<uint8_t>(bytes[index])) << (i * 8));
        ++index;
    }
    for (int i = 0; i < sizeof(meta.file_name); ++i) {
        meta.file_name[i] = bytes[index];
        ++index;
    }
    return meta;
}

std::vector<char> MetaCoder::SerializeHeader(const Header& header) const {
    std::vector<char> header_bytes(kHeaderSize, 0);
    int index = 0;

    for (int i = 0; i < sizeof(header.archive_name); ++i) {
        header_bytes[index] = header.archive_name[i];
        ++index;
    }
    for (int i = sizeof(header.files_count) - 1; i >= 0; --i) {
        header_bytes[index] = (header.files_count >> (i * 8)) & 0xFF;
        ++index;
    }
    for (int i = sizeof(header.last_offset) - 1; i >= 0; --i) {
        header_bytes[index] = (header.last_offset >> (i * 8)) & 0xFF;
        ++index;
    }
    for (int i = sizeof(header.encoding_k) - 1; i >= 0; --i) {
        header_bytes[index] = (header.encoding_k >> (i * 8)) & 0xFF;
        ++index;
    }
    return header_bytes;
}

Header MetaCoder::DeserializeHeader(const std::vector<char>& bytes) const {
    Header header;
    int index = 0;
    
    for (int i = 0; i < sizeof(header.archive_name); ++i) {
        header.archive_name[i] = bytes[index];
        ++index;
    }
    header.files_count = 0;
    for (int i = sizeof(header.files_count) - 1; i >= 0; --i) {
        header.files_count |= (static_cast<uint32_t>(static_cast<uint8_t>(bytes[index])) << (i * 8));
        ++index;
    }
    header.last_offset = 0;
    for (int i = sizeof(header.last_offset) - 1; i >= 0; --i) {
        header.last_offset |= (static_cast<uint64_t>(static_cast<uint8_t>(bytes[index])) << (i * 8));
        ++index;
    }
    header.encoding_k = 0;
    for (int i = sizeof(header.encoding_k) - 1; i >= 0; --i) {
        header.encoding_k |= (static_cast<int32_t>(static_cast<uint8_t>(bytes[index])) << (i * 8));
        ++index;
    }
    
    return header;
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
