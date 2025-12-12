#include "archiver.h"
#include "argparser.h"

#include <iostream>
#include <string>
#include <vector>

bool validateEncodingK(const int& k) {
    return k >= 1 && k <= 8;
}

bool validatePositiveInt(const int& n) {
    return n > 0;
}

int main(int argc, const char* argv[]) {
    try {
        nargparse::ArgumentParser parser = nargparse::CreateParser("hamarc", 256);
        
        nargparse::AddHelp(parser);
        
        bool create_flag = false;
        bool list_flag = false;
        bool extract_flag = false;
        bool append_flag = false;
        bool delete_flag = false;
        bool concatenate_flag = false;
        
        nargparse::AddFlag(parser, "-c", "--create", &create_flag, "create new archive");
        nargparse::AddFlag(parser, "-l", "--list", &list_flag, "list files in archive");
        nargparse::AddFlag(parser, "-x", "--extract", &extract_flag, "extract files from archive");
        nargparse::AddFlag(parser, "-a", "--append", &append_flag, "append file to archive");
        nargparse::AddFlag(parser, "-d", "--delete", &delete_flag, "delete file from archive");
        nargparse::AddFlag(parser, "-A", "--concatenate", &concatenate_flag, "merge two archives");
        
        char archive_name[256] = {0};
        nargparse::AddArgument(parser, "-f", "--file", archive_name, "archive_name", 
                              nargparse::kNargsOptional, nullptr, "archive file name");
        
        int encoding_k = 3;
        nargparse::AddArgument(parser, "-k", "--encoding", &encoding_k, "encoding_k", 
                              nargparse::kNargsOptional, validateEncodingK, 
                              "encoding parameter k (1-8)");
        

        nargparse::AddArgument(parser, static_cast<void*>(nullptr), "files", 
                              nargparse::kNargsZeroOrMore, nullptr, "file names");
        
        if (!nargparse::Parse(parser, argc, argv)) {
            std::cerr << "Error parsing arguments. Use -h for help.\n";
            nargparse::FreeParser(parser);
            return 1;
        }
        
        if (parser.help_request) {
            std::cout << "Usage: hamarc [OPTIONS] [FILES...]\n"
                      << "Hamarc - error-resistant archiver\n\n"
                      << "Options:\n"
                      << "  -c, --create            Create new archive\n"
                      << "  -f, --file=ARCHNAME     Archive file name\n"
                      << "  -l, --list              List files in archive\n"
                      << "  -x, --extract           Extract files from archive\n"
                      << "  -a, --append            Append file to archive\n"
                      << "  -d, --delete            Delete file from archive\n"
                      << "  -A, --concatenate       Merge two archives\n"
                      << "  -k, --encoding=N        Encoding parameter k (default: 3)\n"
                      << "  -h, --help              Show this help message\n\n"
                      << "Examples:\n"
                      << "  hamarc --create --file=ARCHIVE FILE1 FILE2 FILE3\n"
                      << "  hamarc -l -f ARCHIVE\n"
                      << "  hamarc --concatenate ARCHIVE1 ARCHIVE2 -f ARCHIVE3\n";
            nargparse::FreeParser(parser);
            return 0;
        }
        
        if (!concatenate_flag && std::string(archive_name).empty()) {
            std::cerr << "Error: archive file name is required (-f option)\n";
            nargparse::FreeParser(parser);
            return 1;
        }
        
        HammingArchiver archiver;
        
        std::vector<std::string> files;
        int file_count = nargparse::GetRepeatedCount(parser, "files");
        for (int i = 0; i < file_count; i++) {
            const char* filename = nullptr;
            if (nargparse::GetRepeated(parser, "files", i, &filename)) {
                files.push_back(filename);
            }
        }
        
        if (create_flag) {
            if (files.empty()) {
                std::cerr << "Error: no files specified for creation\n";
                nargparse::FreeParser(parser);
                return 1;
            }
            
            std::cout << "Creating archive '" << archive_name << "' with encoding k=" << encoding_k << "\n";
            archiver.CreateArchive(archive_name, encoding_k);
            
            for (const auto& file : files) {
                std::cout << "Adding file: " << file << "\n";
                archiver.AddFile(archive_name, file);
            }
            
            std::cout << "Archive created successfully\n";
        }
        else if (list_flag) {
            std::cout << "Files in archive '" << archive_name << "':\n";
            archiver.ListOfFiles(archive_name);
            std::cout << "\n";
        }
        else if (extract_flag) {
            if (files.empty()) {
                std::cout << "Extracting all files from archive '" << archive_name << "'\n";
                archiver.ExtractAllFiles(archive_name);
                std::cout << "All files extracted successfully\n";
            } else {
                std::cout << "Extracting specified files from archive '" << archive_name << "'\n";
                for (const auto& file : files) {
                    std::cout << "Extracting: " << file << "\n";
                    archiver.ExtractFile(archive_name, file);
                }
                std::cout << "Files extracted successfully\n";
            }
        }
        else if (append_flag) {
            if (files.empty()) {
                std::cerr << "Error: no files specified for appending\n";
                nargparse::FreeParser(parser);
                return 1;
            }
            
            std::cout << "Appending files to archive '" << archive_name << "'\n";
            for (const auto& file : files) {
                std::cout << "Appending: " << file << "\n";
                archiver.AddFile(archive_name, file);
            }
            std::cout << "Files appended successfully\n";
        }
        else if (delete_flag) {
            if (files.empty()) {
                std::cerr << "Error: no files specified for deletion\n";
                nargparse::FreeParser(parser);
                return 1;
            }
            
            std::cout << "Deleting files from archive '" << archive_name << "'\n";
            for (const auto& file : files) {
                std::cout << "Deleting: " << file << "\n";
                archiver.DeleteFile(archive_name, file);
            }
            std::cout << "Files deleted successfully\n";
        }
        else if (concatenate_flag) {
            if (files.size() != 2) {
                std::cerr << "Error: concatenate requires exactly 2 archive names\n";
                nargparse::FreeParser(parser);
                return 1;
            }
            
            if (std::string(archive_name).empty()) {
                std::cerr << "Error: output archive name required for concatenate (-f option)\n";
                nargparse::FreeParser(parser);
                return 1;
            }
            
            std::cout << "Merging archives '" << files[0] << "' and '" << files[1] 
                      << "' into '" << archive_name << "'\n";
            archiver.MergeTwoArchives(files[0], files[1], archive_name);
            std::cout << "Archives merged successfully\n";
        }
        else {
            std::cerr << "Error: no operation specified. Use -h for help.\n";
            nargparse::FreeParser(parser);
            return 1;
        }
        
        nargparse::FreeParser(parser);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}