#include "drm.h"
#include "drm_connector.h"
#include "utils.h"
#include "vec.h"
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <stdio.h>

#define _SNK_DRM_ASSERT(drm)                                                                                           \
    ASSERT(drm != nullptr);                                                                                            \
    ASSERT(drm->_fd != -1)

long _SNK_DRM_ioctl(const SNK_DRM* drm, const unsigned long request, void* arg) {
    _SNK_DRM_ASSERT(drm);

    return ioctl(drm->_fd, request, arg);
}

typedef struct {
    __u32 connector_id;
    __u32 crtc_id;
    __u32 encoder_id;
} _SNK_DRM_Resources;

void _SNK_DRM_Resources_dump(const _SNK_DRM_Resources* resources) {
    ASSERT(resources != nullptr);

    printf("** Resources **\n");
    printf("- Connector ID: %d\n", resources->connector_id);
    printf("- CRTC ID: %d\n", resources->crtc_id);
    printf("- Encoder ID: %d\n", resources->encoder_id);
}

typedef struct {
    __u32 id;
    // struct drm_mode_modeinfo mode;
    SNK_Vec modes;
    // __u32
    SNK_Vec encoders;
    // __u32
    SNK_Vec props;
    // __u64
    SNK_Vec prop_values;
} _SNK_DRM_Connector;

_SNK_DRM_Connector _SNK_DRM_Connector_new(const __u32 id, const size_t mode_count, const size_t encoder_count,
                                          const size_t prop_count) {
    return (_SNK_DRM_Connector){
        .id          = id,
        .modes       = SNK_Vec_new(mode_count, sizeof(struct drm_mode_modeinfo), true),
        .encoders    = SNK_Vec_new(encoder_count, sizeof(__u32), true),
        .props       = SNK_Vec_new(prop_count, sizeof(__u32), true),
        .prop_values = SNK_Vec_new(prop_count, sizeof(__u64), true),
    };
}

void _SNK_DRM_Connnector_dump(const _SNK_DRM_Connector* connector) {
    ASSERT(connector != nullptr);

    printf("** Connector %d **\n", connector->id);
    printf("- Modes: %lu\n", SNK_Vec_size(&connector->modes));
    printf("- Encoders: %lu\n", SNK_Vec_size(&connector->encoders));
    printf("\t[");

    for (size_t i = 0; i < SNK_Vec_size(&connector->encoders); i++) {
        printf(" %d", *(__u32*)SNK_Vec_at(&connector->encoders, i));
    }

    printf(" ]\n");

    printf("- Props: %lu\n", SNK_Vec_size(&connector->props));
}

void _SNK_DRM_Connector_free(_SNK_DRM_Connector* connector) {
    ASSERT(connector != nullptr);

    SNK_Vec_free(&connector->modes);
    SNK_Vec_free(&connector->encoders);
    SNK_Vec_free(&connector->props);
    SNK_Vec_free(&connector->prop_values);
}

typedef struct {
    __u32 handle;
    __u32 pitch;
    __u32 size;
} _SNK_DRM_DumbBuffer;

void _SNK_DRM_DumbBuffer_dump(const _SNK_DRM_DumbBuffer* buffer) {
    ASSERT(buffer != nullptr);

    printf("** Dumb Buffer **\n");
    printf("- Handle: %d\n", buffer->handle);
    printf("- Pitch: %d\n", buffer->pitch);
    printf("- Size: %d\n", buffer->size);
}

typedef struct {
    _SNK_DRM_Resources        resources;
    _SNK_DRM_Connector        connector;
    struct drm_mode_modeinfo* preferred_mode;
    _SNK_DRM_DumbBuffer       dumb_buffer;
    __u32                     fb_id;
    void*                     data;
} _SNK_DRM_Data;

bool SNK_DRM_open(const char* device, SNK_DRM* drm) {
    ASSERT(drm != nullptr);
    ASSERT(device != nullptr);

    const int fd = open(device, O_RDWR);

    if (fd < 0)
        return false;

    drm->_fd = fd;

    return true;
}

