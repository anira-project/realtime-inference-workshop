// read_json: a JSON file as text, with its comments removed, for anira's
// from_json. The exercise files carry hints as // and /* */ comments; anira's
// parser takes plain JSON only, so they are dropped here. Text inside strings
// stays as it is. Main thread only: it reads a file and allocates.
#pragma once

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

// The file name for messages: the part after the last slash.
inline std::string json_file_name(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

// The two mistakes a blank invites, named before anira sees the file:
//   TODOn, unquoted, still in place of a number
//   a number written in quotes, "2048", which JSON reads as text
// @throws std::runtime_error naming the file, the line and the fix
inline void check_blanks(const std::string& json, const std::string& path) {
    size_t line = 1;
    bool in_string = false;
    size_t string_start = 0;
    for (size_t i = 0; i < json.size(); ++i) {
        const char c = json[i];
        if (c == '\n') { ++line; }
        if (in_string) {
            if (c == '\\') { ++i; continue; }
            if (c == '"') {
                in_string = false;
                const std::string text = json.substr(string_start + 1, i - string_start - 1);
                const bool digits = !text.empty() && text.find_first_not_of("0123456789") == std::string::npos;
                if (digits) {
                    throw std::runtime_error(json_file_name(path) + ", line " + std::to_string(line) + ": \"" + text +
                                             "\" is text; write the number without quotes: " + text);
                }
            }
        } else if (c == '"') {
            in_string = true;
            string_start = i;
        } else if (json.compare(i, 4, "TODO") == 0) {
            size_t end = i + 4;
            while (end < json.size() && std::isdigit(static_cast<unsigned char>(json[end]))) { ++end; }
            throw std::runtime_error(json_file_name(path) + ", line " + std::to_string(line) + ": " +
                                     json.substr(i, end - i) + " is still open: replace it with the value");
        }
    }
}

// @throws std::runtime_error if the file cannot be read, or a blank is still
// open (check_blanks)
inline std::string read_json(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) { throw std::runtime_error("cannot read " + path); }
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string text = buffer.str();

    std::string json;
    json.reserve(text.size());
    bool in_string = false;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (in_string) {
            json += c;
            if (c == '\\' && i + 1 < text.size()) { json += text[++i]; }
            else if (c == '"') { in_string = false; }
        } else if (c == '"') {
            in_string = true;
            json += c;
        } else if (c == '/' && i + 1 < text.size() && text[i + 1] == '/') {
            while (i < text.size() && text[i] != '\n') { ++i; }   // to the end of the line
            if (i < text.size()) { json += '\n'; }
        } else if (c == '/' && i + 1 < text.size() && text[i + 1] == '*') {
            i += 2;
            while (i + 1 < text.size() && !(text[i] == '*' && text[i + 1] == '/')) { ++i; }
            ++i;                                                   // past the closing */
        } else {
            json += c;
        }
    }
    check_blanks(json, path);
    return json;
}
