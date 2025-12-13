#pragma once

#include <cstdlib>

namespace nargparse {

enum Nargs {
    kNargsOptional,
    kNargsRequired,
    kNargsZeroOrMore, 
    kNargsOneOrMore,
};

enum ValueType {
    kValueUnknown = 0,
    kValueInt,
    kValueFloat,
    kValueString
};

struct ArgumentParser {
    char* program_name = nullptr;
    size_t max_arg_size = 0;

    struct Argument {
        char* name = nullptr;
        char* short_name = nullptr;
        char* long_name = nullptr;

        char* error_message = nullptr;
        char* help = nullptr;

        void* storage = nullptr;
        union Validator {
            bool (*int_validator)(const int&);
            bool (*float_validator)(const float&);
            bool (*str_validator)(const char* const&);
            void* raw;
            Validator() : raw(nullptr) {}
        };

        Validator validator;
        

        bool is_flag = false;
        bool found = false;
        bool is_positional = false;
        Nargs nargs = kNargsRequired;

        char** repeated_values = nullptr;
        int repeated_count = 0;
        int repeated_capacity = 0;

        ValueType value_type = kValueUnknown;
    };
    
    Argument* arguments = nullptr;
    int arguments_count = 0;
    int arguments_capacity = 0;

    bool help_request = false;

    int positional_index = 0;
};

ArgumentParser CreateParser(const char* program_name, size_t max_arg_size);

void FreeParser(ArgumentParser& parser);

bool Parse(ArgumentParser& parser, int argc, const char* argv[]);

void AddFlag(ArgumentParser& parser, const char* short_name, const char* long_name,
             bool* value, const char* name, bool default_value);

void AddFlag(ArgumentParser& parser, const char* short_name, const char* long_name,
             bool* value, const char* name);

void AddArgument(ArgumentParser& parser, int* storage, const char* name,
                 Nargs nargs = kNargsRequired,
                 bool (*validator)(const int&) = nullptr, const char* error_message = nullptr);

void AddArgument(ArgumentParser& parser, float* storage, const char* name,
                 Nargs nargs = kNargsRequired,
                 bool (*validator)(const float&) = nullptr, const char* error_message = nullptr);

void AddArgument(ArgumentParser& parser, char* storage, const char* name,
                 Nargs nargs = kNargsRequired,
                 bool (*validator)(const char* const&) = nullptr, const char* error_message = nullptr);

void AddArgument(ArgumentParser& parser, const char* short_name, const char* long_name,
                 int* storage, const char* name,
                 Nargs nargs = kNargsOptional,
                 bool (*validator)(const int&) = nullptr, const char* error_message = nullptr);

void AddArgument(ArgumentParser& parser, const char* short_name, const char* long_name,
                 float* storage, const char* name,
                 Nargs nargs = kNargsOptional,
                 bool (*validator)(const float&) = nullptr, const char* error_message = nullptr);

void AddArgument(ArgumentParser& parser, const char* short_name, const char* long_name,
                 char* storage, const char* name,
                 Nargs nargs = kNargsOptional,
                 bool (*validator)(const char* const&) = nullptr, const char* error_message = nullptr);

void AddArgument(ArgumentParser& parser, void* storage, const char* name,
                 Nargs nargs = kNargsRequired,
                 bool (*validator)(const char* const&) = nullptr, const char* error_message = nullptr);

void AddArgument(ArgumentParser& parser, const char* short_name, const char* long_name,
                 void* storage, const char* name,
                 Nargs nargs = kNargsOptional,
                 bool (*validator)(const char* const&) = nullptr, const char* error_message = nullptr);

void AddHelp(ArgumentParser& parser);

int GetRepeatedCount(ArgumentParser& parser, const char* argument_name);

bool GetRepeated(ArgumentParser& parser, const char* argument_name, int value_index, void* value);

} // nargparse
