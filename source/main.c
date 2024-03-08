#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <switch.h>

#include <ft2build.h>
#include <time.h>
#include FT_FREETYPE_H
#include "cursor.h"
#include "logList.h"

// Define the desired framebuffer resolution (here we set it to 720p).
#define FB_WIDTH  1280
#define FB_HEIGHT 720
#define FONT_PATH "romfs:/amiga4ever-pro2.ttf"
//============================menu============================
#define MAIN_MENU_SIZE 3
#define MAIN_MENU_TITLE_X 540
#define MAIN_MENU_TITLE_Y 54
#define CWP_MENU_SIZE 1
#define CWP_MENU_TITLE_X 500
#define CWP_MENU_TITLE_Y 54
#define TIME_X 900
#define TIME_Y 20
#define BATTERY_ICON_X 1000
#define BATTERY_ICON_Y 720
//============================menu============================

//============================log_list============================
#define SCROLL_LOG_LIST_MAX_LINES 32
#define SCROLL_LOG_LIST_MAX_LINE_LENGTH 255
//============================log_list============================

//上下左右白色边框
#define TOP_START_Y 10
#define TOP_END_Y 12
#define START_X 10
#define END_X 12
#define TOP_LINE(y, x) (((y) >= TOP_START_Y) && ((y) <= TOP_END_Y) && ((x) >= START_X) && ((x) <= FB_WIDTH - START_X))
#define BOTTOM_LINE(y, x) (((y) >= FB_HEIGHT - TOP_START_Y) && ((y) <= FB_HEIGHT - ((2*(TOP_START_Y)) - TOP_END_Y)) && ((x) >= START_X) && ((x) <= (FB_WIDTH - START_X)))
#define LEFT_LINE(y, x) (((y) >= TOP_START_Y) && ((y) <= FB_HEIGHT - ((2*(TOP_START_Y)) - TOP_END_Y)) && ((x) >= START_X) && ((x) <= END_X))
#define RIGHT_LINE(y, x) (((y) >= TOP_START_Y) && ((y) <= FB_HEIGHT - ((2*(TOP_START_Y)) - TOP_END_Y)) && ((x) >= FB_WIDTH - END_X) && ((x) <= FB_WIDTH - START_X))
#define HORIZON_START_Y 74
#define HORIZON_END_Y 76
#define HORIZON_LINE(y, x) (((y) >= HORIZON_START_Y) && ((y) <= HORIZON_END_Y) && ((x) >= START_X) && ((x) <= FB_WIDTH - START_X))

static u32 framebuf_width = 0;

static PadState pad;
static u32 batteryPercentage;
static bool power_charge;

static u32 offset_x = 64;
static u32 offset_y = 54;
static u32 offset = 60;

static bool text_draw[FB_WIDTH][FB_HEIGHT];

void draw_glyph_green(FT_Bitmap *bitmap, u32 *framebuf, u32 x, u32 y) {
    u32 framex, framey;
    u32 tmpx, tmpy;
    u8 *imageptr = bitmap->buffer;

    if (bitmap->pixel_mode != FT_PIXEL_MODE_GRAY) return;

    for (tmpy = 0; tmpy < bitmap->rows; tmpy++) {
        for (tmpx = 0; tmpx < bitmap->width; tmpx++) {
            framex = x + tmpx;
            framey = y + tmpy;

            // Check if the pixel is within the framebuffer boundaries
            if (framex >= 0 && framex < framebuf_width && framey >= 0 && framey < FB_HEIGHT) {
                u8 pixel = imageptr[tmpx];
                // Only draw non-blank pixels
                if (pixel != 0) {
                    framebuf[framey * framebuf_width + framex] = RGBA8_MAXALPHA(0, pixel, 0);
                    text_draw[framex][framey] = true;
                }
            }
        }
        imageptr += bitmap->pitch;
    }
}

