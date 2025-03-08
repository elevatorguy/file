#include "efi.h"
#include "efi_lib.h"
#include "arch/x86_64/x86_64.h"

// PSF Font types
// Adapted from https://wiki.osdev.org/PC_Screen_Font
#define PSF2_FONT_MAGIC 0x864ab572

#define ARGB_LIGHTGRAY (0xFFDDDDDD)
#define ARGB_RED (0xFFCC2222)
#define ARGB_BLUE (0xFF2222CC)
#define ARGB_DARKGRAY (0xFF222222)

uint32_t* fb = NULL; // Framebuffer
uint32_t xres = 0;   // X/Horizontal resolution of framebuffer
uint32_t yres = 0;   // Y/Vertical resolution of framebuffer
uint32_t x = 0;      // X offset into framebuffer
uint32_t y = 0;      // Y offset into framebuffer

const uint32_t text_fg_color = ARGB_RED;
const uint32_t text_bg_color = ARGB_DARKGRAY;

void print_string(char* string, Bitmap_Font* font);

typedef struct {
    UINTN        num_fonts;
    Bitmap_Font* fonts;
} Font_Info;

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    (void)ImageHandle;	// Prevent compiler warning

    SystemTable->BootServices->SetWatchdogTimer(0, 0x10000, 0, NULL);

    // Set text to yellow fg/ green bg
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut,
            EFI_TEXT_ATTR(EFI_RED,EFI_BLACK));

    // Clear screen to bg color
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"from OutputString.\r\n");
    //printf_c16(u"from printf.\r\n");

    EFI_HII_PACKAGE_LIST_HEADER* pkg_list = NULL;
    EFI_STATUS status = EFI_SUCCESS;

    Font_Info info = {
        .num_fonts            = 0,
        .fonts                = NULL,
    };

    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"from OutputString again.\r\n");
    //printf_c16(u"from printf again.\r\n");

    info.num_fonts = 2;
    status = SystemTable->BootServices->AllocatePool(EfiLoaderData,
                              info.num_fonts * sizeof *info.fonts,
                              (VOID **)&info.fonts);
    if (EFI_ERROR(status)) {
        error(status, u"Could not allocate buffer for font info.\r\n");
        goto cleanup;
    }

    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"3\r\n");

    UINTN psf_size = 0;
	VOID* psf_font = 0;

	// Try to load font from ESP first (if present, a data partition font becomes secondary)
    CHAR16* font_file = u"\\EFI\\BOOT\\TER-132N.PSF";
    psf_font = read_esp_file_to_buffer(font_file, &psf_size);
    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"4\r\n");
    char* psf_name = "ter-132n.psf";
    if (!psf_font) {
		//TODO: if ESP is whole disk then that's that - abort

	    // Get PSF font file for another bitmap font to use;
	    //   this one should be stored in the disk image's data partition
	    psf_font = read_data_partition_file_to_buffer(psf_name, false, &psf_size);
	}

    if (psf_font) {
        PSF2_Header *psf2_hdr = psf_font;
        info.fonts[1] = (Bitmap_Font){
            .name            = psf_name,
            .width           = psf2_hdr->width,
            .height          = psf2_hdr->height,
            .left_col_first  = true,                   // Pixels in memory are stored left to right
            .num_glyphs      = psf2_hdr->num_glyphs,
            .glyphs          = (uint8_t *)(psf2_hdr+1),
        };
    }

    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"5\r\n");

    // Get GOP protocol via LocateProtocol()
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info = NULL;
    UINTN mode_info_size = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    UINTN mode_index = 0;   // Current mode within entire menu of GOP mode choices;

    status = SystemTable->BootServices->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);
    if (EFI_ERROR(status)) {
        error(status, u"Could not locate GOP! :(\r\n");
        goto cleanup;
    }
    if((*gop).Mode != NULL) {
    	mode_index = (*(*gop).Mode).Mode;
    }

    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"6\r\n");

    gop->QueryMode(gop, mode_index, &mode_info_size, &mode_info);

    fb = (uint32_t*)gop->Mode->FrameBufferBase;

    xres = mode_info->PixelsPerScanLine;
    yres = mode_info->VerticalResolution;

    // Clear screen to solid color
    UINTN color = ARGB_BLUE;
    for (y = 0; y < yres; y++)
        for (x = 0; x < xres; x++)
            fb[y*xres + x] = color;

    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"7\r\n");

    // Print test string(s)
    x = y = 0;
    Bitmap_Font* font1 = &info.fonts[0];
    Bitmap_Font* font2 = &info.fonts[1];
    print_string("Hello, bitmap font world!", font1);
    print_string("\r\nFont 1 Name: ", font1);
    print_string(font1->name, font1);
    print_string("\r\nFont 2 Name: ", font2);
    print_string(font2->name, font2);

    SystemTable->ConOut->OutputString(SystemTable->ConOut, u"8\r\n");

    // Test runtime services by waiting a few seconds and then shutting down
    EFI_TIME old_time = {0}, new_time = {0};
    EFI_TIME_CAPABILITIES time_cap = {0};
    UINTN i = 0;
    while (i < 10) {
        SystemTable->RuntimeServices->GetTime(&new_time, &time_cap);
        if (old_time.Second != new_time.Second) {
            old_time.Second = new_time.Second;
			//print_string(itoa(new_time.Second,s,10), font1);
            i++;
        }
    }

    if (psf_font) SystemTable->BootServices->FreePool(psf_font); // Free memory for data partition file
    cleanup:
    if (info.fonts) {
        // Free memory for font glyph(s) info
        for (UINTN i = 0; i < info.num_fonts; i++)
            SystemTable->BootServices->FreePool(info.fonts[i].glyphs);

        SystemTable->BootServices->FreePool(info.fonts);   // Free memory for font(s) array info
    }

    // Set text to red fg/ black bg
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut,
            EFI_TEXT_ATTR(EFI_RED,EFI_BLACK));

    SystemTable->ConOut->OutputString(SystemTable->ConOut,
            u"Press any key to shutdown...");

    // Wait until keypress, then return
    EFI_INPUT_KEY key;
    while (SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &key) != EFI_SUCCESS)
        ;

    // Shutdown, does not return
    SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

    // Should never get here
    return EFI_SUCCESS;
}