bool SNK_DRM_initFB(SNK_DRM* drm) {
    _SNK_DRM_ASSERT(drm);

    do {
        struct drm_get_cap cap = {
            .capability = DRM_CAP_DUMB_BUFFER,
        };

        if (_SNK_DRM_ioctl(drm, DRM_IOCTL_GET_CAP, &cap) == -1) {
            printf("Failed to get DRM capability: %s\n", strerror(errno));

            return false;
        }

        if (cap.value == 0) {
            printf("dumb buffer capability not supported\n");

            return false;
        }
    } while (false);

    drm->_data = malloc(sizeof(_SNK_DRM_Data));

    if (drm->_data == nullptr)
        SNK_crash("Failed to allocate DRM data");

    const auto data = (_SNK_DRM_Data*)drm->_data;

    do {
        struct drm_mode_card_res card_res = {};

        if (_SNK_DRM_ioctl(drm, DRM_IOCTL_MODE_GETRESOURCES, &card_res) == -1) {
            printf("Failed to get DRM resources: %s\n", strerror(errno));

            return false;
        }

        if (card_res.count_connectors != 1) {
            printf("Expected 1 connector, got %d\n", card_res.count_connectors);

            return false;
        }

        if (card_res.count_crtcs != 1) {
            printf("Expected 1 CRTC, got %d\n", card_res.count_crtcs);

            return false;
        }

        if (card_res.count_encoders != 1) {
            printf("Expected 1 encoder, got %d\n", card_res.count_encoders);

            return false;
        }

        card_res.connector_id_ptr = (__u64)&data->resources.connector_id;
        card_res.crtc_id_ptr      = (__u64)&data->resources.crtc_id;
        card_res.encoder_id_ptr   = (__u64)&data->resources.encoder_id;

        if (_SNK_DRM_ioctl(drm, DRM_IOCTL_MODE_GETRESOURCES, &card_res) == -1) {
            printf("Failed to get DRM resources\n");

            return false;
        }

        if (data->resources.connector_id == 0) {
            printf("Failed to get connector ID\n");

            return false;
        }

        if (data->resources.crtc_id == 0) {
            printf("Failed to get CRTC ID\n");

            return false;
        }

        if (data->resources.encoder_id == 0) {
            printf("Failed to get encoder ID\n");

            return false;
        }
    } while (false);

    _SNK_DRM_Resources_dump(&data->resources);

    do {
        struct drm_mode_modeinfo      temp          = {};
        struct drm_mode_get_connector get_connector = {
            .count_modes  = 1,
            .connector_id = data->resources.connector_id,
            .modes_ptr    = (__u64)&temp,
        };

        if (_SNK_DRM_ioctl(drm, DRM_IOCTL_MODE_GETCONNECTOR, &get_connector) == -1) {
            printf("Failed to get connector meta data: %s\n", strerror(errno));

            return false;
        }

        data->connector = _SNK_DRM_Connector_new(data->resources.connector_id, get_connector.count_modes,
                                                 get_connector.count_encoders, get_connector.count_props);

        get_connector.modes_ptr       = (__u64)SNK_Vec_data(&data->connector.modes);
        get_connector.encoders_ptr    = (__u64)SNK_Vec_data(&data->connector.encoders);
        get_connector.props_ptr       = (__u64)SNK_Vec_data(&data->connector.props);
        get_connector.prop_values_ptr = (__u64)SNK_Vec_data(&data->connector.prop_values);

        if (_SNK_DRM_ioctl(drm, DRM_IOCTL_MODE_GETCONNECTOR, &get_connector) == -1) {
            printf("Failed to get connector meta data: %s\n", strerror(errno));

            return false;
        }

        if (get_connector.connection != connector_status_connected) {
            printf("Connector %d is not connected\n", data->resources.connector_id);

            return false;
        }

        for (size_t i = 0; i < SNK_Vec_size(&data->connector.modes); i++) {
            const auto mode = (struct drm_mode_modeinfo*)SNK_Vec_at(&data->connector.modes, i);

            ASSERT(mode != nullptr);

            if (mode->type & DRM_MODE_TYPE_PREFERRED) {
                data->preferred_mode = mode;

                break;
            }
        }
    } while (false);

    printf("Using connector info:\n");
    _SNK_DRM_Connnector_dump(&data->connector);

    printf("Preferred mode: %s\n", data->preferred_mode->name);

    do {
        struct drm_mode_create_dumb create_dumb = {
            .width  = data->preferred_mode->hdisplay,
            .height = data->preferred_mode->vdisplay,
            .bpp    = 32,
        };

        if (_SNK_DRM_ioctl(drm, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb) == -1) {
            printf("Failed to create dumb buffer: %s\n", strerror(errno));

            return false;
        }

        data->dumb_buffer.handle = create_dumb.handle;
        data->dumb_buffer.pitch  = create_dumb.pitch;
        data->dumb_buffer.size   = create_dumb.size;
    } while (false);

    printf("Using dumb buffer info:\n");
    _SNK_DRM_DumbBuffer_dump(&data->dumb_buffer);

    do {
        struct drm_mode_fb_cmd fb_cmd = {
            .width  = data->preferred_mode->hdisplay,
            .height = data->preferred_mode->vdisplay,
            .pitch  = data->dumb_buffer.pitch,
            .bpp    = 32,
            .depth  = 24,
            .handle = data->dumb_buffer.handle,
        };

        if (_SNK_DRM_ioctl(drm, DRM_IOCTL_MODE_ADDFB, &fb_cmd) == -1) {
            printf("Failed to add framebuffer: %s\n", strerror(errno));

            return false;
        }

        data->fb_id = fb_cmd.fb_id;
    } while (false);

    printf("Using framebuffer ID: %d\n", data->fb_id);

    do {
        struct drm_mode_map_dumb map_dumb = {
            .handle = data->dumb_buffer.handle,
        };

        if (_SNK_DRM_ioctl(drm, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb) == -1) {
            printf("Failed to map dumb buffer: %s\n", strerror(errno));

            return false;
        }

        printf("Framebuffer offset: %llu\n", map_dumb.offset);

        data->data =
            mmap(nullptr, data->dumb_buffer.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm->_fd, (off_t)map_dumb.offset);

        if (data->data == MAP_FAILED) {
            printf("Failed to mmap dumb buffer: %s\n", strerror(errno));

            return false;
        }

        printf("Framebuffer data: %p\n", data->data);
    } while (false);

    memset(data->data, 0, data->dumb_buffer.size);

    printf("DRM is ready\n");

    return true;
}