void draw_text_green(FT_Face face, u32 *framebuf, u32 x, u32 y, const char *str) {
    u32 tmpx = x;
    FT_Error ret = 0;
    FT_UInt glyph_index;
    FT_GlyphSlot slot = face->glyph;

    u32 i;
    u32 str_size = strlen(str);
    uint32_t tmpchar;
    ssize_t unitcount = 0;

    for (i = 0; i < str_size;) {
        unitcount = decode_utf8(&tmpchar, (const uint8_t *) &str[i]);
        if (unitcount <= 0) break;
        i += unitcount;

        if (tmpchar == '\n') {
            tmpx = x;
            y += face->size->metrics.height / 64;
            continue;
        }

        glyph_index = FT_Get_Char_Index(face, tmpchar);
        //If using multiple fonts, you could check for glyph_index==0 here and attempt using the FT_Face for the other fonts with FT_Get_Char_Index.

        ret = FT_Load_Glyph(
                face,          /* handle to face object */
                glyph_index,   /* glyph index           */
                FT_LOAD_DEFAULT);

        if (ret == 0) {
            ret = FT_Render_Glyph(face->glyph,   /* glyph slot  */
                                  FT_RENDER_MODE_NORMAL);  /* render mode */
        }

        if (ret) return;

        draw_glyph_green(&slot->bitmap, framebuf, tmpx + slot->bitmap_left, y - slot->bitmap_top);

        tmpx += slot->advance.x >> 6;
        y += slot->advance.y >> 6;
    }
}

void draw_glyph_black(FT_Bitmap *bitmap, u32 *framebuf, u32 x, u32 y) {
    u32 framex, framey;
    u32 tmpx, tmpy;
    u8 *imageptr = bitmap->buffer;

    if (bitmap->pixel_mode != FT_PIXEL_MODE_GRAY) return;

    for (tmpy = 0; tmpy < bitmap->rows; tmpy++) {
        for (tmpx = 0; tmpx < bitmap->width; tmpx++) {
            framex = x + tmpx;
            framey = y + tmpy;

            // Check if the pixel is within the framebuffer boundaries
            if (framex >= 0 && framex < framebuf_width && framey >= 0 && framey < FB_HEIGHT) {
                u8 pixel = imageptr[tmpx];
                // Only draw non-blank pixels
                if (pixel != 0) {
                    framebuf[framey * framebuf_width + framex] = RGBA8_MAXALPHA(0, 0, 0);
                    text_draw[framex][framey] = true;
                }
            }
        }
        imageptr += bitmap->pitch;
    }
}

void draw_text_black(FT_Face face, u32 *framebuf, u32 x, u32 y, const char *str) {
    u32 tmpx = x;
    FT_Error ret = 0;
    FT_UInt glyph_index;
    FT_GlyphSlot slot = face->glyph;

    u32 i;
    u32 str_size = strlen(str);
    uint32_t tmpchar;
    ssize_t unitcount = 0;

    for (i = 0; i < str_size;) {
        unitcount = decode_utf8(&tmpchar, (const uint8_t *) &str[i]);
        if (unitcount <= 0) break;
        i += unitcount;

        if (tmpchar == '\n') {
            tmpx = x;
            y += face->size->metrics.height / 64;
            continue;
        }

        glyph_index = FT_Get_Char_Index(face, tmpchar);
        //If using multiple fonts, you could check for glyph_index==0 here and attempt using the FT_Face for the other fonts with FT_Get_Char_Index.

        ret = FT_Load_Glyph(
                face,          /* handle to face object */
                glyph_index,   /* glyph index           */
                FT_LOAD_DEFAULT);

        if (ret == 0) {
            ret = FT_Render_Glyph(face->glyph,   /* glyph slot  */
                                  FT_RENDER_MODE_NORMAL);  /* render mode */
        }

        if (ret) return;

        draw_glyph_black(&slot->bitmap, framebuf, tmpx + slot->bitmap_left, y - slot->bitmap_top);

        tmpx += slot->advance.x >> 6;
        y += slot->advance.y >> 6;
    }
}

