#include "input.h"
#include "utils.h"
#include <stdio.h>

void SNK_InputEvent_dump(const SNK_InputEvent* ev) {
    printf("** Input Event **\n");
    printf("- Type: ");

    switch (ev->type) {
    case EV_SYN:
        printf("EV_SYN\n");
        break;

    case EV_KEY:
        printf("EV_KEY\n");

        break;
    case EV_REL:
        printf("EV_REL\n");

        break;
    case EV_ABS:
        printf("EV_ABS\n");

        break;
    case EV_MSC:
        printf("EV_MSC\n");

        break;
    case EV_SW:
        printf("EV_SW\n");

        break;
    case EV_LED:
        printf("EV_LED\n");

        break;
    case EV_SND:
        printf("EV_SND\n");

        break;
    case EV_REP:
        printf("EV_REP\n");

        break;
    case EV_FF:
        printf("EV_FF\n");

        break;
    case EV_PWR:
        printf("EV_PWR\n");

        break;
    case EV_FF_STATUS:
        printf("EV_FF_STATUS\n");

        break;
    case EV_MAX:
        printf("EV_MAX\n");

        break;
    case EV_CNT:
        printf("EV_CNT\n");

        break;
    default:
        printf("Unknown\n");

        break;
    }

    printf("- Code: %d\n", ev->code);
    printf("- Value: %d\n", ev->value);
}

bool SNK_InputDevice_open(SNK_InputDevice* device, const char* path) {
    ASSERT(device != nullptr);
    ASSERT(path != nullptr);

    const int fd = open(path, O_RDONLY);

    if (fd < 0)
        return false;

    device->_fd = fd;

    return true;
}

bool SNK_InputDevice_poll(const SNK_InputDevice* device, SNK_InputEvent* ev) {
    ASSERT(device != nullptr);
    ASSERT(device->_fd >= 0);

    struct timeval tv = {
        .tv_sec  = 0,
        .tv_usec = 0,
    };

    fd_set read_set;

    FD_ZERO(&read_set);
    FD_SET(device->_fd, &read_set);

    const int ret = select(device->_fd + 1, &read_set, nullptr, nullptr, &tv);

    if (ret < 0)
        SNK_crash("Failed to poll input device: %s", strerror(errno));

    if (ret == 0)
        return false;

    const ssize_t bytes = read(device->_fd, ev, sizeof(SNK_InputEvent));

    ASSERT(bytes != 0);

    if (bytes < 0)
        SNK_crash("Failed to read input device: %s", strerror(errno));

    return true;
}

void SNK_InputDevice_close(SNK_InputDevice* device) {
    ASSERT(device != nullptr);

    if (device->_fd >= 0) {
        close(device->_fd);
        device->_fd = -1;
    }
}

SNK_Keyboard SNK_Keyboard_new(const SNK_InputDevice device) {
    return (SNK_Keyboard){
        ._device       = device,
        ._old_keyboard = {false},
        ._keyboard     = {false},
    };
}

bool SNK_Keyboard_wasPressed(const SNK_Keyboard* keyboard, const uint16_t key) {
    ASSERT(keyboard != nullptr);
    ASSERT(key < KEY_CNT);

    return keyboard->_old_keyboard[key] && !keyboard->_keyboard[key];
}

bool SNK_Keyboard_isPressed(const SNK_Keyboard* keyboard, const uint16_t key) {
    ASSERT(keyboard != nullptr);
    ASSERT(key < KEY_CNT);

    return keyboard->_keyboard[key];
}

void SNK_Keyboard_update(SNK_Keyboard* keyboard) {
    ASSERT(keyboard != nullptr);

    memset(keyboard->_old_keyboard, false, sizeof(keyboard->_old_keyboard));

    SNK_InputEvent ev = {};

    while (SNK_InputDevice_poll(&keyboard->_device, &ev)) {
        if (ev.type != EV_KEY)
            continue;

        ASSERT(ev.code < KEY_CNT);

        // SNK_InputEvent_dump(&ev);

        keyboard->_old_keyboard[ev.code] = keyboard->_keyboard[ev.code];
        keyboard->_keyboard[ev.code]     = ev.value == 0 ? false : true;
    }
}

void SNK_Keyboard_free(SNK_Keyboard* keyboard) {
    ASSERT(keyboard != nullptr);

    SNK_InputDevice_close(&keyboard->_device);

    *keyboard = (SNK_Keyboard){};
}
