#pragma once

#include "hamming.h"
#include "coder.h"

#include <vector>
#include <string>
#include <cstdint>

class HammingArchiver {
private:
    Header header_;
    std::vector<FileMeta> meta_;

    void ArchiveInit(const std::string& archive_path);
    int FindFileIndex(const std::string& file_name) const;
    void CreateHeader(const std::string& archive_path, const int encoding_k);
    std::string ExtractFilename(const std::string& file_path) const;
    void DeleteDublicate(const std::string& archive_path, const std::string& file_name);
    
public:
    HammingArchiver();

    void CreateArchive(const std::string& archive_path, int encoding_k);
    void AddFile(const std::string& archive_path, const std::string& file_path);
    void DeleteFile(const std::string& archive_path, const std::string file_path);
    void ExtractFile(const std::string& archive_path, const std::string file_path);
    void ExtractAllFiles(const std::string& archive_path);
    void ListOfFiles(const std::string& archive_path);
    void MergeTwoArchives(const std::string& archive_path1, const std::string& archive_path2, const std::string& new_merged_archive_path);
};