void draw_glyph(FT_Bitmap *bitmap, u32 *framebuf, u32 x, u32 y) {
    u32 framex, framey;
    u32 tmpx, tmpy;
    u8 *imageptr = bitmap->buffer;

    if (bitmap->pixel_mode != FT_PIXEL_MODE_GRAY) return;

    for (tmpy = 0; tmpy < bitmap->rows; tmpy++) {
        for (tmpx = 0; tmpx < bitmap->width; tmpx++) {
            framex = x + tmpx;
            framey = y + tmpy;

            // Check if the pixel is within the framebuffer boundaries
            if (framex >= 0 && framex < framebuf_width && framey >= 0 && framey < FB_HEIGHT) {
                u8 pixel = imageptr[tmpx];
                // Only draw non-blank pixels
                if (pixel != 0) {
                    framebuf[framey * framebuf_width + framex] = RGBA8_MAXALPHA(pixel, pixel, pixel);
                    text_draw[framex][framey] = true;
                }
            }
        }

        imageptr += bitmap->pitch;
    }
}

void draw_text(FT_Face face, u32 *framebuf, u32 x, u32 y, const char *str) {
    u32 tmpx = x;
    FT_Error ret = 0;
    FT_UInt glyph_index;
    FT_GlyphSlot slot = face->glyph;

    u32 i;
    u32 str_size = strlen(str);
    uint32_t tmpchar;
    ssize_t unitcount = 0;

    for (i = 0; i < str_size;) {
        unitcount = decode_utf8(&tmpchar, (const uint8_t *) &str[i]);
        if (unitcount <= 0) break;
        i += unitcount;

        if (tmpchar == '\n') {
            tmpx = x;
            y += face->size->metrics.height / 64;
            continue;
        }

        glyph_index = FT_Get_Char_Index(face, tmpchar);
        //If using multiple fonts, you could check for glyph_index==0 here and attempt using the FT_Face for the other fonts with FT_Get_Char_Index.

        ret = FT_Load_Glyph(
                face,          /* handle to face object */
                glyph_index,   /* glyph index           */
                FT_LOAD_DEFAULT);

        if (ret == 0) {
            ret = FT_Render_Glyph(face->glyph,   /* glyph slot  */
                                  FT_RENDER_MODE_NORMAL);  /* render mode */
        }

        if (ret) return;

        draw_glyph(&slot->bitmap, framebuf, tmpx + slot->bitmap_left, y - slot->bitmap_top);

        tmpx += slot->advance.x >> 6;
        y += slot->advance.y >> 6;
    }
}

// 计算先前文本的像素宽度
u32 next_x(const char *previous_text, FT_Face face, u32 previous_x) {
    u32 previous_text_width = 0;
    for (int i = 0; i < strlen(previous_text); i++) {
        FT_UInt glyph_index = FT_Get_Char_Index(face, previous_text[i]);
        if (glyph_index) {
            FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER);
            previous_text_width += face->glyph->advance.x >> 6;
        }
    }
    return previous_x + previous_text_width;
}