bool SNK_DRM_refresh(const SNK_DRM* drm) {
    _SNK_DRM_ASSERT(drm);
    ASSERT(drm->_data != nullptr);

    const auto data = (_SNK_DRM_Data*)drm->_data;

    ASSERT(data->preferred_mode != nullptr);
    ASSERT(data->fb_id != 0);
    ASSERT(data->resources.crtc_id != 0);

    struct drm_mode_crtc crtc = {
        .set_connectors_ptr = (__u64)&data->resources.connector_id,
        .count_connectors   = 1,
        .crtc_id            = data->resources.crtc_id,
        .fb_id              = data->fb_id,
        .x                  = 0,
        .y                  = 0,
        .mode_valid         = 1,
        .mode               = *data->preferred_mode,
    };

    if (_SNK_DRM_ioctl(drm, DRM_IOCTL_MODE_SETCRTC, &crtc) == -1) {
        printf("Failed to set CRTC: %s\n", strerror(errno));

        return false;
    }

    return true;
}

SNK_DRM_FBInfo SNK_DRM_getFBInfo(const SNK_DRM* drm) {
    _SNK_DRM_ASSERT(drm);
    ASSERT(drm->_data != nullptr);

    const auto data = (_SNK_DRM_Data*)drm->_data;

    ASSERT(data->data != nullptr);
    ASSERT(data->preferred_mode != nullptr);
    ASSERT(data->dumb_buffer.size != 0);

    return (SNK_DRM_FBInfo){
        .width  = data->preferred_mode->hdisplay,
        .height = data->preferred_mode->vdisplay,
        .stride = data->dumb_buffer.pitch,
        .size   = data->dumb_buffer.size,
        .buffer = data->data,
    };
}

void SNK_DRM_free(SNK_DRM* drm) {
    _SNK_DRM_ASSERT(drm);

    if (drm->_data != nullptr) {
        const auto data = (_SNK_DRM_Data*)drm->_data;

        _SNK_DRM_Connector_free(&data->connector);

        if (data->fb_id != 0) {
            if (_SNK_DRM_ioctl(drm, DRM_IOCTL_MODE_RMFB, &data->fb_id) == -1)
                printf("Failed to remove framebuffer: %s\n", strerror(errno));
        }

        if (data->dumb_buffer.handle != 0) {
            struct drm_mode_destroy_dumb destroy_dumb = {
                .handle = data->dumb_buffer.handle,
            };

            if (_SNK_DRM_ioctl(drm, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_dumb) == -1)
                printf("Failed to destroy dumb buffer: %s\n", strerror(errno));
        }

        free(drm->_data);
        drm->_data = nullptr;
    }

    if (drm->_fd >= 0)
        close(drm->_fd);

    drm->_fd = -1;
}
