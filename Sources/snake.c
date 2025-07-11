#include "snake.h"
#include "drm.h"
#include "input.h"
#include "utils.h"
#include "vec.h"
#include <stdio.h>

uint64_t _SNK_rand() {
    const int fd = open("/dev/urandom", O_RDONLY);

    if (fd == -1)
        SNK_crash("Failed to open /dev/urandom: %s", strerror(errno));

    uint64_t rand = 0;

    if (read(fd, &rand, sizeof(rand)) != sizeof(rand))
        SNK_crash("Failed to read from /dev/urandom: %s", strerror(errno));

    close(fd);

    return rand;
}

uint64_t _SNK_randRange(const uint64_t min, const uint64_t max) { return min + (_SNK_rand() % (max - min)); }

typedef struct {
    int64_t x;
    int64_t y;
} _SNK_IVec2;

_SNK_IVec2 _SNK_IVec2_mult(const _SNK_IVec2 a, const _SNK_IVec2 b) { return (_SNK_IVec2){a.x * b.x, a.y * b.y}; }

bool _SNK_IVec2_eq(const _SNK_IVec2 a, const _SNK_IVec2 b) { return a.x == b.x && a.y == b.y; }

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} _SNK_RGB;

typedef enum {
    SNK_Direction_Up,
    SNK_Direction_Down,
    SNK_Direction_Left,
    SNK_Direction_Right,
} _SNK_Direction;

typedef struct {
    bool           is_paused;
    bool           quit;
    size_t         score;
    _SNK_IVec2     grid;
    _SNK_IVec2     scale;
    float          move_progress;
    float          move_speed;
    _SNK_Direction direction;
    _SNK_IVec2     snake_head;
    _SNK_IVec2     food;
    // SNK_IVec2
    SNK_Vec snake_body;
} SNK_Game;

const _SNK_RGB  SNK_SNAKE_HEAD_COLOR = {2, 181, 38};
const _SNK_RGB  SNK_SNAKE_BODY_COLOR = {38, 126, 5};
const _SNK_RGB  SNAKE_FOOD_COLOR     = {240, 255, 0};
constexpr float SNK_DELTA_TIME       = 0.033f;

int64_t _SNK_wrap(const int64_t value, const int64_t min, const int64_t max) {
    if (value < min)
        return max - (min - value);

    if (value >= max)
        return min + (value - max);

    return value;
}

bool _SNK_isThereBody(const SNK_Game* game, const _SNK_IVec2 pos, const _SNK_IVec2* target) {
    const _SNK_IVec2 wrapped_pos = {_SNK_wrap(pos.x, 0, game->grid.x), _SNK_wrap(pos.y, 0, game->grid.y)};

    if (target != nullptr)
        return _SNK_IVec2_eq(wrapped_pos, *target);

    for (size_t i = 0; i < SNK_Vec_size(&game->snake_body); i++) {
        const auto body = (_SNK_IVec2*)SNK_Vec_at(&game->snake_body, i);

        ASSERT(body != nullptr);

        if (body->x == pos.x && body->y == pos.y)
            return true;
    }

    return false;
}

_SNK_IVec2 _SNK_move(const _SNK_IVec2 pos, const _SNK_Direction direction) {
    switch (direction) {
    case SNK_Direction_Up:
        return (_SNK_IVec2){pos.x, pos.y - 1};
    case SNK_Direction_Down:
        return (_SNK_IVec2){pos.x, pos.y + 1};
    case SNK_Direction_Left:
        return (_SNK_IVec2){pos.x - 1, pos.y};
    case SNK_Direction_Right:
        return (_SNK_IVec2){pos.x + 1, pos.y};
    default:
        ASSERT(false);
    }
}

