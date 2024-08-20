#pragma once

#include <string>

void Print(const std::string& str);
void Print(const char* str);
void ErrorPrint(const std::string& str);
void ErrorPrint(const char* str);

__attribute__((noreturn)) __attribute__((noinline)) void Quit(int32_t err);
__attribute__((noreturn)) __attribute__((noinline)) void ErrorExit(
    const char* /*err*/);
