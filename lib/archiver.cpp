#include "archiver.h"
#include "coder.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <cstdio>

namespace {
    static const std::size_t kCopyBuffer = 64 * 1024;
} // namespace

inline std::string HammingArchiver::ExtractFilename(const std::string &file_path) const {
    std::filesystem::path p(file_path);
    return p.filename().string();
}

HammingArchiver::HammingArchiver() = default;

void HammingArchiver::ArchiveInit(const std::string &archive_path) {
    std::ifstream in(archive_path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("ArchiveInit: unable to open archive: " + archive_path);
    }

    MetaCoder meta_coder;
    in.seekg(0);
    header_ = meta_coder.DecodeHeader(in);

    meta_.clear();
    uint64_t current = kEncodedHeaderSize;
    for (uint64_t i = 0; i < header_.files_count; ++i) {
        in.seekg(static_cast<std::streamoff>(current));
        FileMeta file_meta = meta_coder.DecodeMeta(in);
        meta_.push_back(file_meta);
        current = file_meta.next_meta_offset;
    }
}

int HammingArchiver::FindFileIndex(const std::string &file_name) const {
    for (std::size_t i = 0; i < meta_.size(); ++i) {
        if (std::string(meta_[i].file_name) == file_name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void HammingArchiver::CreateHeader(const std::string &archive_path, const int encoding_k) {
    std::string name = ExtractFilename(archive_path);
    std::memset(header_.archive_name, 0, kMaxFileNameSize);
    std::strncpy(header_.archive_name, name.c_str(), kMaxFileNameSize - 1);
    header_.encoding_k = static_cast<uint64_t>(encoding_k);
    header_.files_count = 0;
    header_.last_offset = kEncodedHeaderSize;

     // std::cerr << "DEBUG CreateHeader: encoding_k = " << encoding_k << ", header_.encoding_k = " << header_.encoding_k << std::endl;
}

void HammingArchiver::CreateArchive(const std::string &archive_path, int encoding_k) {
    // std::cerr << "DEBUG CreateArchive: called with encoding_k = " << encoding_k << std::endl;

    CreateHeader(archive_path, encoding_k);
    meta_.clear();

    std::ofstream out(archive_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "CreateArchive: failed to create archive '" << archive_path << "'";
        return;
    }

    MetaCoder meta_coder;
    meta_coder.EncodeHeader(out, header_);

    
}

void HammingArchiver::DeleteDublicate(const std::string &archive_path, const std::string &file_name) {
    int idx = FindFileIndex(file_name);
    if (idx == -1) {
        return;
    }
    DeleteFile(archive_path, meta_[idx].file_name);
    ArchiveInit(archive_path);
}

void HammingArchiver::AddFile(const std::string &archive_path, const std::string &file_path) {
    ArchiveInit(archive_path);

    // std::cerr << "DEBUG AddFile: header_.encoding_k = " << header_.encoding_k << std::endl;


    std::string file_name = ExtractFilename(file_path);
    DeleteDublicate(archive_path, file_name);

    std::fstream archive(archive_path, std::ios::binary | std::ios::in | std::ios::out);
    if (!archive) {
        std::cerr << "AddFile: cannot open archive for update: " << archive_path << "'";
        return;
    }

    MetaCoder meta_coder;
    DataCoder data_coder(static_cast<int>(header_.encoding_k));
    //std::cerr << "DEBUG: Created DataCoder with encoding_k = " << static_cast<int>(header_.encoding_k) << std::endl;

    archive.seekp(0, std::ios::end);
    uint64_t meta_pos = static_cast<uint64_t>(archive.tellp());

    FileMeta placeholder;
    std::memset(&placeholder, 0, sizeof(placeholder));
    std::strncpy(placeholder.file_name, file_name.c_str(), kMaxFileNameSize - 1);
    meta_coder.EncodeMeta(reinterpret_cast<std::ofstream &>(archive), placeholder);

    uint64_t data_start = static_cast<uint64_t>(archive.tellp());

    std::ifstream in_file(file_path, std::ios::binary);
    if (!in_file) {
        std::cerr << "AddFile: cannot open input file: " << file_path << "'";
        return;
    }

    FileMeta result_meta = data_coder.EncodeFile(in_file, reinterpret_cast<std::ofstream &>(archive));

    uint64_t data_end = static_cast<uint64_t>(archive.tellp());

    FileMeta final_meta = placeholder;
    final_meta.original_size = result_meta.original_size;
    final_meta.encoded_size = result_meta.encoded_size;
    final_meta.data_offset = data_start;
    final_meta.next_meta_offset = data_end;

    archive.seekp(static_cast<std::streamoff>(meta_pos));
    meta_coder.EncodeMeta(reinterpret_cast<std::ofstream &>(archive), final_meta);

    header_.files_count = header_.files_count + 1;
    header_.last_offset = data_end;

    archive.seekp(0);
    meta_coder.EncodeHeader(reinterpret_cast<std::ofstream &>(archive), header_);

    meta_.push_back(final_meta);
}

void HammingArchiver::ExtractFile(const std::string &archive_path, const std::string &file_name) {
    ArchiveInit(archive_path);
    
    std::ifstream archive(archive_path, std::ios::binary);
    if (!archive) {
        std::cerr << "ExtractFile: cannot open archive: '" << archive_path << "'";
        return;
    }
    
    std::string target_filename = std::filesystem::path(file_name).filename().string();
    bool found = false;
    
    for (std::size_t i = 0; i < meta_.size(); ++i) {
        const FileMeta& file_meta = meta_[i];
        
        if (std::string(file_meta.file_name) == target_filename) {
            found = true;
            
            archive.clear();
            archive.seekg(static_cast<std::streamoff>(file_meta.data_offset));
            
            if (!archive) {
                std::cerr << "ExtractFile: failed to seek to data offset for file '" 
                          << file_meta.file_name << "'";
                continue;
            }
            
            std::ofstream out(file_meta.file_name, std::ios::binary | std::ios::trunc);
            if (!out) {
                std::cerr << "ExtractFile: cannot create output file: '" 
                          << file_meta.file_name << "'";
                continue;
            }
            
            DataCoder decoder(static_cast<int>(header_.encoding_k));
            decoder.DecodeFile(archive, out, file_meta.original_size);
            
            out.flush();
            if (!out) {
                std::cerr << "ExtractFile: failed to write file '" 
                          << file_meta.file_name << "'";
            }
            
            break;
        }
    }
    
    if (!found) {
        std::cerr << "ExtractFile: file '" << target_filename << "' not found in archive";
    }
}

void HammingArchiver::ExtractAllFiles(const std::string &archive_path) {
    ArchiveInit(archive_path);
    
    std::ifstream archive(archive_path, std::ios::binary);
    if (!archive) {
        std::cerr << "ExtractAllFiles: cannot open archive: '" << archive_path << "'";
        return;
    }
    
    DataCoder decoder(static_cast<int>(header_.encoding_k));
    
    for (std::size_t i = 0; i < meta_.size(); ++i) {
        const FileMeta& file_meta = meta_[i];
        
        archive.clear();
        archive.seekg(static_cast<std::streamoff>(file_meta.data_offset));
        
        if (!archive) {
            std::cerr << "ExtractAllFiles: failed to seek to data offset for file '" 
                      << file_meta.file_name << "'";
            continue;
        }
        
        std::ofstream out(file_meta.file_name, std::ios::binary | std::ios::trunc);
        if (!out) {
            std::cerr << "ExtractAllFiles: failed to create '" 
                      << file_meta.file_name << "'";
            continue;
        }
        
        decoder.DecodeFile(archive, out, file_meta.original_size);
        
        out.flush();
        if (!out) {
            std::cerr << "ExtractAllFiles: failed to write file '" 
                      << file_meta.file_name << "'";
        }
    }
}

void HammingArchiver::ListOfFiles(const std::string &archive_path) {
    ArchiveInit(archive_path);
    for (std::size_t i = 0; i < meta_.size(); ++i) {
        std::cout << meta_[i].file_name << "'";
    }
}

void HammingArchiver::DeleteFile(const std::string &archive_path, const std::string file_path) {
    ArchiveInit(archive_path);

    std::string file_name = ExtractFilename(file_path);
    int found = -1;
    for (std::size_t i = 0; i < meta_.size(); ++i) {
        if (file_name == std::string(meta_[i].file_name)) {
            found = static_cast<int>(i);
            break;
        }
    }
    if (found == -1) {
        std::cerr << "DeleteFile: '" << file_name << "' not found";
        return;
    }

    FileMeta removed = meta_[static_cast<std::size_t>(found)];
    uint64_t removed_block = kEncodedMetaSize + removed.encoded_size;

    std::ifstream in(archive_path, std::ios::binary);
    std::string tmp = archive_path + ".tmp";
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);

    MetaCoder meta_coder;
    Header new_header = header_; 
    new_header.files_count = header_.files_count - 1;
    new_header.last_offset = header_.last_offset - removed_block;
    meta_coder.EncodeHeader(out, new_header);

    bool after_removed = false;
    std::vector<char> buffer(kCopyBuffer);

    for (std::size_t i = 0; i < meta_.size(); ++i) {
        FileMeta file_meta = meta_[i];
        if (static_cast<int>(i) == found) {
            after_removed = true;
            continue;
        }

        FileMeta adjusted = file_meta;
        if (after_removed) {
            adjusted.data_offset = file_meta.data_offset - removed_block;
            adjusted.next_meta_offset = file_meta.next_meta_offset - removed_block;
        }

        meta_coder.EncodeMeta(out, adjusted);

        in.seekg(static_cast<std::streamoff>(file_meta.data_offset));
        uint64_t remaining = file_meta.encoded_size;
        while (remaining) {
            std::size_t to_read = static_cast<std::size_t>(std::min<uint64_t>(remaining, buffer.size()));
            in.read(buffer.data(), static_cast<std::streamsize>(to_read));
            out.write(buffer.data(), static_cast<std::streamsize>(to_read));
            remaining -= to_read;
        }
    }

    in.close();
    out.close();
    std::remove(archive_path.c_str());
    std::rename(tmp.c_str(), archive_path.c_str());
}

void HammingArchiver::MergeTwoArchives(const std::string &archive_path1, const std::string &archive_path2, const std::string &merged_archive_path) {
    HammingArchiver first;
    HammingArchiver second;
    first.ArchiveInit(archive_path1);
    second.ArchiveInit(archive_path2);

    CreateArchive(merged_archive_path, static_cast<int>(first.header_.encoding_k));

    for (std::size_t i = 0; i < first.meta_.size(); ++i) {
        FileMeta file_meta = first.meta_[i];
        first.ExtractFile(archive_path1, file_meta.file_name);
        AddFile(merged_archive_path, file_meta.file_name);
        std::remove(file_meta.file_name);
    }

    for (std::size_t i = 0; i < second.meta_.size(); ++i) {
        FileMeta file_meta = second.meta_[i];
        second.ExtractFile(archive_path2, file_meta.file_name);
        AddFile(merged_archive_path, file_meta.file_name);
        std::remove(file_meta.file_name);
    }
}
