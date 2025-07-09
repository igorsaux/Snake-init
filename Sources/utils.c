#include "utils.h"
#include <stdio.h>

[[noreturn]]
void SNK_crash(const char* msg, ...) {
    printf("[SnakeOS] crash: ");

    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");

    printf("Rebooting in 5 seconds...\n");

    sleep(5);
    reboot(LINUX_REBOOT_CMD_POWER_OFF);
}

bool SNK_exists(const char* path) {
    ASSERT(path != nullptr);

    struct stat st;

    if (stat(path, &st) != 0)
        return false;

    return true;
}

bool SNK_isDir(const char* path) {
    ASSERT(path != nullptr);

    struct stat st;

    if (stat(path, &st) != 0)
        return false;

    return S_ISDIR(st.st_mode);
}
