#include "game.h"

#include <cjson/cJSON.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL_timer.h>

#include "shortcut_mod.h"
#include "input_manager.h"
#include "screen.h"

#define SETBIT(x, y) ((x) |= (1 << y))
#define CLRBIT(x, y) ((x) &= ~(1 << y))

static inline char *_read_file(const char *filename)
{
    FILE *file = NULL;
    long length = 0;
    char *content = NULL;
    size_t read_chars = 0;

    /* open in read binary mode */
    file = fopen(filename, "rb");
    if (file == NULL)
    {
        goto cleanup;
    }

    /* get the length */
    if (fseek(file, 0, SEEK_END) != 0)
    {
        goto cleanup;
    }
    length = ftell(file);
    if (length < 0)
    {
        goto cleanup;
    }
    if (fseek(file, 0, SEEK_SET) != 0)
    {
        goto cleanup;
    }

    /* allocate content buffer */
    content = (char *)malloc((size_t)length + sizeof(""));
    if (content == NULL)
    {
        goto cleanup;
    }

    /* read the file into memory */
    read_chars = fread(content, sizeof(char), (size_t)length, file);
    if ((long)read_chars != length)
    {
        free(content);
        content = NULL;
        goto cleanup;
    }
    content[read_chars] = '\0';

cleanup:
    if (file != NULL)
    {
        fclose(file);
    }

    return content;
}

static inline void sc_game_map_free(struct sc_game_map *map)
{
    for (int i = 0; i < map->size; i++)
    {
        if (map->img != NULL)
        {
            SDL_FreeSurface(map->img);
        }
        if (map->img_texture != NULL)
        {
            SDL_DestroyTexture(map->img_texture);
        }
        free(&map->data[i]);
    }
    free(map);
}

static inline void sc_game_free(struct sc_game *game)
{
    for (int i = 0; i < game->size; i++)
    {
        sc_game_map_free(&game->maps[i]);
    }
    free(game);
}

struct sc_game *sc_game_init(const char *path)
{
    if (!path)
    {
        goto fail3;
    }

    char *content = _read_file(path);
    if (!content)
    {
        goto fail3;
    }

    cJSON *root = cJSON_Parse(content);
    free(content);
    if (!root)
    {
        goto fail2;
    }

    // 返回值
    struct sc_game *game = malloc(sizeof(struct sc_game_map));

    game->size = cJSON_GetArraySize(root);
    if (game->size <= 0)
    {
        goto fail2;
    }

    if (!IMG_Init(IMG_INIT_PNG))
    {
        goto fail2;
    }

