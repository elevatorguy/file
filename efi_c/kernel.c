#include <stdint.h>
#include <stdnoreturn.h>

#include "file.h"
#include "lib.h"

#define arch_header <arch/ARCH/ARCH.h>
#include arch_header

enum {
    LIGHT_GRAY = 0,
    RED,
    DARK_GRAY,
    COLOR_MAX,
} COLOR_NAMES;

const uint32_t colors[COLOR_MAX] = {
    [LIGHT_GRAY] = 0xFFDDDDDD, 
    [RED]        = 0xFFCC2222, 
    [DARK_GRAY]  = 0xFF222222, 
};

//Table 12-1: Local APIC Register Address Map (pdf page 3467 - June 2025)
#define LVT_CORRECTED_MACHINE_CHECK_INTERRUPT *(volatile uint32_t*)(0xFEE002F0)
#define LVT_TIMER                             *(volatile uint32_t*)(0xFEE00320)
//#define LVT_THERMAL_SENSOR                    *(volatile uint32_t*)(0xFEE00330)
//#define LVT_PERFORMANCE_MONITORING_COUNTERS   *(volatile uint32_t*)(0xFEE00340)
#define LVT_LINT0                             *(volatile uint32_t*)(0xFEE00350)
#define LVT_LINT1                             *(volatile uint32_t*)(0xFEE00360)
#define LVT_ERROR                             *(volatile uint32_t*)(0xFEE00370)

uint32_t *fb = NULL;
uint32_t xres = 0;
uint32_t yres = 0;
uint32_t x = 0;
uint32_t y = 0;

const uint32_t text_fg_color = colors[LIGHT_GRAY];
const uint32_t text_bg_color = colors[DARK_GRAY];

void print_string(char *string, Bitmap_Font *font);

EFI_EVENT timer_event;
char text1[255];
char text2[255];

void update_text(EFI_TIME* t);

__attribute__((section(".kernel"), aligned(0x1000))) 
noreturn void EFIAPI kmain(Kernel_Parms *kargs) {
    fb = (UINT32 *)kargs->gop_mode.FrameBufferBase;  
    xres = kargs->gop_mode.Info->PixelsPerScanLine;
    yres = kargs->gop_mode.Info->VerticalResolution;

    UINTN color = colors[DARK_GRAY];
    for (y = 0; y < yres; y++) 
        for (x = 0; x < xres; x++) 
            fb[y*xres + x] = color;

    for(uint16_t i = 0; i < 256; i++) {
        text1[i] = '\0';
        text2[i] = '\0';
    }

    x = y = 0;
    Bitmap_Font *font1 = &kargs->fonts[0];
    Bitmap_Font *font2 = &kargs->fonts[1];
    
    EFI_TIME old_time = {0}, new_time = {0};
    EFI_TIME_CAPABILITIES time_cap = {0};
    bool needed = true;
    while (needed) {
        kargs->RuntimeServices->GetTime(&new_time, &time_cap);
        if (old_time.Second != new_time.Second) {
            x = y = 0;
            new_time = adjust(new_time, utc_offset);
            update_text(&new_time);
            print_string(text1, font1);
            print_string("\r\n", font1);
            print_string(text2, font2);
            old_time.Second = new_time.Second;
            
            /*
            EFI_KEY_DATA data = {0};
            EFI_INPUT_KEY key = {0};
            EFI_STATUS status = cin->ReadKeyStrokeEx(cin, &data);
            if(status == EFI_SUCCESS) {
                key = data.Key;
                if(key.ScanCode == SCANCODE_ESC) {
                    needed = false;
                }
            }
            */
        }
    }

    kargs->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    while (true) arch_cpu_halt();
    //__builtin_unreachable();
}

void update_text(EFI_TIME* time) {
    sprintf(text1,
           "%u-%c%u-%c%u",
           time->Year,
           time->Month  < 10 ? '0' : '\0', time->Month,
           time->Day    < 10 ? '0' : '\0', time->Day);
    sprintf(text2,
           "%c%u:%c%u:%c%u",
           time->Hour   < 10 ? '0' : '\0', time->Hour,
           time->Minute < 10 ? '0' : '\0', time->Minute,
           time->Second < 10 ? '0' : '\0', time->Second);
}

void line_feed(Bitmap_Font *font) {
    if (y + font->height < yres - font->height) y += font->height;
    else {
        uint32_t char_line_px    = xres * font->height;
        uint32_t char_line_bytes = char_line_px * 4;
        uint32_t char_lines      = yres / font->height;

        memcpy(fb, fb + char_line_px, char_line_bytes * (char_lines-1)); 

        uint32_t px = ((yres / font->height) - 1) * font->height * xres; 
        for (uint32_t i = 0; i < char_line_px; i++)
            fb[px++] = text_bg_color;
    }
}

void print_string(char *string, Bitmap_Font *font) {
    uint32_t glyph_size = ((font->width + 7) / 8) * font->height;
    uint32_t glyph_width_bytes = (font->width + 7) / 8;
    for (char c = *string++; c != '\0'; c = *string++) {
        if (c == '\r') { x = 0; continue; }
        if (c == '\n') { line_feed(font); continue; }

        uint8_t *glyph = &font->glyphs[c * glyph_size];

        for (uint32_t i = 0; i < font->height; i++) {
            uint64_t mask = 1 << (font->width-1);
            uint64_t bytes = font->left_col_first ? 
                             ((uint64_t)glyph[0] << 56) |
                             ((uint64_t)glyph[1] << 48) |
                             ((uint64_t)glyph[2] << 40) | 
                             ((uint64_t)glyph[3] << 32) |
                             ((uint64_t)glyph[4] << 24) |
                             ((uint64_t)glyph[5] << 16) | 
                             ((uint64_t)glyph[6] <<  8) |
                             ((uint64_t)glyph[7] <<  0) 
                             : *(uint64_t *)glyph;
            for (uint32_t px = 0; px < font->width; px++) {
                //uint32_t x_1 = x/2;
                //uint32_t y_1 = y/2;
                fb[(y*2)*xres + (x*2)] = bytes & mask ? text_fg_color : text_bg_color;
                fb[(y*2)*xres + (x*2)+1] = bytes & mask ? text_fg_color : text_bg_color;
                fb[((y*2)+1)*xres + (x*2)] = bytes & mask ? text_fg_color : text_bg_color;
                fb[((y*2)+1)*xres + (x*2)+1] = bytes & mask ? text_fg_color : text_bg_color;
                mask >>= 1;
                x++;
            }
            y++;
            x -= font->width;
            glyph += glyph_width_bytes;
        }

        y -= font->height;      
        if (x + font->width < (xres/2) - font->width) x += font->width; 
        else {
            x = 0;
            line_feed(font);
        }
    }
}