__attribute__((format(printf, 1, 2)))
static int error_screen(const char *fmt, ...) {
    consoleInit(NULL);
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    printf("Press PLUS to exit\n");
    while (appletMainLoop()) {
        padUpdate(&pad);
        if (padGetButtonsDown(&pad) & HidNpadButton_Plus)
            break;
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
    return EXIT_FAILURE;
}

static u64 getSystemLanguage(void) {
    Result rc;
    u64 code = 0;

    rc = setInitialize();
    if (R_SUCCEEDED(rc)) {
        rc = setGetSystemLanguage(&code);
        setExit();
    }

    return R_SUCCEEDED(rc) ? code : 0;
}

// LanguageCode is only needed with shared-folder when using plGetSharedFont.
static u64 LanguageCode;

void userAppInit(void) {
    Result rc;

    rc = plInitialize(PlServiceType_User);
    if (R_FAILED(rc))
        diagAbortWithResult(rc);

    LanguageCode = getSystemLanguage();
}

void userAppExit(void) {
    plExit();
}

const char *const months[12] = {"01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12"};
const char *const days[9] = {"01", "02", "03", "04", "05", "06", "07", "08", "09"};

char *get_time() {
    time_t unixTime = time(NULL);
    struct tm *timeStruct = localtime(
            (const time_t *) &unixTime);//Gets UTC time. If you want local-time use localtime().

    int hours = timeStruct->tm_hour;
    int minutes = timeStruct->tm_min;
    int seconds = timeStruct->tm_sec;
    int day = timeStruct->tm_mday;
    int month = timeStruct->tm_mon;
    int year = timeStruct->tm_year + 1900;
    char *time = (char *) malloc(255 * sizeof(char));
    if (day <= 9) {
        char c[10];
        sprintf(c, "%s", days[day]);
        sprintf(time, "%i-%s-%s %02i:%02i:%02i", year, months[month], days[day], hours, minutes, seconds);
    } else {
        sprintf(time, "%i-%s-%i %02i:%02i:%02i", year, months[month], day, hours, minutes, seconds);
    }
    return time;
}

bool contains_chinese(const char *str) {
    while (*str) {
        wchar_t c;
        mbtowc(&c, str, MB_CUR_MAX);
        if (c >= 0x4E00 && c <= 0x9FFF) {
            return true;
        }
        str++;
    }
    return false;
}

// 加载字体文件到内存中
unsigned char *loadFontFile(const char *filename, long *fileSize) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        error_screen("Failed to open font file: %s\n", filename);
        return NULL;
    }

    //将文件指针移动到文件末尾
    fseek(file, 0, SEEK_END);
    //计算出文件大小
    *fileSize = ftell(file);
    //将文件指针移动到文件开头，一遍后续遍历文件数据
    fseek(file, 0, SEEK_SET);

    unsigned char *buffer = (unsigned char *) malloc(*fileSize);
    if (!buffer) {
        fclose(file);
        error_screen("Failed to allocate memory for font file\n");
        return NULL;
    }

    fread(buffer, 1, *fileSize, file);
    fclose(file);

    return buffer;
}