    game->maps = malloc(game->size * sizeof(struct sc_game_map));
    cJSON *tmp, *item, *data, *data_item, *key_code, *mouse_button;
    int i, j;
    uint64_t k = 0;
    for (i = 0; i < game->size; i++)
    {
        item = cJSON_GetArrayItem(root, i);
        if (!item)
        {
            goto fail1;
        }

        game->maps[i].type = cJSON_GetObjectItem(item, "type")->valueint;
        key_code = cJSON_GetObjectItem(item, "key_code");
        if (key_code)
        {
            game->maps[i].input.key_code = key_code->valueint;
        }
        mouse_button = cJSON_GetObjectItem(item, "mouse_button");
        if (mouse_button)
        {
            game->maps[i].input.mouse_button = mouse_button->valueint;
        }

        data = cJSON_GetObjectItem(item, "data");
        game->maps[i].size = cJSON_GetArraySize(data);
        if (game->maps[i].size <= 0)
        {
            goto fail1;
        }

        tmp = cJSON_GetObjectItem(item, "alpha");
        if (tmp)
        {
            game->maps[i].alpha = tmp->valueint;
        }
        else
        {
            game->maps[i].alpha = 255;
        }

        game->maps[i].data = malloc(game->maps[i].size * sizeof(struct sc_game_map_data));
        for (j = 0; j < game->maps[i].size; j++)
        {
            data_item = cJSON_GetArrayItem(data, j);
            if (!data_item)
            {
                continue;
            }
            tmp = cJSON_GetObjectItem(data_item, "x");
            if (tmp)
            {
                game->maps[i].data[j].x = tmp->valuedouble;
            }
            tmp = cJSON_GetObjectItem(data_item, "y");
            if (tmp)
            {
                game->maps[i].data[j].y = tmp->valuedouble;
            }
            tmp = cJSON_GetObjectItem(data_item, "type");
            if (tmp)
            {
                game->maps[i].data[j].type = tmp->valueint;
            }
            tmp = cJSON_GetObjectItem(data_item, "w");
            if (tmp)
            {
                game->maps[i].data[j].w = tmp->valuedouble;
            }
            tmp = cJSON_GetObjectItem(data_item, "h");
            if (tmp)
            {
                game->maps[i].data[j].h = tmp->valuedouble;
            }
            tmp = cJSON_GetObjectItem(data_item, "key_duration");
            if (tmp)
            {
                game->maps[i].data[j].key_duration = tmp->valueint;
            }
            tmp = cJSON_GetObjectItem(data_item, "key_interval");
            if (tmp)
            {
                game->maps[i].data[j].key_interval = tmp->valueint;
            }
            game->maps[i].data[j].x1 = -1;
            game->maps[i].data[j].y1 = -1;
            game->maps[i].data[j].times = 0;
            game->maps[i].data[j].timestamp = 0;
            game->maps[i].data[j].pointer_id = k++;
        }
        game->maps[i].data->keyactions = 0b00000000;

        if (game->maps[i].type == 1)
        {
            switch (game->maps[i].input.key_code)
            {
            case SDLK_SPACE:

                game->maps[i].img = IMG_Load("assets/game/key_space.png");
                break;

            default:
                printf("未知按键 %d\n", game->maps[i].input.key_code);
                goto fail1;
            }
            if (!game->maps[i].img)
            {
                printf("1 图片加载失败 - %s\n", SDL_GetError());
                goto fail1;
            }
        }
        else if (game->maps[i].type == 2)
        {
            switch (game->maps[i].input.mouse_button)
            {
            case 1: // 鼠标左键
                game->maps[i].img = IMG_Load("assets/game/mouse_left.png");
                break;

            case 3: // 鼠标右键
                game->maps[i].img = IMG_Load("assets/game/mouse_right.png");
                break;

            default:
                printf("未知鼠标按键 %d\n", game->maps[i].input.mouse_button);
                break;
            }
            if (!game->maps[i].img)
            {
                printf("2 图片加载失败 - %s\n", SDL_GetError());
                goto fail1;
            }
        }
        else if (game->maps[i].type == 3)
        {
            game->maps[i].img = IMG_Load("assets/game/movement.png");
            if (!game->maps[i].img)
            {
                printf("3 图片加载失败 - %s\n", SDL_GetError());
                goto fail1;
            }
        }
        else if (game->maps[i].type == 5)
        {
            game->maps[i].img = IMG_Load("assets/game/key_wasd.png");
            if (!game->maps[i].img)
            {
                printf("5 图片加载失败 - %s\n", SDL_GetError());
                goto fail1;
            }
        }
        else
        {
            game->maps[i].img = NULL;
        }

        game->maps[i].img_texture = NULL;
    }
    game->is_gaming = false;
    game->show = true;

    cJSON_Delete(root);

    // touch 需要随机数
    srand((unsigned)time(NULL));

    return game;

fail1:
    sc_game_free(game);
fail2:
    cJSON_Delete(root);
fail3:
    return NULL;
}

static inline void touch(struct sc_input_manager *im, SDL_EventType action, int32_t x, int32_t y, uint64_t pointer_id)
{
    if (!im->mp->ops->process_touch)
    {
        return;
    }
    struct sc_touch_event evt = {
        .position = {
            .screen_size = im->screen->frame_size,
            .point =
                sc_screen_convert_drawable_to_frame_coords(im->screen, x, y),
        },
        .action = sc_touch_action_from_sdl(action),
        .pointer_id = pointer_id,
        .pressure = action == SDL_FINGERUP ? 0 : 1,
    };
    im->mp->ops->process_touch(im->mp, &evt);
}

static inline void rtouch(struct sc_input_manager *im, bool is_down, bool is_repeat, struct sc_game_map_data *data)
{
    assert(data->type == 1);

    if (is_down)
    {
        data->x1 = data->w0 * (float)rand() / (float)RAND_MAX + data->x0;
        data->y1 = data->h0 * (float)rand() / (float)RAND_MAX + data->y0;
    }
    else if (data->times <= 0)
    {
        return;
    }
    Uint32 now = SDL_GetTicks();

    if (!is_repeat)
    { // 非长按
        if (is_down)
        {
            touch(im, SDL_FINGERDOWN, data->x1, data->y1, data->pointer_id);
            data->times++;
            data->timestamp = now;
        }
        else
        {
            touch(im, SDL_FINGERUP, data->x1, data->y1, data->pointer_id);
            data->times = 0;
            data->timestamp = 0;
        }
    }
    else
    { // 长按
        if (data->times % 2 != 0)
        { // UP
            if (now - data->timestamp >= data->key_duration)
            {
                touch(im, SDL_FINGERUP, data->x1, data->y1, data->pointer_id);
                data->times++;
                data->timestamp = now;
            }
        }
        else
        { // DOWN
            if (now - data->timestamp >= data->key_interval)
            {
                touch(im, SDL_FINGERDOWN, data->x1, data->y1, data->pointer_id);
                data->times++;
                data->timestamp = now;
            }
        }
    }

    if (!is_down)
    {
        data->x1 = -1;
        data->y1 = -1;
    }
}