void _SNK_tick(SNK_Game* game, SNK_Keyboard* keyboard) {
    ASSERT(game != nullptr);
    ASSERT(keyboard != nullptr);

    SNK_Keyboard_update(keyboard);

    if (SNK_Keyboard_wasPressed(keyboard, KEY_ESC)) {
        game->is_paused = !game->is_paused;

        return;
    }

    if (SNK_Keyboard_isPressed(keyboard, KEY_LEFTCTRL) && SNK_Keyboard_wasPressed(keyboard, KEY_C)) {
        game->quit = true;

        return;
    }

    const _SNK_IVec2* first_body = SNK_Vec_at(&game->snake_body, 0);

    if (SNK_Keyboard_isPressed(keyboard, KEY_W) || SNK_Keyboard_isPressed(keyboard, KEY_UP)) {
        if (!_SNK_isThereBody(game, (_SNK_IVec2){game->snake_head.x, game->snake_head.y - 1}, first_body))
            game->direction = SNK_Direction_Up;
    } else if (SNK_Keyboard_isPressed(keyboard, KEY_S) || SNK_Keyboard_isPressed(keyboard, KEY_DOWN)) {
        if (!_SNK_isThereBody(game, (_SNK_IVec2){game->snake_head.x, game->snake_head.y + 1}, first_body))
            game->direction = SNK_Direction_Down;
    } else if (SNK_Keyboard_isPressed(keyboard, KEY_A) || SNK_Keyboard_isPressed(keyboard, KEY_LEFT)) {
        if (!_SNK_isThereBody(game, (_SNK_IVec2){game->snake_head.x - 1, game->snake_head.y}, first_body))
            game->direction = SNK_Direction_Left;
    } else if (SNK_Keyboard_isPressed(keyboard, KEY_D) || SNK_Keyboard_isPressed(keyboard, KEY_RIGHT)) {
        if (!_SNK_isThereBody(game, (_SNK_IVec2){game->snake_head.x + 1, game->snake_head.y}, first_body))
            game->direction = SNK_Direction_Right;
    }

    if (game->is_paused)
        return;

    if (SNK_Keyboard_isPressed(keyboard, KEY_LEFTSHIFT))
        game->move_progress += game->move_speed * 2.0f * SNK_DELTA_TIME;
    else
        game->move_progress += game->move_speed * SNK_DELTA_TIME;

    if (game->move_progress >= 1.0f) {
        _SNK_IVec2 prev_pos = game->snake_head;

        game->move_progress = 0.0f;
        game->snake_head    = _SNK_move(game->snake_head, game->direction);

        game->snake_head.x = _SNK_wrap(game->snake_head.x, 0, game->grid.x);
        game->snake_head.y = _SNK_wrap(game->snake_head.y, 0, game->grid.y);

        for (size_t i = 0; i < SNK_Vec_size(&game->snake_body); i++) {
            const auto       body      = (_SNK_IVec2*)SNK_Vec_at(&game->snake_body, i);
            const _SNK_IVec2 body_copy = *body;

            ASSERT(body != nullptr);

            if (_SNK_IVec2_eq(*body, game->snake_head)) {
                game->quit = true;

                printf("You lose! Score: %lu\n", game->score);

                return;
            }

            if (_SNK_IVec2_eq(*body, prev_pos)) {
                continue;
            }

            *body    = prev_pos;
            prev_pos = body_copy;
        }
    }

    if (_SNK_IVec2_eq(game->snake_head, game->food)) {
        game->score++;
        game->move_speed += 0.05f;

        const _SNK_IVec2 new_body = {game->snake_head.x, game->snake_head.y};
        SNK_Vec_push(&game->snake_body, &new_body, sizeof(_SNK_IVec2));

        if (SNK_Vec_size(&game->snake_body) + 1 == game->grid.x * game->grid.y) {
            game->quit = true;

            printf("You win! Score: %lu\n", game->score);

            return;
        }

        while (true) {
        spawn_food:
            game->food =
                (_SNK_IVec2){(int64_t)_SNK_randRange(0, game->grid.x), (int64_t)_SNK_randRange(0, game->grid.y)};

            if (_SNK_IVec2_eq(game->food, game->snake_head)) {
                goto spawn_food;
            }

            for (size_t i = 0; i < SNK_Vec_size(&game->snake_body); i++) {
                const auto body = (_SNK_IVec2*)SNK_Vec_at(&game->snake_body, i);

                ASSERT(body != nullptr);

                if (_SNK_IVec2_eq(game->food, *body))
                    goto spawn_food;
            }

            break;
        }
    }
}

