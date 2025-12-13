#include "argparser.h"
#include <cstring>
#include <cstdlib>
#include <cctype>

#include <iostream>

namespace nargparse {

static char* CopyString(const char* str) {
    if (!str) return nullptr;
    size_t len = strlen(str) + 1;
    char* r = (char*)malloc(len);
    memcpy(r, str, len);
    return r;
}

static bool StringToInt(const char* str, int* out) {
    char* end;
    long value = strtol(str, &end, 10);
    if (end == str || *end != '\0') return false;
    *out = value;
    return true;
}

static bool StringToFloat(const char* str, float* out) {
    char* end;
    float value = strtof(str, &end);
    if (end == str || *end != '\0') return false;
    *out = value;
    return true;
}

static bool AddRepeatValue(ArgumentParser::Argument* argument, const char* value) {
    if (argument->repeated_count == argument->repeated_capacity) {
        int new_capacity = argument->repeated_capacity == 0 ? 4 : argument->repeated_capacity * 2;
        char** nw = (char**)malloc(new_capacity * sizeof(char*));
        if (!nw) return false;
        for (int index = 0; index < argument->repeated_count; ++index) nw[index] = argument->repeated_values[index];
        for (int index = argument->repeated_count; index < new_capacity; ++index) nw[index] = nullptr;
        free(argument->repeated_values);
        argument->repeated_values = nw;
        argument->repeated_capacity = new_capacity;
    }
    argument->repeated_values[argument->repeated_count] = CopyString(value);
    if (!argument->repeated_values[argument->repeated_count]) return false;
    argument->repeated_count++;
    return true;
}

static void FreeRepeatedValues(ArgumentParser::Argument* argument) {
    if (!argument->repeated_values) return;
    for (int index = 0; index < argument->repeated_count; ++index) {
        free(argument->repeated_values[index]);
    }
    free(argument->repeated_values);
    argument->repeated_values = nullptr;
    argument->repeated_count = 0;
    argument->repeated_capacity = 0;
}

static bool CallValidator(ArgumentParser::Argument* argument, const char* value) {
    if (!argument->validator.raw) return true;

    int iv;
    float fv;

    switch (argument->value_type) {
        case kValueInt:
            if (!StringToInt(value, &iv)) return false;
            return ((bool(*)(const int&))argument->validator.int_validator)(iv);

        case kValueFloat:
            if (!StringToFloat(value, &fv)) return false;
            return ((bool(*)(const float&))argument->validator.float_validator)(fv);

        case kValueString:
            return ((bool(*)(const char* const&))argument->validator.str_validator)(value);

        case kValueUnknown:
        default:
            return true;
    }
}

static bool AssignSingleValue(ArgumentParser& parser, ArgumentParser::Argument* argument, const char* value) {
    if (!CallValidator(argument, value)) return false;
    if (!argument->storage) return true;

    switch (argument->value_type) {
        case kValueInt: {
            int iv;
            if (!StringToInt(value, &iv)) return false;
            *static_cast<int*>(argument->storage) = iv;
            return true;
        }

        case kValueFloat: {
            float fv;
            if (!StringToFloat(value, &fv)) return false;
            *static_cast<float*>(argument->storage) = fv;
            return true;
        }

        case kValueString: {
            size_t len = strlen(value);
            if (len >= parser.max_arg_size) return false;
            char* dst = (char*)argument->storage;
            strncpy(dst, value, parser.max_arg_size - 1);
            dst[parser.max_arg_size - 1] = '\0';
            return true;
        }

        case kValueUnknown:
        default:
            return false;
    }
}

static bool CheckCapacityForArguments(ArgumentParser& parser) {
    if (parser.arguments_count < parser.arguments_capacity) return true;
    int new_capacity = parser.arguments_capacity * 2;
    ArgumentParser::Argument* nw = (ArgumentParser::Argument*)realloc(parser.arguments, new_capacity * sizeof(ArgumentParser::Argument));
    if (!nw) return false;

    for (int index = parser.arguments_capacity; index < new_capacity; ++index) {
        nw[index] = ArgumentParser::Argument();
        nw[index].value_type = kValueUnknown;
    }

    parser.arguments = nw;
    parser.arguments_capacity = new_capacity;
    return true;
}

ArgumentParser CreateParser(const char* program_name, size_t max_arg_size) {
    ArgumentParser parser;
    parser.program_name = CopyString(program_name);
    parser.max_arg_size = max_arg_size;
    parser.arguments_count = 0;
    parser.arguments_capacity = 16;
    parser.positional_index = 0;
    parser.help_request = false;

    parser.arguments = (ArgumentParser::Argument*)malloc(parser.arguments_capacity * sizeof(ArgumentParser::Argument));

    for (int index = 0; index < parser.arguments_capacity; ++index) {
        parser.arguments[index] = ArgumentParser::Argument();
        parser.arguments[index].value_type = kValueUnknown;
    }

    return parser;
}

void FreeParser(ArgumentParser& parser) {
    free(parser.program_name);
    if (parser.arguments) {
        for (int index = 0; index < parser.arguments_count; ++index) {
            ArgumentParser::Argument* argument = &parser.arguments[index];
            free(argument->name);
            free(argument->short_name);
            free(argument->long_name);
            free(argument->help);
            free(argument->error_message);
            FreeRepeatedValues(argument);
        }
        free(parser.arguments);
    }
    parser.arguments = nullptr;
    parser.arguments_count = 0;
}

void AddArgument(ArgumentParser& parser, int* storage, const char* name,
                 Nargs nargs, bool (*validator)(const int&), const char* error_message)
{
    if (!CheckCapacityForArguments(parser)) return;
    ArgumentParser::Argument* argument = &parser.arguments[parser.arguments_count++];

    argument->name = CopyString(name);
    argument->storage = storage;
    argument->nargs = nargs;
    argument->validator.int_validator = validator;
    argument->error_message = CopyString(error_message);
    argument->is_flag = false;
    argument->is_positional = true;
    argument->value_type = kValueInt;
}

void AddArgument(ArgumentParser& parser, float* storage, const char* name,
                 Nargs nargs, bool (*validator)(const float&), const char* error_message)
{
    if (!CheckCapacityForArguments(parser)) return;
    ArgumentParser::Argument* argument = &parser.arguments[parser.arguments_count++];

    argument->name = CopyString(name);
    argument->storage = storage;
    argument->nargs = nargs;
    argument->validator.float_validator = validator;
    argument->error_message = CopyString(error_message);
    argument->is_flag = false;
    argument->is_positional = true;
    argument->value_type = kValueFloat;
}

void AddArgument(ArgumentParser& parser, char* storage, const char* name,
                 Nargs nargs, bool (*validator)(const char* const&), const char* error_message)
{
    if (!CheckCapacityForArguments(parser)) return;
    ArgumentParser::Argument* argument = &parser.arguments[parser.arguments_count++];

    argument->name = CopyString(name);
    argument->storage = storage;
    argument->nargs = nargs;
    argument->validator.str_validator = validator;
    argument->error_message = CopyString(error_message);
    argument->is_flag = false;
    argument->is_positional = true;
    argument->value_type = kValueString;
}

void AddArgument(ArgumentParser& parser, const char* sn, const char* ln,
                 int* storage, const char* name,
                 Nargs nargs, bool (*validator)(const int&), const char* error_message)
{
    if (!CheckCapacityForArguments(parser)) return;
    ArgumentParser::Argument* argument = &parser.arguments[parser.arguments_count++];

    argument->short_name = CopyString(sn);
    argument->long_name  = CopyString(ln);
    argument->storage = storage;
    argument->validator.int_validator = validator;
    argument->error_message = CopyString(error_message);
    argument->nargs = nargs;
    argument->is_flag = false;
    argument->is_positional = false;
    argument->value_type = kValueInt;
    argument->name = CopyString(name);
}

void AddArgument(ArgumentParser& parser, const char* sn, const char* ln,
                 float* storage, const char* name,
                 Nargs nargs, bool (*validator)(const float&), const char* error_message)
{
    if (!CheckCapacityForArguments(parser)) return;
    ArgumentParser::Argument* argument = &parser.arguments[parser.arguments_count++];

    argument->short_name = CopyString(sn);
    argument->long_name  = CopyString(ln);
    argument->name = CopyString(name);
    argument->storage = storage;
    argument->validator.float_validator = validator;
    argument->error_message = CopyString(error_message);
    argument->nargs = nargs;
    argument->is_flag = false;
    argument->is_positional = false;
    argument->value_type = kValueFloat;
}

void AddArgument(ArgumentParser& parser, const char* sn, const char* ln,
                 char* storage, const char* name,
                 Nargs nargs, bool (*validator)(const char* const&), const char* error_message)
{
    if (!CheckCapacityForArguments(parser)) return;
    ArgumentParser::Argument* argument = &parser.arguments[parser.arguments_count++];

    argument->short_name = CopyString(sn);
    argument->long_name  = CopyString(ln);
    argument->name = CopyString(name);
    argument->storage = storage;
    argument->validator.str_validator = validator;
    argument->error_message = CopyString(error_message);
    argument->nargs = nargs;
    argument->is_flag = false;
    argument->is_positional = false;
    argument->value_type = kValueString;
}

void AddArgument(ArgumentParser& parser, void* storage, const char* name,
                 Nargs nargs, bool (*validator)(const char* const&), const char* error_message)
{
    if (!CheckCapacityForArguments(parser)) return;
    ArgumentParser::Argument* argument = &parser.arguments[parser.arguments_count++];

    argument->name = CopyString(name);
    argument->storage = storage;
    argument->nargs = nargs;
    argument->validator.str_validator = validator;
    argument->error_message = CopyString(error_message);
    argument->is_flag = false;
    argument->is_positional = true;
    argument->value_type = kValueString;
}

void AddArgument(ArgumentParser& parser, const char* sn, const char* ln,
                 void* storage, const char* name,
                 Nargs nargs, bool (*validator)(const char* const&), const char* error_message)
{
    if (!CheckCapacityForArguments(parser)) return;
    ArgumentParser::Argument* argument = &parser.arguments[parser.arguments_count++];

    argument->short_name = CopyString(sn);
    argument->long_name = CopyString(ln);
    argument->name = CopyString(name);
    argument->storage = storage;
    argument->validator.str_validator = validator;
    argument->error_message = CopyString(error_message);
    argument->nargs = nargs;
    argument->is_flag = false;
    argument->is_positional = false;
    argument->value_type = kValueString;
}

void AddFlag(ArgumentParser& parser, const char* sn, const char* ln,
             bool* value, const char* name, bool default_value)
{
    if (!CheckCapacityForArguments(parser)) return;
    ArgumentParser::Argument* argument = &parser.arguments[parser.arguments_count++];

    argument->short_name = CopyString(sn);
    argument->long_name = CopyString(ln);
    argument->name = CopyString(name);
    argument->storage = value;
    argument->validator.raw = nullptr;
    argument->error_message = nullptr;
    argument->nargs = kNargsOptional;
    argument->is_flag = true;
    argument->is_positional = false;
    argument->value_type = kValueUnknown;

    if (value) *value = default_value;
}

void AddFlag(ArgumentParser& parser, const char* sn, const char* ln,
             bool* value, const char* name)
{
    AddFlag(parser, sn, ln, value, name, false);
}

void AddHelp(ArgumentParser& parser) {
    AddFlag(parser, "-h", "--help", &parser.help_request, "help message");
}

static bool MatchNamed(const char* tok, ArgumentParser::Argument* argument,
                        const char** out, bool* eq)
{
    *out = nullptr;
    *eq = false;

    if (argument->long_name) {
        size_t n = strlen(argument->long_name);
        if (strncmp(tok, argument->long_name, n) == 0) {
            if (tok[n] == '\0') return true;
            if (tok[n] == '=') {
                *out = tok + n + 1;
                *eq = true;
                return true;
            }
        }
    }

    if (argument->short_name) {
        size_t n = strlen(argument->short_name);
        if (strncmp(tok, argument->short_name, n) == 0) {
            if (tok[n] == '\0') return true;
            if (tok[n] == '=') {
                *out = tok + n + 1;
                *eq = true;
                return true;
            }
        }
    }

    return false;
}

static bool CheckNegtive(const char* str) {
    if (*str == '-') {
        str++;  
        
        if (*str == '\0') return false;
        
        char* endptr;
        strtof(str, &endptr); 

        return (endptr != str && *endptr == '\0');
    }
    return false;
}

bool Parse(ArgumentParser& parser, int argc, const char* argv[]) {
    for (int index = 0; index < parser.arguments_count; ++index) {
        ArgumentParser::Argument* argument = &parser.arguments[index];
        argument->found = false;
        FreeRepeatedValues(argument);
    }
    parser.positional_index = 0;

    if (parser.help_request) {
        for (int index = 1; index < argc; ++index)
            if (!strcmp(argv[index], "--help") || !strcmp(argv[index], "-h"))
                std::cout << "NOBODY WILL HELP YOU";
                return true;
    }

    for (int index = 1; index < argc; ++index) {
        const char* tok = argv[index];

        if (tok[0] == '-' && !CheckNegtive(tok)) {
            bool handled = false;
            for (int jndex = 0; jndex < parser.arguments_count; ++jndex) {
                ArgumentParser::Argument* argument = &parser.arguments[jndex];
                const char* eqv = nullptr;
                bool has_eq = false;

                if (!MatchNamed(tok, argument, &eqv, &has_eq)) continue;
                handled = true;

                if (argument->is_flag) {
                    if (argument->storage)
                        *static_cast<bool*>(argument->storage) = true;
                    argument->found = true;
                    break;
                }

                const char* value = nullptr;
                if (has_eq) value = eqv;
                else {
                    if (index + 1 >= argc) return false;
                    value = argv[++index];
                }

                if (argument->nargs == kNargsZeroOrMore || argument->nargs == kNargsOneOrMore) {
                    if (!CallValidator(argument, value)) return false;
                    if (!AddRepeatValue(argument, value)) return false;

                    if (!argument->found && argument->storage) {
                        if (!AssignSingleValue(parser, argument, value)) return false;
                    }

                    argument->found = true;
                } else {
                    if (argument->found) return false;
                    if (!AssignSingleValue(parser, argument, value)) return false;
                    argument->found = true;
                    break;
                }
            }

            if (!handled) return false;
        } else {
            ArgumentParser::Argument* target = nullptr;
            for (int jndex = parser.positional_index; jndex < parser.arguments_count; ++jndex) {
                ArgumentParser::Argument* argument = &parser.arguments[jndex];
                if (!argument->is_positional) continue;
                if ((argument->nargs == kNargsOptional || argument->nargs == kNargsRequired) && argument->found)
                    continue;
                target = argument;
                parser.positional_index = jndex;
                break;
            }

            if (!target) return false;

            if (target->nargs == kNargsZeroOrMore || target->nargs == kNargsOneOrMore) {
                if (!CallValidator(target, tok)) return false;
                if (!AddRepeatValue(target, tok)) return false;
                if (target->repeated_count == 1 && target->storage)
                    if (!AssignSingleValue(parser, target, tok)) return false;
                target->found = true;
            } else {
                if (target->found) return false;
                if (!AssignSingleValue(parser, target, tok)) return false;
                target->found = true;
                parser.positional_index++;
            }
        }
    }

    for (int index = 0; index < parser.arguments_count; ++index) {
        ArgumentParser::Argument* argument = &parser.arguments[index];
        if (argument->nargs == kNargsRequired && !argument->found) return false;
        if (argument->nargs == kNargsOneOrMore && argument->repeated_count == 0) return false;
    }

    return true;
}

int GetRepeatedCount(ArgumentParser& parser, const char* name) {
    for (int index = 0; index < parser.arguments_count; ++index) {
        ArgumentParser::Argument* argument = &parser.arguments[index];
        if (argument->name && strcmp(argument->name, name) == 0)
            return argument->repeated_count;
    }
    return 0;
}

bool GetRepeated(ArgumentParser& parser, const char* name, int value_index, void* out) {
    for (int index = 0; index < parser.arguments_count; ++index) {
        ArgumentParser::Argument* argument = &parser.arguments[index];
        if (argument->name && strcmp(argument->name, name) == 0) {
            if (!argument->repeated_values || value_index < 0 || value_index >= argument->repeated_count)
                return false;

            const char* str = argument->repeated_values[value_index];

            int iv;
            float fv;
            if (StringToInt(str, &iv)) {
                *static_cast<int*>(out) = iv;
                return true;
            }
            if (StringToFloat(str, &fv)) {
                *static_cast<float*>(out) = fv;
                return true;
            }

            *(const char**)out = str;
            return true;
        }
    }
    return false;
}

}
