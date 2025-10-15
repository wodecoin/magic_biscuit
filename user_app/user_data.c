typedef enum
{
    MODE_CHIPS = 0, // 薯条
    MODE_CHICKEN,   // 鸡翅
    MODE_FISH,      // 鱼
    MODE_STEAK,     // 牛排
    MODE_SHRIMP,    // 虾
    MODE_BABYFOOD,  // 宝宝辅食
    MODE_KEEPWARM,  // 保温
} recipes_t;

typedef struct
{
    recipes_t recipe;          // 菜谱类型
    uint8_t area_temp[4];      // 温度（℃）
    uint8_t area_time[4];      // 时间（分钟）
    uint8_t area_countdown[4]; // 倒计时（分钟）
} cooking_data_t;

/* 菜谱初始化数据 */
static const cooking_data_t cooking_table[] =
    {
        // 薯条
        {
            .recipe = MODE_CHIPS,
            .area_temp = {180, 180, 180, 180},
            .area_time = {15, 15, 15, 15},
            .area_countdown = {0, 0, 0, 0},
        },
        // 鸡翅
        {
            .recipe = MODE_CHICKEN,
            .area_temp = {200, 200, 200, 200},
            .area_time = {20, 20, 20, 20},
            .area_countdown = {0, 0, 0, 0},
        },
        // 鱼
        {
            .recipe = MODE_FISH,
            .area_temp = {190, 190, 190, 190},
            .area_time = {15, 15, 15, 15},
            .area_countdown = {0, 0, 0, 0},
        },
        // 牛排
        {
            .recipe = MODE_STEAK,
            .area_temp = {210, 210, 210, 210},
            .area_time = {12, 12, 12, 12},
            .area_countdown = {0, 0, 0, 0},
        },
        // 虾
        {
            .recipe = MODE_SHRIMP,
            .area_temp = {190, 190, 190, 190},
            .area_time = {10, 10, 10, 10},
            .area_countdown = {0, 0, 0, 0},
        },
        // 宝宝辅食
        {
            .recipe = MODE_BABYFOOD,
            .area_temp = {130, 130, 130, 130},
            .area_time = {20, 20, 20, 20},
            .area_countdown = {0, 0, 0, 0},
        },
        // 保温
        {
            .recipe = MODE_KEEPWARM,
            .area_temp = {70, 70, 70, 70},
            .area_time = {40, 40, 40, 40},
            .area_countdown = {0, 0, 0, 0},
        },
};
