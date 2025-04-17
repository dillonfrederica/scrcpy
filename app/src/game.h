#ifndef SC_GAME_H
#define SC_GAME_H

#include "common.h"

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>

struct sc_game_map_data
{
    uint8_t type;        // 1:点击; 2:拖动
    float x;             // 坐标(百分比)
    float y;             // 坐标(百分比)
    float x0;            // 换算后的坐标
    float y0;            // 换算后的坐标
    float x1;            // 实际坐标(随机生成)
    float y1;            // 实际坐标(随机生成)
    uint8_t keyactions;  // 0b XXXXDSAW
    float w;             // 宽度(百分比)
    float h;             // 高度(百分比)
    float w0;            // 换算后的宽度
    float h0;            // 换算后的高度
    Uint32 key_duration; // 按键持续时间
    Uint32 key_interval; // 同一按键间隔
    Uint32 timestamp;    // 时间戳
    uint64_t pointer_id; // 多点触控的指针ID
    int times;           // 重复次数
};

union sc_game_map_input
{
    SDL_Keycode key_code;
    Uint8 mouse_button; // 1鼠标左键; 2鼠标右键; 3鼠标中键
};

struct sc_game_map
{
    // 1 键盘输入
    // 2 鼠标点击
    // 3 鼠标移动
    // 4 鼠标滚轮
    // 5 WASD
    uint8_t type;

    union sc_game_map_input input;
    SDL_Surface *img;
    SDL_Texture *img_texture;
    uint8_t alpha; // 透明度(0~255)

    struct sc_game_map_data *data;
    uint8_t size;
};

struct sc_game
{
    struct sc_game_map *maps;
    uint8_t size;
    bool is_gaming;
    bool show;
};

/// @brief 从文件读取输入映射
/// @param path 文件路径
/// @return 输入映射
struct sc_game *sc_game_init(const char *path);

void sc_game_on(struct sc_game *game, SDL_Window *win);

void sc_game_display(const struct sc_game *game, SDL_Renderer *renderer);

#endif