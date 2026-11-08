// The checks' verdict, shared by the steps: one line per thing checked, PASS
// or FAIL, then the step's result in a frame: SOLVED or NOT SOLVED. In colour
// when the output is a terminal (not on Windows), plain text otherwise.
#pragma once

#include <array>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "common/checks/todos.h"

#ifndef _WIN32
#include <unistd.h>
#endif

namespace verdict_detail {

inline bool colour() {
#ifdef _WIN32
    return false;
#else
    static const bool k_on = isatty(fileno(stdout)) != 0;
    return k_on;
#endif
}

constexpr const char* k_green = "\033[1;32m";
constexpr const char* k_red = "\033[1;31m";
constexpr const char* k_yellow = "\033[1;33m";
constexpr const char* k_reset = "\033[0m";

// Every FAIL so far, for the result line.
inline std::vector<std::string>& failures() {
    static std::vector<std::string> messages;
    return messages;
}

// "TODO 1", "TODO 1 and 2", "TODO 2 or 3" at the start of `text`, or empty.
inline std::string todo_in(const std::string& text) {
    if (text.rfind("TODO ", 0) != 0) { return {}; }
    size_t end = 5;
    while (end < text.size() && (std::isdigit(static_cast<unsigned char>(text[end])) != 0)) { ++end; }
    for (const char* joint : {" and ", " or "}) {
        const std::string with(joint);
        if (text.compare(end, with.size(), with) == 0 && end + with.size() < text.size() &&
            (std::isdigit(static_cast<unsigned char>(text[end + with.size()])) != 0)) {
            end += with.size();
            while (end < text.size() && (std::isdigit(static_cast<unsigned char>(text[end])) != 0)) { ++end; }
        }
    }
    return end > 5 ? text.substr(0, end) : std::string{};
}

// "  PASS  " and the message; a message over several lines stays indented.
// Returns the message.
inline std::string line(const char* word, const char* code, const char* format, va_list args) {
    std::array<char, 1024> text{};
    std::vsnprintf(text.data(), text.size(), format, args);
    std::string indented;
    for (const char* c = text.data(); *c != '\0'; ++c) {
        indented += *c;
        if (*c == '\n' && c[1] != '\0') { indented += "        "; }
    }
    if (colour()) {
        std::printf("  %s%s%s  %s\n", code, word, k_reset, indented.c_str());
    } else {
        std::printf("  %s  %s\n", word, indented.c_str());
    }
    return text.data();
}

}  // namespace verdict_detail

// The compiler checks the arguments against the format, as for printf.
#if defined(__GNUC__) || defined(__clang__)
#define WORKSHOP_PRINTF_FORMAT __attribute__((format(printf, 1, 2)))
#else
#define WORKSHOP_PRINTF_FORMAT
#endif

// One thing that is right.
WORKSHOP_PRINTF_FORMAT inline void pass(const char* format, ...) {
    va_list args;
    va_start(args, format);
    verdict_detail::line("PASS", verdict_detail::k_green, format, args);
    va_end(args);
}

// One thing that is wrong, and what to do about it.
WORKSHOP_PRINTF_FORMAT inline void fail(const char* format, ...) {
    va_list args;
    va_start(args, format);
    const std::string text = verdict_detail::line("FAIL", verdict_detail::k_red, format, args);
    va_end(args);
    verdict_detail::failures().push_back(text);
}

// Something the checks could not look at, and why.
WORKSHOP_PRINTF_FORMAT inline void note(const char* format, ...) {
    va_list args;
    va_start(args, format);
    verdict_detail::line("NOTE", verdict_detail::k_yellow, format, args);
    va_end(args);
}

// The step's result in a frame, after the lines above. Returns `code`, what
// main exits with: 0 is solved.
// @step: the step's number, or k_bonus
// @code: what the checks returned
// @note: added to SOLVED, e.g. what was not checked
inline int step_result(int step, int code, const char* note = nullptr) {
    const char* rule = "==============================================================";
    std::string text = step == k_bonus ? std::string("BONUS") : "STEP " + std::to_string(step);
    if (code == 0) {
        text += " SOLVED";
        if (note != nullptr) { text += std::string(", ") + note; }
    } else {
        // The first FAIL that names a TODO; else the TODO behind the first FAIL
        std::string todo;
        for (const std::string& message : verdict_detail::failures()) {
            todo = verdict_detail::todo_in(message);
            if (!todo.empty()) { break; }
        }
        if (todo.empty() && !verdict_detail::failures().empty()) {
            todo = todo_for(step, verdict_detail::failures().front());
        }
        if (todo.empty()) {
            text += " NOT SOLVED: see FAIL above, and check your TODOs";
        } else {
            const bool several = todo.find(" and ") != std::string::npos;
            text += " NOT SOLVED: " + todo + (several ? " are" : " is") + " not right yet, see FAIL above";
        }
    }
    const bool on = verdict_detail::colour();
    std::printf("\n%s\n  %s%s%s\n%s\n",
                rule,
                on ? (code == 0 ? verdict_detail::k_green : verdict_detail::k_red) : "",
                text.c_str(),
                on ? verdict_detail::k_reset : "",
                rule);
    return code;
}

// What a check threw: a FAIL line with its message, then NOT SOLVED.
inline int step_failed(int step, const std::exception& error) {
    fail("%s", error.what());
    return step_result(step, 1);
}
