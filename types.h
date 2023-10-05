/* Constants */
#define DEV_NAME "oled"
#define OLED_COLS 128
#define OLED_ROWS 64
#define I2C_BUS   0x01
#define OLED_ADDR 0x3C
#define KERNEL_OUT(...) { printk(DEV_NAME": ", __VA_ARGS__); }

#define OLED_NO_ERRORS 0
#define OLED_INPUT_ERROR 1

/* Typedefs */
typedef enum {
    OLED_RESUME_TO_RAM = 0xA4,
    OLED_ENTIRE_DISPLAY_ON
} OledEntireDisplay;

typedef enum {
    OLED_MODE_NORMAL = 0xA6,
    OLED_MODE_INVERSE
} OledMode;

typedef enum {
    OLED_POWER_OFF = 0xAE,
    OLED_POWER_ON
} OledPower;

typedef enum {
    OLED_SCROLL_HOTIZONTAL_RIGHT = 0x26,
    OLED_SCROLL_HOTIZONTAL_LEFT
} OledScrollHrz;

typedef enum {
    OLED_SCROLL_VERT_HRZ_RIGHT = 0x29,
    OLED_SCROLL_VERT_HRZ_LEFT
} OledScrollVH;

typedef enum {
    OLED_DEACT_SCROLL = 0x2E,
    OLED_ACT_SCROLL
} OledScrollState;

typedef enum {
    OLED_PAGE0 = 0x0,
    OLED_PAGE1,
    OLED_PAGE2,
    OLED_PAGE3,
    OLED_PAGE4,
    OLED_PAGE5,
    OLED_PAGE6,
    OLED_PAGE7
} OledPageAddr;

typedef enum {
    OLED_TIME_INT_5_FRAMES = 0x0,
    OLED_TIME_INT_64_FRAMES,
    OLED_TIME_INT_128_FRAMES,
    OLED_TIME_INT_256_FRAMES,
    OLED_TIME_INT_3_FRAMES,
    OLED_TIME_INT_4_FRAMES,
    OLED_TIME_INT_25_FRAMES,
    OLED_TIME_INT_2_FRAMES
} OledFrameFreq;

typedef enum {
    OLED_MM_HRZ_AM = 0x0,
    OLED_MM_VERT_AM,
    OLED_MM_PAGE_AM,
    OLED_MM_INV_AM
} OledMemAddrMode;

typedef enum {
    OLED_OSD_NORMAL = 0xA0,
    OLED_OSD_REMAP = 0xA8
} OledOSDMode;