int main(int argc, char **argv) {
    Result rc = 0;
    FT_Error ret = 0;
    Result res = setInitialize();
    if (R_FAILED(res)) return error_screen("setInitialize() failed: 0X%x\n", res);

    res = setsysInitialize();
    if (R_FAILED(res)) return error_screen("setsysInitialize() failed: 0X%x\n", res);

    res = nifmInitialize(NifmServiceType_Admin);
    if (R_FAILED(res)) return error_screen("nifmInitialize() failed: 0X%x\n", res);

    res = psmInitialize();
    if (R_FAILED(res)) return error_screen("psmInitialize() failed: 0X%x\n", res);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    padInitializeDefault(&pad);

    // Use this when you want to use specific shared-folder(s). Since this example only uses 1 folder, only the folder loaded by this will be used.
    PlFontData font;
    rc = plGetSharedFontByType(&font, PlSharedFontType_ChineseSimplified);
    if (R_FAILED(rc))
        return error_screen("plGetSharedFontByType() failed: 0x%x\n", rc);

    rc = romfsInit();
    if (R_FAILED(rc))
        return error_screen("romfsInit() failed: 0x%x\n", rc);

    FT_Library library;
    ret = FT_Init_FreeType(&library);
    if (ret)
        return error_screen("FT_Init_FreeType() failed: %d\n", ret);

    FT_Face chinese_face;
    ret = FT_New_Memory_Face(library,
                             font.address,    /* first byte in memory */
                             font.size,       /* size in bytes        */
                             0,            /* face_index           */
                             &chinese_face);
    if (ret) {
        FT_Done_FreeType(library);
        return error_screen("FT_New_Memory_Face() failed: %d\n", ret);
    }

    // 从内存中加载第三方字体文件，如果从文件中直接加载字体文件输出的时候会很慢
    long fileSize;
    unsigned char *fontData = loadFontFile(FONT_PATH, &fileSize);
    if (!fontData) {
        FT_Done_FreeType(library);
        return 1;
    }

    FT_Face face;
    ret = FT_New_Memory_Face(library, fontData, (FT_Long) fileSize, 0, &face);
    if (ret) {
        FT_Done_FreeType(library);
        return error_screen("FT_New_Face() failed: %d\n", ret);
    }

    ret = FT_Set_Char_Size(
            face,                     /* face对象     */
            0,             /* 字符宽度 1/64 */
            14 * 64,     /* 字符高度 1/64 */
            96,      /* 水平分辨率     */
            96);    /* 垂直分辨率     */
    if (ret) {
        FT_Done_Face(face);
        FT_Done_Face(chinese_face);
        FT_Done_FreeType(library);
        return error_screen("FT_Set_Char_Size() failed: %d\n", ret);
    }

    Framebuffer fb;
    framebufferCreate(&fb, nwindowGetDefault(), FB_WIDTH, FB_HEIGHT, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    //主菜单
    Menu *main_menu;
    Init_Menu(&main_menu, MAIN_MENU_SIZE, true);
    strcpy(main_menu->selection[main_menu_title_selection].name, "Main Menu");
    main_menu->selection[main_menu_title_selection].x = MAIN_MENU_TITLE_X;
    main_menu->selection[main_menu_title_selection].y = MAIN_MENU_TITLE_Y;
    strcpy(main_menu->selection[clear_wifi_profiler_selection].name, "Clear Wifi Profile");
    main_menu->selection[clear_wifi_profiler_selection].x = offset_x;
    main_menu->selection[clear_wifi_profiler_selection].y = offset_y + clear_wifi_profiler_selection * offset;
//    strcpy(main_menu->selection[text_selection].name, "test");
//    main_menu->selection[text_selection].x = offset_x;
//    main_menu->selection[text_selection].y = main_menu->selection[clear_wifi_profiler_selection].y + MENU_DISTANCE;
    strcpy(main_menu->selection[exit_selection].name, "Exit");
    main_menu->selection[exit_selection].x = offset_x;
    main_menu->selection[exit_selection].y = offset_y + exit_selection * offset;

    //clear WiFi profiler 菜单
    Menu *cwp_menu;
    Init_Menu(&cwp_menu, CWP_MENU_SIZE, false);
    strcpy(cwp_menu->selection[cwp_title_selection].name, "Clear Wifi Profile");
    cwp_menu->selection[cwp_title_selection].x = CWP_MENU_TITLE_X;
    cwp_menu->selection[cwp_title_selection].y = CWP_MENU_TITLE_Y;
    bool search_wifi = false;
    bool detect_airplane_mode = false;
    s32 r_total_out = 0;

    Cursor cursor;
    /**
     * font_size：光标绘制是从y的坐标自底向上绘制，所以是当前y的坐标减去字体的高度\n
     * face->size->metrics.height >> 6: 计算字体的高度
     */
    u32 font_size = face->size->metrics.height >> 6;
    u32 cursor_start_x = offset_x;
    u32 cursor_start_y = offset_y + offset;
    Init_Cursor(&cursor, cursor_start_x, cursor_start_y, 1200, font_size, offset);

    logList *ll = NULL;

    u32 time_right_position = next_x("0000-00-00 00:00:00", face, TIME_X);
    SetSysNetworkSettings *NetworkSettings = NULL;
    char **ssid_list = NULL;
    int ssid_list_length = 0;
    int additional = 0;
    while (appletMainLoop()) {

        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u32 kDown = padGetButtonsDown(&pad);

        //进入 clear WiFi profiler
        if ((kDown & HidNpadButton_A) && (cursor.y == main_menu->selection[clear_wifi_profiler_selection].y)) {
            main_menu->print_flag = false;
            cwp_menu->print_flag = true;
            ll = createLogList(SCROLL_LOG_LIST_MAX_LINES, SCROLL_LOG_LIST_MAX_LINE_LENGTH);
        }

        // break in order to return to hbmenu
        if ((kDown & HidNpadButton_A) && (cursor.y == main_menu->selection[exit_selection].y))
            break;

        // 返回到上一级
        if (kDown & HidNpadButton_B && cwp_menu->print_flag) {
            if (ll != NULL) {
                clearLogList(ll);
                ll = NULL;
            }
            main_menu->print_flag = true;
            cwp_menu->print_flag = false;
            search_wifi = false;
            detect_airplane_mode = false;
            additional = 0;
            if (ssid_list != NULL) {
                for (int i = 0; i < ssid_list_length; ++i) {
                    free(ssid_list[i]);
                }
                free(ssid_list);
                ssid_list = NULL;
            }
            if (NetworkSettings != NULL) {
                free(NetworkSettings);
                NetworkSettings = NULL;
            }
        }

        u32 stride;
        u32 *framebuf = (u32 *) framebufferBegin(&fb, &stride);
        framebuf_width = stride / sizeof(u32);

        memset(framebuf, 0, stride * FB_HEIGHT);
        memset(text_draw, 0, sizeof(text_draw));

        for (u32 i = 0; i < FB_HEIGHT; i++) {
            for (u32 j = 0; j < FB_WIDTH; j++) {
                u32 pos = i * stride / sizeof(u32) + j;
                if (TOP_LINE(i, j) || BOTTOM_LINE(i, j) || LEFT_LINE(i, j) || RIGHT_LINE(i, j) ||
                    HORIZON_LINE(i, j))
                    framebuf[pos] = RGBA8_MAXALPHA(255, 255, 255);
                else
                    framebuf[pos] = RGBA8_MAXALPHA(0, 0, 165);
            }
        }

        char *time = get_time();
        for (u32 i = TOP_START_Y; i <= TOP_END_Y; i++) {
            for (u32 j = TIME_X; j < time_right_position; j++) {
                u32 pos = i * stride / sizeof(u32) + j;
                framebuf[pos] = RGBA8_MAXALPHA(0, 0, 165);
            }
        }
        draw_text(face, framebuf, TIME_X, TIME_Y,
                  time);

        for (u32 i = FB_HEIGHT - TOP_START_Y; i <= FB_HEIGHT - (2*TOP_START_Y - TOP_END_Y); i++) {
            for (u32 j = BATTERY_ICON_X; j < BATTERY_ICON_X + 200; j++) {
                u32 pos = i * stride / sizeof(u32) + j;
                framebuf[pos] = RGBA8_MAXALPHA(0, 0, 165);
            }
        }
        psmGetBatteryChargePercentage(&batteryPercentage);
        draw_text(face, framebuf, BATTERY_ICON_X, BATTERY_ICON_Y, "[");
        u32 w = next_x("[", face, BATTERY_ICON_X);
        draw_text(face, framebuf, 100 + w, BATTERY_ICON_Y, "}");
        char v[255];
        sprintf(v, "%d%%", batteryPercentage);
        u32 battery_percentage_x = next_x("}", face, 100 + w) + 10;
        draw_text(face, framebuf, battery_percentage_x, BATTERY_ICON_Y, v);

        u32 end = w + batteryPercentage;

        for (u32 i = BATTERY_ICON_Y - (face->size->metrics.height >> 6); i < BATTERY_ICON_Y; i++) {
            for (u32 j = w; j < end; j++) {
                u32 pos = i * stride / sizeof(u32) + j;
                if(batteryPercentage <= 100 && batteryPercentage >= 60)
                    framebuf[pos] = RGBA8_MAXALPHA(0, 255, 0);
                else if (batteryPercentage < 60 && batteryPercentage > 20)
                    framebuf[pos] = RGBA8_MAXALPHA(255, 255, 0);
                else
                    framebuf[pos] = RGBA8_MAXALPHA(255, 0, 0);
            }
        }
        psmIsEnoughPowerSupplied(&power_charge);
        if(power_charge)
            draw_text_black(face, framebuf, w + 100 / 2 - 10, BATTERY_ICON_Y - 2, ":D-");

        if (main_menu->print_flag) {
            // 绘制光标，使用光标的位置和尺寸确定绘制范围
            Draw_Cursor(&cursor, stride, &text_draw, framebuf, 0, 0, 0);
            // 绘制主菜单选项
            for (int i = 0; i <= exit_selection; ++i) {
                draw_text(face, framebuf, main_menu->selection[i].x, main_menu->selection[i].y,
                          main_menu->selection[i].name);
            }
            if (kDown & HidNpadButton_Down || kDown & HidNpadButton_StickLDown) {
                Draw_Cursor(&cursor, stride, &text_draw, framebuf, 0, 0, 165);
                decide_menu_down(&cursor, main_menu, exit_selection);
                if (main_menu->print_flag)
                    Draw_Cursor(&cursor, stride, &text_draw, framebuf, 0, 0, 0);
            }

            if (kDown & HidNpadButton_Up || kDown & HidNpadButton_StickLUp) {
                Draw_Cursor(&cursor, stride, &text_draw, framebuf, 0, 0, 165);
                decide_menu_up(&cursor, main_menu, exit_selection);
                if (main_menu->print_flag)
                    Draw_Cursor(&cursor, stride, &text_draw, framebuf, 0, 0, 0);
            }
        }

        if (cwp_menu->print_flag) {
            draw_text(face, framebuf, cwp_menu->selection[cwp_title_selection].x,
                      cwp_menu->selection[cwp_title_selection].y, cwp_menu->selection[cwp_title_selection].name);
            //日志开始输出的y坐标
            u32 start_y = offset_y + offset - (face->size->metrics.height >> 6);
            //检测switch当前的互联网状态（是否是飞行模式）
            if (!detect_airplane_mode) {
                bool enable;
                res = nifmIsWirelessCommunicationEnabled(&enable);
                if (R_FAILED(res)) return error_screen("nifmIsWirelessCommunicationEnabled() failed: 0X%x\n", res);
                //如果不是飞行模式，就开启飞行模式
                if (enable) {
                    enable = false;
                    res = nifmSetWirelessCommunicationEnabled(enable);
                    if (R_FAILED(res)) return error_screen("nifmSetWirelessCommunicationEnabled() failed: 0X%x\n", res);
                }
                add(ll, "airplane mode: enable");
                detect_airplane_mode = true;
            }

            //查询一共有多少WiFi
            if (!search_wifi) {
                s32 r_count;
                s32 count = 10;
                do {
                    s32 total_out;
                    r_count = count;
                    free(NetworkSettings);
                    NetworkSettings = malloc(count * sizeof(SetSysNetworkSettings));
                    res = setsysGetNetworkSettings(&total_out, NetworkSettings, count);
                    if (R_FAILED(res)) {
                        return error_screen("setsysGetNetworkSettings() failed: 0X%x\n", res);
                    }

                    r_total_out = total_out;

                    if (total_out == count) {
                        count += 10;
                    }
                } while (r_total_out == r_count);

                char r_total_out_s[255];
                sprintf(r_total_out_s, "total out: %d", r_total_out - 1);
                add(ll, r_total_out_s);
                if (r_total_out - 1 == 0) {
                    add(ll, "no wifi profiles are currently set up.");
                } else {
                    add(ll, "press plus(+) to remove all wifi profiles or press B to return Main Menu");
                }
                u32 tempY = start_y;
                for (int i = 0; i <= ll->cur_index; ++i) {
                    draw_text(face, framebuf, offset_x, tempY, ll->logs[i].log);
                    tempY += (face->size->metrics.height >> 6);
                }
                search_wifi = true;
            }

            if (kDown & HidNpadButton_Plus && NetworkSettings != NULL && r_total_out - 1 != 0) {
                Service *service = nifmGetServiceSession_GeneralService();
                ssid_list = malloc(r_total_out * sizeof(char *));
                for (int i = 0; i < r_total_out; ++i) {
                    ssid_list[i] = malloc(255 * sizeof(char));
                }

                //删除switch的所有ssid
                for (int i = 0; i < r_total_out; ++i) {
                    if (strlen((NetworkSettings + i)->access_point_ssid) > 0 &&
                        strlen((NetworkSettings + i)->access_point_ssid) <= 33) {
                        res = serviceDispatchImpl(service, 10, &((NetworkSettings + i)->uuid), sizeof(Uuid), NULL,
                                                  0,
                                                  (SfDispatchParams) {0});
                        if (R_FAILED(res)) {
                            free(NetworkSettings);
                            NetworkSettings = NULL;
                            return error_screen("serviceDispatchImpl() failed: 0X%x", res);
                        }
                        char ssid[255];
                        sprintf(ssid, "ssid: %s", (NetworkSettings + i)->access_point_ssid);
                        strcpy(ssid_list[i - 1], ssid);
                    }
                }

                strcpy(ssid_list[r_total_out - 1], "delete all wifi profiles! press B to return.");

                int other = SCROLL_LOG_LIST_MAX_LINES - ll->cur_index - 1;
                for (int i = 0; i < (r_total_out <= other ? r_total_out : other); ++i) {
                    add(ll, ssid_list[i]);
                }

                if(r_total_out > other){
                    additional = r_total_out - other;
                }

                free(NetworkSettings);
                NetworkSettings = NULL;
            }

            for (int i = 0; i <= ll->cur_index; ++i) {
                u32 temp_offset_x = offset_x;
                bool chinese_str = contains_chinese(ll->logs[i].log);
                if (chinese_str) {
                    if (strncmp(ll->logs[i].log, "ssid:", strlen("ssid:")) == 0) {
                        draw_text(face, framebuf, offset_x, start_y, "ssid: ");
                    }
                    temp_offset_x = next_x("ssid: ", face, offset_x);
                    char chinese_ssid[50];
                    strncpy(chinese_ssid, ll->logs[i].log + strlen("ssid: "), strlen(ll->logs[i].log));
                    draw_text(chinese_face, framebuf, temp_offset_x, start_y, chinese_ssid);
                } else {
                    draw_text(face, framebuf, offset_x, start_y, ll->logs[i].log);
                }

                if (strncmp(ll->logs[i].log, "ssid:", strlen("ssid:")) == 0) {
                    char str[] = "...";
                    u32 str_next;
                    if (chinese_str) {
                        str_next = next_x(ll->logs[i].log, chinese_face, temp_offset_x);
                    } else {
                        str_next = next_x(ll->logs[i].log, face, temp_offset_x);
                    }
                    u32 str_next_next = next_x(str, face, str_next);
                    draw_text(face, framebuf, str_next, start_y, str);
                    draw_text_green(face, framebuf, str_next_next, start_y, "[CLEAR]");
                }
                start_y += (face->size->metrics.height >> 6);
            }

            if (additional > 0) {
                add(ll, ssid_list[r_total_out - additional]);
                additional--;
            }
        }
        free(time);
        framebufferEnd(&fb);
    }
    if (ll != NULL) {
        clearLogList(ll);
        ll = NULL;
    }
    if (ssid_list != NULL) {
        for (int i = 0; i < ssid_list_length; ++i) {
            free(ssid_list[i]);
        }
        free(ssid_list);
    }
    if (NetworkSettings != NULL)
        free(NetworkSettings);

    free(main_menu);
    psmExit();
    romfsExit();
    framebufferClose(&fb);
    FT_Done_Face(chinese_face);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    nifmExit();
    setsysExit();
    setExit();
    return 0;
}
