#pragma once

#include <stdint.h>

typedef void* SNK_DRM_Data;

typedef struct {
    int          _fd;
    SNK_DRM_Data _data;
} SNK_DRM;

bool SNK_DRM_open(const char* device, SNK_DRM* drm);

bool SNK_DRM_initFB(SNK_DRM* drm);

bool SNK_DRM_refresh(const SNK_DRM* drm);

typedef struct {
    size_t    width;
    size_t    height;
    size_t    stride;
    size_t    size;
    uint32_t* buffer;
} SNK_DRM_FBInfo;

SNK_DRM_FBInfo SNK_DRM_getFBInfo(const SNK_DRM* drm);

void SNK_DRM_free(SNK_DRM* drm);