static inline void motion(struct sc_input_manager *im, struct sc_game_map_data *data, float xrel, float yrel)
{
    assert(data->type == 2);

    if (data->times <= 0)
    {
        if (data->x1 < 0 || data->y1 < 0)
        {
            data->x1 = data->x0 + data->w0 / 2 - xrel * 2.2;
            data->y1 = data->y0 + data->h0 / 2 - yrel * 2.2;
        }
        touch(im, SDL_FINGERDOWN, data->x1, data->y1, data->pointer_id);
        data->times++;
    }

    data->x1 += xrel;
    data->y1 += yrel;
    if (data->x1 < data->x0 + data->w0 &&
        data->y1 < data->y0 + data->h0 &&
        data->x1 > data->x0 &&
        data->y1 > data->y0)
    { // 移动
        touch(im, SDL_FINGERMOTION, data->x1 + xrel, data->y1 + yrel, data->pointer_id);
        data->times++;
    }
    else
    { // 抬起
        touch(im, SDL_FINGERUP, data->x1, data->y1, data->pointer_id);
        data->times = 0;
        data->x1 = data->x0 + data->w0 / 2 - xrel * 2.2;
        data->y1 = data->y0 + data->h0 / 2 - yrel * 2.2;
    }
}

static inline bool wasd(struct sc_input_manager *im, struct sc_game_map_data *data, bool is_down, SDL_Keycode sym)
{ // 0b XXXXDSAW
    uint8_t index;
    switch (sym)
    {
    case SDLK_w:
        index = 0;
        break;
    case SDLK_a:
        index = 1;
        break;
    case SDLK_s:
        index = 2;
        break;
    case SDLK_d:
        index = 3;
        break;
    default:
        return false;
    }

    if (is_down)
    {
        SETBIT(data->keyactions, index);
    }
    else
    {
        CLRBIT(data->keyactions, index);
    }

    float step = (float)rand() / (float)RAND_MAX + 9;
    if (data->times <= 0)
    { // DOWN
        assert(is_down);
        data->x1 = data->x0 + data->w0 / 2;
        data->y1 = data->y0 + data->h0 / 2;
        touch(im, SDL_FINGERDOWN, data->x1, data->y1, data->pointer_id);
        data->times++;
        return true;
    }

    if (data->keyactions == 0)
    { // UP
        assert(!is_down);
        touch(im, SDL_FINGERUP, data->x1, data->y1, data->pointer_id);
        data->times = 0;
        data->x1 = data->x0 + data->w0 / 2;
        data->y1 = data->y0 + data->h0 / 2;
        return true;
    }

    // MOTION
    float ymax = data->y0 + data->h0,
          xmax = data->x0 + data->w0;
    if (data->keyactions >> 0 & 1)
    { // W
        if (data->y1 > data->y0)
        {
            data->y1 = max(data->y0, data->y1 - step);
        }
    }
    if (data->keyactions >> 1 & 1)
    { // A
        if (data->x1 > data->x0)
        {
            data->x1 = max(data->x0, data->x1 - step);
        }
    }
    if (data->keyactions >> 2 & 1)
    { // S
        if (data->y1 < ymax)
        {
            data->y1 = min(ymax, data->y1 + step);
        }
    }
    if (data->keyactions >> 3 & 1)
    { // D
        if (data->x1 < xmax)
        {
            data->x1 = min(xmax, data->x1 + step);
        }
    }

    touch(im, SDL_FINGERMOTION, data->x1, data->y1, data->pointer_id);
    data->times++;
    return true;
}

void sc_input_manager_process_key_game(struct sc_input_manager *im, const SDL_KeyboardEvent *event)
{
    bool is_down = event->type == SDL_KEYDOWN;
    for (int i = 0; i < im->game->size; i++)
    {
        if (im->game->maps[i].type == 5)
        {
            if (wasd(im, &im->game->maps[i].data[0], is_down, event->keysym.sym))
            {
                continue;
            }
        }

        if (im->game->maps[i].type != 1 ||
            im->game->maps[i].input.key_code != event->keysym.sym)
        {
            continue;
        }

        for (int j = 0; j < im->game->maps[i].size; j++)
        {
            rtouch(im, is_down, event->repeat != 0, &im->game->maps[i].data[j]);
        }
        return;
    }
}

