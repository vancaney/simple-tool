#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <switch.h>

#include <ft2build.h>
#include <unistd.h>
#include <time.h>
#include FT_FREETYPE_H
#include "cursor.h"
#include "menu.h"

// Define the desired framebuffer resolution (here we set it to 720p).
#define FB_WIDTH  1280
#define FB_HEIGHT 720
#define LOG_LIST_MAX_LINES 21
#define LOG_LIST_MAX_LINE_LENGTH 255

#define HORIZON_LINE(y, x) (((y) >= 74) && ((y) <= 76) && ((x) >= 10) && ((x) <= FB_WIDTH - 10))
//上下左右白色边框
#define TOP_LINE(y, x) (((y) >= 10) && ((y) <= 12) && ((x) >= 10) && ((x) <= FB_WIDTH - 10))
#define BOTTOM_LINE(y, x) ((y) >= FB_HEIGHT - 10 && ((y) <= FB_HEIGHT - 8) && ((x) >= 10) && ((x) <= (FB_WIDTH - 10)))
#define LEFT_LINE(y, x) ((y) >= 10 && ((y) <= FB_HEIGHT - 8) && ((x) >= 10) && ((x) <= 12))
#define RIGHT_LINE(y, x) ((y) >= 10 && ((y) <= FB_HEIGHT - 8) && ((x) >= FB_WIDTH - 12) && ((x) <= FB_WIDTH - 10))

static u32 framebuf_width = 0;

static PadState pad;

static u32 offset_x = 64;
static u32 offset_y = 54;
static u32 offset = 60;

static bool text_draw[FB_WIDTH][FB_HEIGHT];

//Note that this doesn't handle any blending.
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
//void draw_glyph(FT_Bitmap *bitmap, u32 *framebuf, u32 x, u32 y) {
//    u32 framex, framey;
//    u32 tmpx, tmpy;
//    u8 *imageptr = bitmap->buffer;
//
//    if (bitmap->pixel_mode != FT_PIXEL_MODE_GRAY) return;
//
//    for (tmpy = 0; tmpy < bitmap->rows; tmpy++) {
//        for (tmpx = 0; tmpx < bitmap->width; tmpx++) {
//            framex = x + tmpx;
//            framey = y + tmpy;
//
//            framebuf[framey * framebuf_width + framex] = RGBA8_MAXALPHA(imageptr[tmpx], imageptr[tmpx], imageptr[tmpx]);
//        }
//
//        imageptr += bitmap->pitch;
//    }
//}

//Note that this doesn't handle {tmpx > width}, etc.
//str is UTF-8.
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

