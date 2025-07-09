#pragma once

#include <linux/input-event-codes.h>
#include <stdio.h>

typedef struct {
    struct timeval time;
    __u16          type;
    __u16          code;
    __s32          value;
} SNK_InputEvent;

void SNK_InputEvent_dump(const SNK_InputEvent* ev);

typedef struct {
    int _fd;
} SNK_InputDevice;

bool SNK_InputDevice_open(SNK_InputDevice* device, const char* path);

bool SNK_InputDevice_poll(const SNK_InputDevice* device, SNK_InputEvent* ev);

void SNK_InputDevice_close(SNK_InputDevice* device);

typedef struct {
    SNK_InputDevice _device;
    bool            _old_keyboard[KEY_CNT];
    bool            _keyboard[KEY_CNT];
} SNK_Keyboard;

SNK_Keyboard SNK_Keyboard_new(const SNK_InputDevice device);

bool SNK_Keyboard_wasPressed(const SNK_Keyboard* keyboard, const uint16_t key);

bool SNK_Keyboard_isPressed(const SNK_Keyboard* keyboard, const uint16_t key);

void SNK_Keyboard_update(SNK_Keyboard* keyboard);

void SNK_Keyboard_free(SNK_Keyboard* keyboard);
