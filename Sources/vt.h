#pragma once

typedef struct {
    int _fd;
} SNK_VT;

SNK_VT SNK_VT_init();

bool SNK_VT_open(SNK_VT* vt, const char* path);

long SNK_VT_ioctl(const SNK_VT* vt, unsigned int cmd, unsigned long arg);

void SNK_VT_setConsoleTo(const SNK_VT* vt);

void SNK_VT_close(SNK_VT* vt);
