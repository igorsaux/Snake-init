#include "shell.h"
#include "snake.h"
#include "utils.h"
#include <dirent.h>

void SNK_ls(const char* path) {
    if (!SNK_exists(path)) {
        printf("ls: '%s' does not exist\n", path);

        return;
    }

    if (!SNK_isDir(path)) {
        printf("ls: '%s' is not a directory\n", path);

        return;
    }

    DIR* dir = opendir(path);

    if (dir == nullptr) {
        printf("ls: failed to open '%s': %s\n", path, strerror(errno));

        return;
    }

    struct dirent  entry;
    struct dirent* result = nullptr;

    while (1) {
        if (readdir_r(dir, &entry, &result) != 0) {
            printf("ls: failed to read '%s': %s\n", path, strerror(errno));

            return;
        }

        if (result == nullptr)
            break;

        printf("%s\n", entry.d_name);
    }

    if (closedir(dir) != 0)
        printf("ls: failed to close '%s': %s", path, strerror(errno));
}

void SNK_cat(const char* path) {
    ASSERT(path != nullptr);

    if (!SNK_exists(path)) {
        printf("cat: '%s' does not exist\n", path);

        return;
    }

    if (SNK_isDir(path)) {
        printf("cat: '%s' is not a file\n", path);

        return;
    }

    const int f = open(path, O_RDONLY);

    if (f < 0) {
        printf("cat: failed to open '%s': %s\n", path, strerror(errno));

        return;
    }

    lseek(f, 0, SEEK_SET);
    const size_t f_size = lseek(f, 0, SEEK_END);

    if (f_size <= 0) {
        if (close(f) != 0)
            printf("cat: failed to close '%s': %s\n", path, strerror(errno));

        return;
    }

    lseek(f, 0, SEEK_SET);

    bool is_binary = false;

    while (1) {
        char          buf[512];
        const ssize_t bytes = read(f, buf, sizeof(buf));

        if (bytes < 0) {
            printf("cat: failed to read '%s': %s\n", path, strerror(errno));

            return;
        }

        if (bytes <= 0)
            break;

        if (!is_binary) {
            for (size_t i = 0; i < bytes; i++) {
                if (buf[i] == '\0') {
                    is_binary = true;
                    break;
                }
            }
        }

        if (is_binary) {
            for (size_t i = 0; i < bytes; i++)
                printf("\\%02x", buf[i]);

            printf("\n");
        } else
            for (size_t i = 0; i < bytes; i++)
                printf("%c", buf[i]);

        if (bytes < sizeof(buf))
            break;
    }

    if (close(f) != 0)
        printf("cat: failed to close '%s': %s\n", path, strerror(errno));
}

void SNK_write(const char* msg, const char* path) {
    ASSERT(msg != nullptr);
    ASSERT(path != nullptr);

    if (!SNK_exists(path)) {
        printf("write: '%s' does not exist\n", path);

        return;
    }

    if (SNK_isDir(path)) {
        printf("write: '%s' is not a file\n", path);

        return;
    }

    const int f = open(path, O_WRONLY);

    if (f < 0) {
        printf("write: failed to open '%s': %s\n", path, strerror(errno));

        return;
    }

    if (write(f, msg, strlen(msg)) != strlen(msg))
        printf("write: failed to write '%s': %s\n", path, strerror(errno));

    if (close(f) != 0)
        printf("write: failed to close '%s': %s\n", path, strerror(errno));
}

void SNK_cp(const char* src, const char* dst) {
    if (SNK_exists(dst)) {
        if (SNK_isDir(dst)) {
            printf("cp: '%s' is not a file\n", dst);

            return;
        }
    }

    const int src_f = open(src, O_RDONLY);

    if (src_f < 0) {
        printf("cp: failed to open '%s': %s\n", src, strerror(errno));

        return;
    }

    const int dst_f = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (dst_f < 0) {
        printf("cp: failed to open '%s': %s\n", dst, strerror(errno));

        goto cleanup;
    }

    for (;;) {
        char buf[512] = {};

        const ssize_t bytes = read(src_f, buf, sizeof(buf));

        if (bytes < 0) {
            printf("cp: failed to read '%s': %s\n", src, strerror(errno));

            goto cleanup;
        }

        const ssize_t written = write(dst_f, buf, bytes);

        if (written < 0) {
            printf("cp: failed to write '%s': %s\n", dst, strerror(errno));

            goto cleanup;
        }

        if (bytes < sizeof(buf)) {
            break;
        }
    }

cleanup:
    close(src_f);

    if (dst_f >= 0)
        close(dst_f);
}

void _SNK_help() {
    printf("Available commands:\n"
           "cat <PATH> - print content of the file\n"
           "ls [PATH] - print contents of the directory\n"
           "cp <SRC> <DST> - copy file\n"
           "write <PATH> <MSG> - write message to the file\n"
           "quit/q - exit the shell and reboot\n"
           "snake - run the snake game\n"
           "help - print this message\n");
}

void SNK_shell() {
    printf("Welcome to SnakeOS shell!\n");
    _SNK_help();

    while (1) {
        printf("$ ");
        fflush(stdout);

        char   buf[512];
        size_t bytes = read(STDIN_FILENO, buf, sizeof(buf));

        if (bytes <= 1)
            continue;

        buf[bytes - 1] = '\0';
        bytes -= 1;

        if (strncmp(buf, "q", bytes) == 0 || strncmp(buf, "quit", bytes) == 0) {
            break;
        }

        if (strncmp(buf, "cat", 3) == 0) {
            char* delim = strchr(buf, ' ');

            if (delim == nullptr) {
                printf("cat: missing argument\n");

                continue;
            }

            *delim = '\0';

            SNK_cat(delim + 1);

            continue;
        }

        if (strncmp(buf, "ls", 2) == 0) {
            char* delim = strchr(buf, ' ');

            if (delim == nullptr) {
                SNK_ls(".");

                continue;
            }

            *delim = '\0';

            SNK_ls(delim + 1);

            continue;
        }

        if (strncmp(buf, "cp", 2) == 0) {
            char* src_delim = strchr(buf, ' ');

            if (src_delim == nullptr) {
                printf("cp: missing argument\n");

                continue;
            }

            char* dst_delim = strchr(src_delim + 1, ' ');

            if (dst_delim == nullptr) {
                printf("cp: missing argument\n");

                continue;
            }

            *src_delim = '\0';
            *dst_delim = '\0';

            SNK_cp(src_delim + 1, dst_delim + 1);

            continue;
        }

        if (strncmp(buf, "write", 5) == 0) {
            char* path_delim = strchr(buf, ' ');

            if (path_delim == nullptr) {
                printf("write: missing argument\n");

                continue;
            }

            char* msg_delim = strchr(path_delim + 1, ' ');

            if (msg_delim == nullptr) {
                printf("write: missing argument\n");

                continue;
            }

            *path_delim = '\0';
            *msg_delim  = '\0';

            SNK_write(msg_delim + 1, path_delim + 1);

            continue;
        }

        if (strncmp(buf, "snake", 5) == 0) {
            SNK_snake();

            continue;
        }

        if (strncmp(buf, "help", 4) == 0) {
            _SNK_help();

            continue;
        }

        printf("Unknown command: '%s'\n", buf);
    }
}