void check_top_line(u32 *framebuf, u32 stride, u32 x, u32 y) {
    for (u32 i = 0; i < 15; i++) {
        for (u32 j = x; j < 1220; j++) {
            u32 pos = i * stride / sizeof(u32) + j;
            framebuf[pos] = RGBA8_MAXALPHA(0, 0, 165);
        }
    }
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

int main(int argc, char **argv) {
    Result rc = 0;
    FT_Error ret = 0;
    Result res = setInitialize();
    if (R_FAILED(res)) return error_screen("setInitialize() failed: 0X%x\n", res);

    res = setsysInitialize();
    if (R_FAILED(res)) return error_screen("setsysInitialize() failed: 0X%x\n", res);

    res = nifmInitialize(NifmServiceType_Admin);
    if (R_FAILED(res)) return error_screen("nifmInitialize() failed: 0X%x\n", res);

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    padInitializeDefault(&pad);

    //Use this when using multiple shared-fonts.
    /*
    PlFontData fonts[PlSharedFontType_Total];
    size_t total_fonts=0;
    rc = plGetSharedFont(LanguageCode, fonts, PlSharedFontType_Total, &total_fonts);
    if (R_FAILED(rc))
        return error_screen("plGetSharedFont() failed: 0x%x\n", rc);
    */

    // Use this when you want to use specific shared-folder(s). Since this example only uses 1 folder, only the folder loaded by this will be used.
    PlFontData font;
    rc = plGetSharedFontByType(&font, PlSharedFontType_Standard);
    if (R_FAILED(rc))
        return error_screen("plGetSharedFontByType() failed: 0x%x\n", rc);

    rc = romfsInit();
    if (R_FAILED(rc))
        return error_screen("romfsInit() failed: 0x%x\n", rc);

    FT_Library library;
    ret = FT_Init_FreeType(&library);
    if (ret)
        return error_screen("FT_Init_FreeType() failed: %d\n", ret);

    FT_Face face;

    ret = FT_New_Face(library, "romfs:/amiga4ever-pro2.ttf", 0, &face);
    if (ret) {
        FT_Done_FreeType(library);
        return error_screen("FT_New_Memory_Face() failed: %d\n", ret);
    }

    ret = FT_Set_Char_Size(
            face,                     /* handle to face object           */
            0,             /* char_width in 1/64th of points  */
            14 * 64,     /* char_height in 1/64th of points */
            96,      /* horizontal device resolution    */
            96);    /* vertical device resolution      */
    if (ret) {
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return error_screen("FT_Set_Char_Size() failed: %d\n", ret);
    }

    Framebuffer fb;
    framebufferCreate(&fb, nwindowGetDefault(), FB_WIDTH, FB_HEIGHT, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    Cursor cursor;
    u32 font_size = 20;
    u32 cursor_start_x = offset_x;
    u32 cursor_start_y = offset_y + offset - font_size;
    u32 cursor_start_height = 115;
    Init_Cursor(&cursor, cursor_start_x, cursor_start_y, 1200, 115, offset);

    u32 cursor_end_height = 0;
    Menu *main_menu;
    Init_Menu(&main_menu, 3, true);
    strcpy(main_menu->selection[0].name, "Main Menu");
    main_menu->selection[0].x = 540;
    main_menu->selection[0].y = 54;
    strcpy(main_menu->selection[1].name, "Clear Wifi Profile");
    main_menu->selection[1].x = offset_x;
    main_menu->selection[1].y = offset_y + offset;
    strcpy(main_menu->selection[2].name, "Exit");
    main_menu->selection[2].x = offset_x;
    main_menu->selection[2].y = offset_y + 2 * offset;

    Menu *cwp_menu;
    Init_Menu(&cwp_menu, 1, false);
    strcpy(cwp_menu->selection[0].name, "Clear Wifi Profile");
    cwp_menu->selection[0].x = 500;
    cwp_menu->selection[0].y = 54;
    bool enable;
    bool start_delete_ssid = true;
    s32 r_total_out = 0;

    char log_list[LOG_LIST_MAX_LINES][LOG_LIST_MAX_LINE_LENGTH];
    char **ssid_list = NULL;
    int ssid_list_dif_log_list = 0;
    int num_lines_to_display = 0;
    bool print_return_sentence = true;
    while (appletMainLoop()) {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u32 kDown = padGetButtonsDown(&pad);
        if ((kDown & HidNpadButton_A) && (cursor.y == main_menu->selection[1].y - 20)) {
            main_menu->print_flag = false;
            cwp_menu->print_flag = true;
        }
        if ((kDown & HidNpadButton_A) && (cursor.y == main_menu->selection[2].y - 20))
            break; // break in order to return to hbmenu

        if (kDown & HidNpadButton_B && cwp_menu->print_flag) {
            main_menu->print_flag = true;
            cwp_menu->print_flag = false;
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

        check_top_line(framebuf, stride, 900, 20);
        char *time = get_time();
        draw_text(face, framebuf, 900, 20,
                  time);

        if (main_menu->print_flag) {
            memset(log_list, 0, sizeof(log_list));
            start_delete_ssid = true;

            // 绘制光标，使用光标的位置和尺寸确定绘制范围
            Draw_Cursor(&cursor, stride, &text_draw, framebuf, 0, 0, 0);

            draw_text(face, framebuf, main_menu->selection[0].x, main_menu->selection[0].y,
                      main_menu->selection[0].name);
            draw_text(face, framebuf, main_menu->selection[1].x, main_menu->selection[1].y,
                      main_menu->selection[1].name);
            draw_text(face, framebuf, main_menu->selection[2].x, main_menu->selection[2].y,
                      main_menu->selection[2].name);
        }

        if (cwp_menu->print_flag) {

            draw_text(face, framebuf, cwp_menu->selection[0].x, cwp_menu->selection[0].y, cwp_menu->selection[0].name);
            //检测switch当前的互联网状态（是否是飞行模式）
            res = nifmIsWirelessCommunicationEnabled(&enable);
            if (R_FAILED(res)) return error_screen("nifmIsWirelessCommunicationEnabled() failed: 0X%x\n", res);
            //如果不是飞行模式，就开启飞行模式
            if (enable) {
                enable = false;
                res = nifmSetWirelessCommunicationEnabled(enable);
                if (R_FAILED(res)) return error_screen("nifmSetWirelessCommunicationEnabled() failed: 0X%x\n", res);
            }

//            draw_text(face, framebuf, offset_x, offset_y + offset, "airplane mode: enable");
            if (start_delete_ssid) {
                //下面是查询WiFi，并且删除（这个逻辑只能被执行一次）
                s32 r_count;
                s32 count = 10;
                SetSysNetworkSettings *NetworkSettings = NULL;
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

                if (r_total_out - 1 != 0) {
                    Service *service = nifmGetServiceSession_GeneralService();

                    //删除switch的所有ssid
                    for (int i = 0; i < r_count; ++i) {
                        if (strlen((NetworkSettings + i)->access_point_ssid) > 0 &&
                            strlen((NetworkSettings + i)->access_point_ssid) <= 33) {
                            res = serviceDispatchImpl(service, 10, &((NetworkSettings + i)->uuid), sizeof(Uuid), NULL,
                                                      0,
                                                      (SfDispatchParams) {0});
                            if (R_FAILED(res)) return error_screen("serviceDispatchImpl() failed: 0X%x", res);
                        }
                    }

//                printf("delete all wifi profiles!");

                } else {
//                printf("no wifi profiles are currently set up.");
                }
                strcpy(log_list[0], "airplane mode: enable");
                char s[255];
                sprintf(s, "total out: %d", r_total_out - 1);
                strcpy(log_list[1], s);
                if (r_total_out - 1 == 0) {
                    strcpy(log_list[2], "no wifi profiles are currently set up.");
                }
                for (int i = 1; i < r_total_out; ++i) {
                    char ssid[255];
                    sprintf(ssid, "ssid: %s", (NetworkSettings + i)->access_point_ssid);
                    if (i < sizeof(log_list) / sizeof(log_list[0]) - 1) {
                        int index = i + 1;
                        strcpy(log_list[index], ssid);
                    } else break;
                }

                ssid_list = malloc(r_total_out * sizeof(char *));

                for (int i = 0; i < r_total_out; ++i) {
                    ssid_list[i] = malloc(50 * sizeof(char));
                }

                for (int i = 1; i < r_total_out; ++i) {
                    char ssid[50];
                    sprintf(ssid, "ssid: %s", (NetworkSettings + i)->access_point_ssid);
                    strcpy(ssid_list[i - 1], ssid);
                }

                ssid_list_dif_log_list = r_total_out - 1 - (LOG_LIST_MAX_LINES - 2);

                free(NetworkSettings);
                NetworkSettings = NULL;
                start_delete_ssid = false;
            }

            u32 start_y = offset_y + offset - 15;
            for (int i = 0; i < num_lines_to_display; ++i) {
                draw_text(face, framebuf, offset_x, start_y, log_list[i]);
                if (strncmp(log_list[i], "ssid:", strlen("ssid:")) == 0)
                    draw_text_green(face, framebuf, offset_x + 700, start_y, "[CLEAR]");
                start_y += 30;
//                char str[255];
//                sprintf(str, "%d", ssid_list_dif_log_list);
//                draw_text(face, framebuf, offset_x, 684, str);
            }

            // Increment the number of lines to display, up to the maximum
            if (num_lines_to_display < LOG_LIST_MAX_LINES) {
                num_lines_to_display++;
            } else {
                if (ssid_list_dif_log_list > 0) {
                    for (int i = 0; i < LOG_LIST_MAX_LINES - 1; i++)
                        strcpy(log_list[i], log_list[i + 1]);

                    strcpy(log_list[LOG_LIST_MAX_LINES - 1], ssid_list[r_total_out - 1 - ssid_list_dif_log_list]);
                    ssid_list_dif_log_list--;
                }

                if (num_lines_to_display == LOG_LIST_MAX_LINES && print_return_sentence && r_total_out - 1 > 0) {
                    if(r_total_out - 1 < LOG_LIST_MAX_LINES - 2){
                        strcpy(log_list[2 + r_total_out - 1], "delete all wifi profiles! press B to return");
                    } else{
                        for (int i = 0; i < LOG_LIST_MAX_LINES - 1; i++)
                            strcpy(log_list[i], log_list[i + 1]);

                        strcpy(log_list[LOG_LIST_MAX_LINES - 1], "delete all wifi profiles! press B to return");
                    };

                    print_return_sentence = false;
                }
            }
            free(time);
            time = NULL;
        }

        cursor_end_height = offset_y + 2 * offset;

        if (kDown & HidNpadButton_Down) {
            Draw_Cursor(&cursor, stride, &text_draw, framebuf, 0, 0, 165);
            cursor.y += cursor.offset;
            cursor.height += cursor.offset;
            if (cursor.height > cursor_end_height + 1) {
                cursor.y = cursor_start_y;
                cursor.height = cursor_start_height;
            }
            if (main_menu->print_flag)
                Draw_Cursor(&cursor, stride, &text_draw, framebuf, 0, 0, 0);
        }

        if (kDown & HidNpadButton_Up) {
            Draw_Cursor(&cursor, stride, &text_draw, framebuf, 0, 0, 165);

            cursor.y -= cursor.offset;
            cursor.height -= cursor.offset;
            if (cursor.y < cursor_start_y) {
                cursor.y = cursor_end_height - 20;
                cursor.height = cursor_end_height;
            }
            if (main_menu->print_flag)
                Draw_Cursor(&cursor, stride, &text_draw, framebuf, 0, 0, 0);
        }
        free(time);
        framebufferEnd(&fb);
    }

    for (int i = 0; i < r_total_out; ++i)
        free(ssid_list[i]);
    free(ssid_list);
    free(main_menu);
    romfsExit();
    framebufferClose(&fb);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    return 0;
}