void _SNK_drawRect(const _SNK_IVec2 pos, const _SNK_IVec2 size, const _SNK_RGB color, const SNK_DRM_FBInfo fbInfo) {
    for (size_t y = pos.y; y < pos.y + size.y; y++) {
        for (size_t x = pos.x; x < pos.x + size.x; x++) {
            const size_t y_wrapped = _SNK_wrap((int64_t)y, 0, (int64_t)fbInfo.height);
            const size_t x_wrapped = _SNK_wrap((int64_t)x, 0, (int64_t)fbInfo.width);

            const size_t offset = y_wrapped * fbInfo.stride + x_wrapped * 4;

            fbInfo.buffer[offset / 4] = (color.r << 16) | (color.g << 8) | color.b;
        }
    }
}

void _SNK_render(const SNK_Game* game, const SNK_DRM* drm, const SNK_DRM_FBInfo fbInfo) {
    ASSERT(game != nullptr);

    memset(fbInfo.buffer, 0, fbInfo.size);

    _SNK_drawRect(_SNK_IVec2_mult(game->food, game->scale), game->scale, SNAKE_FOOD_COLOR, fbInfo);

    for (size_t i = 0; i < SNK_Vec_size(&game->snake_body); i++) {
        const auto body = (_SNK_IVec2*)SNK_Vec_at(&game->snake_body, i);

        ASSERT(body != nullptr);

        _SNK_drawRect(_SNK_IVec2_mult(*body, game->scale), game->scale, SNK_SNAKE_BODY_COLOR, fbInfo);
    }

    _SNK_drawRect(_SNK_IVec2_mult(game->snake_head, game->scale), game->scale, SNK_SNAKE_HEAD_COLOR, fbInfo);

    if (!SNK_DRM_refresh(drm))
        SNK_crash("Failed to refresh DRM device");
}

void SNK_snake() {
    SNK_DRM drm;

    SNK_switchConsoleTo("/dev/ttyAMA0");
    
    if (!SNK_DRM_open("/dev/dri/card0", &drm)) {
        printf("Failed to open DRM device: %s\n", strerror(errno));

        return;
    }

    if (!SNK_DRM_initFB(&drm)) {
        printf("Failed to initialize framebuffer\n");

        goto cleanup;
    }

    SNK_Keyboard keyboard = {};

    do {
        SNK_InputDevice input = {};

        if (!SNK_InputDevice_open(&input, "/dev/input/event0")) {
            printf("Failed to open input device: %s\n", strerror(errno));

            goto cleanup;
        }

        keyboard = SNK_Keyboard_new(input);
    } while (false);

    const SNK_DRM_FBInfo fbInfo = SNK_DRM_getFBInfo(&drm);

    const _SNK_IVec2 scale = {26, 26};
    const _SNK_IVec2 grid  = {(int64_t)fbInfo.width / scale.x, (int64_t)fbInfo.height / scale.y};
    const _SNK_IVec2 food  = {(int64_t)_SNK_randRange(0, grid.x), (int64_t)_SNK_randRange(0, grid.y)};

    SNK_Game game = {
        .grid       = grid,
        .scale      = scale,
        .direction  = SNK_Direction_Right,
        .move_speed = 1.0f,
        .snake_head = {grid.x / 2, grid.y / 2},
        .snake_body = SNK_Vec_new(64, sizeof(_SNK_IVec2), false),
        .food       = food,
    };

    printf("** Game Info **\n");
    printf("- Grid: %lld x %lld\n", game.grid.x, game.grid.y);
    printf("- Scale: %lld x %lld\n", game.scale.x, game.scale.y);

    while (!game.quit) {
        _SNK_tick(&game, &keyboard);
        _SNK_render(&game, &drm, fbInfo);

        msleep((unsigned int)(SNK_DELTA_TIME * 1000));
    }

cleanup:
    SNK_Keyboard_free(&keyboard);
    SNK_DRM_free(&drm);
    SNK_switchConsoleTo("/dev/tty0");
}