// TODO: 长按处理(注意: SDL_MouseButtonEvent 没有repeat字段)
// 应该是另开一个线程，在这个线程里面处理
void sc_input_manager_process_mouse_button_game(struct sc_input_manager *im, const SDL_MouseButtonEvent *event)
{
    int i, j;
    for (i = 0; i < im->game->size; i++)
    {
        if (im->game->maps[i].type != 2 ||
            im->game->maps[i].input.mouse_button != event->button)
        {
            continue;
        }

        for (j = 0; j < im->game->maps[i].size; j++)
        {
            rtouch(im, event->type == SDL_MOUSEBUTTONDOWN, false, &im->game->maps[i].data[j]);
        }
        return;
    }
}

void sc_input_manager_process_mouse_motion_game(struct sc_input_manager *im, const SDL_MouseMotionEvent *event)
{
    int i;
    for (i = 0; i < im->game->size; i++)
    {
        if (im->game->maps[i].type != 3)
        {
            continue;
        }
        if (im->game->maps[i].size != 1)
        {
            printf("[game] mouse_motion size != 1\n");
            return;
        }
        im->game->maps[i].data[0].timestamp = event->timestamp;
        motion(im, &im->game->maps[i].data[0], (float)event->xrel, (float)event->yrel);
        return;
    }
}

void sc_game_display(const struct sc_game *game, SDL_Renderer *renderer)
{
    assert(game->is_gaming && game->show);

    SDL_Rect rct;
    int i, j;
    for (i = 0; i < game->size; i++)
    {
        for (j = 0; j < game->maps[i].size; j++)
        {
            if (game->maps[i].img == NULL)
            {
                continue;
            }

            if (game->maps[i].img_texture == NULL)
            {
                game->maps[i].img_texture = SDL_CreateTextureFromSurface(renderer, game->maps[i].img);
                if (game->maps[i].img_texture == NULL)
                {
                    printf("SDL_CreateTextureFromSurface Error - %s\n", SDL_GetError());
                    continue;
                }
                if (SDL_SetTextureBlendMode(game->maps[i].img_texture, SDL_BLENDMODE_BLEND) != 0)
                {
                    printf("SDL_SetTextureBlendMode Error - %s\n", SDL_GetError());
                }
                if (SDL_SetTextureAlphaMod(game->maps[i].img_texture, game->maps[i].alpha) != 0)
                {
                    printf("SDL_SetTextureAlphaMod Error - %s\n", SDL_GetError());
                }
                SDL_FreeSurface(game->maps[i].img);
            }

            rct.x = game->maps[i].data[j].x0;
            rct.y = game->maps[i].data[j].y0;
            rct.w = game->maps[i].data[j].w0;
            rct.h = game->maps[i].data[j].h0;
            if (SDL_RenderCopy(renderer, game->maps[i].img_texture, NULL, &rct) != 0)
            {
                printf("SDL_RenderCopy Error - %s\n", SDL_GetError());
                continue;
            }
        }
    }
}

void sc_game_on(struct sc_game *game, SDL_Window *win)
{
    if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0)
    {
        printf("SDL_SetRelativeMouseMode Error - %s\n", SDL_GetError());
        return;
    }
    int dw, dh;
    SDL_GL_GetDrawableSize(win, &dw, &dh);
    for (int i = 0; i < game->size; i++)
    {
        for (int j = 0; j < game->maps[i].size; j++)
        {
            game->maps[i].data[j].x0 = game->maps[i].data[j].x * dw;
            game->maps[i].data[j].y0 = game->maps[i].data[j].y * dh;
            game->maps[i].data[j].w0 = game->maps[i].data[j].w * dw;
            game->maps[i].data[j].h0 = game->maps[i].data[j].h * dh;
        }
    }
    game->is_gaming = true;
    printf("开启游戏模式\n");
}

void sc_game_off(struct sc_input_manager *im)
{
    if (SDL_SetRelativeMouseMode(SDL_FALSE) != 0)
    {
        printf("SDL_SetRelativeMouseMode Error - %s\n", SDL_GetError());
        return;
    }
    im->game->is_gaming = false;

    // 退出时清理所有数据
    int i, j;
    for (i = 0; i < im->game->size; i++)
    {
        for (j = 0; j < im->game->maps[i].size; j++)
        {
            if (im->game->maps[i].data[j].times > 0)
            {
                touch(im, SDL_FINGERUP,
                      im->game->maps[i].data[j].x1,
                      im->game->maps[i].data[j].y1,
                      im->game->maps[i].data[j].pointer_id);
                im->game->maps[i].data[j].times = 0;
                im->game->maps[i].data[j].timestamp = 0;
                im->game->maps[i].data[j].x1 = -1;
                im->game->maps[i].data[j].y1 = -1;
            }
        }
    }
    printf("退出游戏模式\n");
}