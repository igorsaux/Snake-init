#include "shell.h"
#include "utils.h"
#include "vt.h"
#include <linux/reboot.h>
#include <stdio.h>

void _SNK_mountFS() {
    printf("Mounting vfs...\n");

    if (mount("devtmpfs", "/dev", "devtmpfs", 0, nullptr) != 0)
        SNK_crash("Failed to mount /dev: %s", strerror(errno));

    if (mount("sysfs", "/sys", "sysfs", 0, nullptr) != 0)
        SNK_crash("Failed to mount /sys: %s", strerror(errno));

    if (mount("proc", "/proc", "proc", 0, nullptr) != 0)
        SNK_crash("Failed to mount /proc: %s", strerror(errno));

    if (mount("tmpfs", "/tmp", "tmpfs", 0, nullptr) != 0)
        SNK_crash("Failed to mount /tmp: %s", strerror(errno));
}

int main() {
    printf("-- Starting SnakeOS --\n");

    _SNK_mountFS();
    SNK_shell();

    reboot(LINUX_REBOOT_CMD_RESTART);
}