// ======================================================================
// Print a line feed visually (go down 1 line and/or scroll the screen)
// ======================================================================
void line_feed(Bitmap_Font *font) {
    // Can we draw another line of characters below current line?
    if (y + font->height < yres - font->height) y += font->height; // Yes, go down 1 line
    else {
        // No more room, move all lines on screen 1 row up by overwriting 1st line with lines 2+
        // NOTE: This is probably slow due to reading and writing to framebuffer
        uint32_t char_line_px    = xres * font->height;
        uint32_t char_line_bytes = char_line_px * 4;    // Assuming ARGB8888
        uint32_t char_lines      = yres / font->height;

        memcpy(fb, fb + char_line_px, char_line_bytes * (char_lines-1));

        // Blank out last row by making all pixels the background color
        // Get X,Y start of last character line
        uint32_t px = ((yres / font->height) - 1) * font->height * xres;
        for (uint32_t i = 0; i < char_line_px; i++)
            fb[px++] = text_bg_color;
    }
}

// ===========================================================
// Print a bitmapped font string to the screen (framebuffer)
// ===========================================================
void print_string(char *string, Bitmap_Font *font) {
    uint32_t glyph_size = ((font->width + 7) / 8) * font->height;   // Size of all glyph lines
    uint32_t glyph_width_bytes = (font->width + 7) / 8;             // Size of 1 line of a glyph
    for (char c = *string++; c != '\0'; c = *string++) {
        if (c == '\r') { x = 0; continue; }             // Carriage return (CR)
        if (c == '\n') { line_feed(font); continue; }   // Line Feed (LF)

        uint8_t *glyph = &font->glyphs[c * glyph_size];

        // Draw each line of glyph
        for (uint32_t i = 0; i < font->height; i++) {
            // e.g. for glyph 'F':
            // Left to right 1 bits:      Right to left 1 bits:
            // mask starts here ------v   mask starts here ------v
            // data starts here ->11111   data starts here ->11111
            //                    1                              1
            //                    111                          111
            //                    1                              1
            //                    1                              1
            // The bitmask starts at the font width and goes down to 0, which ends up
            //   drawing the bits mirrored from how they are stored in memory.
            //   We want to byteswap the bits if it is already stored left to right, so that
            //   drawing it mirrored will be left to right visually, not right to left.
            // NOTE: This only works for fonts <= 64 pixels in width, and assumes font data
            //    is padded with 0s to not overflow, if last line of character data is less than 8 bytes.
            uint64_t mask = 1 << (font->width-1);
            uint64_t bytes = font->left_col_first ?
                             ((uint64_t)glyph[0] << 56) | // Byteswap character data; can
                             ((uint64_t)glyph[1] << 48) | //   also use __builtin_bswap64(*(uint64_t *)glyph)
                             ((uint64_t)glyph[2] << 40) |
                             ((uint64_t)glyph[3] << 32) |
                             ((uint64_t)glyph[4] << 24) |
                             ((uint64_t)glyph[5] << 16) |
                             ((uint64_t)glyph[6] <<  8) |
                             ((uint64_t)glyph[7] <<  0)
                             : *(uint64_t *)glyph;   // Else pixels are stored right to left
            for (uint32_t px = 0; px < font->width; px++) {
                fb[y*xres + x] = bytes & mask ? text_fg_color : text_bg_color;
                mask >>= 1;
                x++;            // Next pixel of character
            }
            y++;                // Next line of character
            x -= font->width;   // Back up to start of character
            glyph += glyph_width_bytes;
        }

        // Go to start of next character, top left pixel
        y -= font->height;
        if (x + font->width < xres - font->width) x += font->width;
        else {
            // Wrap text to next line with a CR/LF
            x = 0;
            line_feed(font);
        }
    }
}
