#include "vt.h"
#include "utils.h"
#include <stdio.h>

SNK_VT SNK_VT_init() {
    return (SNK_VT){
        ._fd = -1,
    };
}

bool SNK_VT_open(SNK_VT* vt, const char* path) {
    ASSERT(vt != nullptr);
    ASSERT(vt->_fd <= 0);
    ASSERT(path != nullptr);

    const int fd = open(path, O_RDWR);

    if (fd < 0)
        return false;

    vt->_fd = fd;

    return true;
}

long SNK_VT_ioctl(const SNK_VT* vt, unsigned int cmd, unsigned long arg) {
    ASSERT(vt != nullptr);
    ASSERT(vt->_fd >= 0);

    return ioctl(vt->_fd, cmd, arg);
}

void SNK_VT_setConsoleTo(const SNK_VT* vt) {
    ASSERT(vt != nullptr);
    ASSERT(vt->_fd >= 0);

    if (SNK_VT_ioctl(vt, TIOCCONS, 1) != 0)
        SNK_crash("Failed to set TIOCCONS: %s", strerror(errno));

    dup2(vt->_fd, STDIN_FILENO);
}

void SNK_VT_close(SNK_VT* vt) {
    ASSERT(vt != nullptr);
    ASSERT(vt->_fd >= 0);

    close(vt->_fd);
    vt->_fd = -1;
}
