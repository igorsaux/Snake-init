#pragma once

#define ARRSIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define ASSERT(x)                                                                                                      \
    if (!(x))                                                                                                          \
    SNK_crash("Assertion failed: '%s' on %s:%s", #x, __FILE__, __LINE__)

[[noreturn]]
void SNK_crash(const char* msg, ...);

bool SNK_exists(const char* path);

bool SNK_isDir(const char* path);
