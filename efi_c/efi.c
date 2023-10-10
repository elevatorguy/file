#include <stdarg.h>
#include "efi.h"

// -----------------
// Global macros
// -----------------
#define ARRAY_SIZE(x) (sizeof (x) / sizeof (x)[0])

// -----------------
// Global constants
// -----------------
#define SCANCODE_UP_ARROW   0x1
#define SCANCODE_DOWN_ARROW 0x2
#define SCANCODE_ESC        0x17

#define DEFAULT_FG_COLOR EFI_YELLOW
#define DEFAULT_BG_COLOR EFI_BLUE

#define HIGHLIGHT_FG_COLOR EFI_BLUE
#define HIGHLIGHT_BG_COLOR EFI_CYAN

// -----------------
// Global variables
// -----------------
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cout = NULL;  // Console output
EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *cin  = NULL;  // Console input
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cerr = NULL;  // Console output - stderr
EFI_BOOT_SERVICES    *bs;   // Boot services
EFI_RUNTIME_SERVICES *rs;   // Runtime services
EFI_HANDLE image = NULL;    // Image handle

// ====================
// Set global vars
// ====================
void init_global_variables(EFI_HANDLE handle, EFI_SYSTEM_TABLE *systable) {
    cout = systable->ConOut;
    cin = systable->ConIn;
    //cerr = systable->StdErr; // TODO: Research why this does not work in emulation, nothing prints
    cerr = cout;    // TEMP: Use stdout for error printing also
    bs = systable->BootServices;
    rs = systable->RuntimeServices;
    image = handle;
}

// ================================
// Print a number to stderr
// ================================
BOOLEAN eprint_number(UINTN number, UINT8 base, BOOLEAN is_signed) {
    const CHAR16 *digits = u"0123456789ABCDEF";
    CHAR16 buffer[24];  // Hopefully enough for UINTN_MAX (UINT64_MAX) + sign character
    UINTN i = 0;
    BOOLEAN negative = FALSE;

    if (base > 16) {
        cerr->OutputString(cerr, u"Invalid base specified!\r\n");
        return FALSE;    // Invalid base
    }

    // Only use and print negative numbers if decimal and signed True
    if (base == 10 && is_signed && (INTN)number < 0) {
       number = -(INTN)number;  // Get absolute value of correct signed value to get digits to print
       negative = TRUE;
    }

    do {
       buffer[i++] = digits[number % base];
       number /= base;
    } while (number > 0);

    switch (base) {
        case 2:
            // Binary
            buffer[i++] = u'b';
            buffer[i++] = u'0';
            break;

        case 8:
            // Octal
            buffer[i++] = u'o';
            buffer[i++] = u'0';
            break;

        case 10:
            // Decimal
            if (negative) buffer[i++] = u'-';
            break;

        case 16:
            // Hexadecimal
            buffer[i++] = u'x';
            buffer[i++] = u'0';
            break;

        default:
            // Maybe invalid base, but we'll go with it (no special processing)
            break;
    }

    // NULL terminate string
    buffer[i--] = u'\0';

    // Reverse buffer before printing
    for (UINTN j = 0; j < i; j++, i--) {
        // Swap digits
        UINTN temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }

    // Print number string
    cerr->OutputString(cerr, buffer);

    return TRUE;
}

// ================================
// Print a number to stdout
// ================================
BOOLEAN print_number(UINTN number, UINT8 base, BOOLEAN is_signed) {
    const CHAR16 *digits = u"0123456789ABCDEF";
    CHAR16 buffer[24];  // Hopefully enough for UINTN_MAX (UINT64_MAX) + sign character
    UINTN i = 0;
    BOOLEAN negative = FALSE;

    if (base > 16) {
        cerr->OutputString(cerr, u"Invalid base specified!\r\n");
        return FALSE;    // Invalid base
    }

    // Only use and print negative numbers if decimal and signed True
    if (base == 10 && is_signed && (INTN)number < 0) {
       number = -(INTN)number;  // Get absolute value of correct signed value to get digits to print
       negative = TRUE;
    }

    do {
       buffer[i++] = digits[number % base];
       number /= base;
    } while (number > 0);

    switch (base) {
        case 2:
            // Binary
            buffer[i++] = u'b';
            buffer[i++] = u'0';
            break;

        case 8:
            // Octal
            buffer[i++] = u'o';
            buffer[i++] = u'0';
            break;

        case 10:
            // Decimal
            if (negative) buffer[i++] = u'-';
            break;

        case 16:
            // Hexadecimal
            buffer[i++] = u'x';
            buffer[i++] = u'0';
            break;

        default:
            // Maybe invalid base, but we'll go with it (no special processing)
            break;
    }

    // NULL terminate string
    buffer[i--] = u'\0';

    // Reverse buffer before printing
    for (UINTN j = 0; j < i; j++, i--) {
        // Swap digits
        UINTN temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
    }

    // Print number string
    cout->OutputString(cout, buffer);

    return TRUE;
}

// ====================================
// Print formatted strings to stderr
// ====================================
bool eprintf(CHAR16 *fmt, ...) {
    bool result = true;
    CHAR16 charstr[2];    // TODO: Replace initializing this with memset and use = { } initializer
    va_list args;

    va_start(args, fmt);

    // Initialize buffers
    charstr[0] = u'\0', charstr[1] = u'\0';

    // Print formatted string values
    for (UINTN i = 0; fmt[i] != u'\0'; i++) {
        if (fmt[i] == u'%') {
            i++;

            // Grab next argument type from input args, and print it
            switch (fmt[i]) {
                case u's': {
                    // Print CHAR16 string; printf("%s", string)
                    CHAR16 *string = va_arg(args, CHAR16*);
                    cerr->OutputString(cerr, string);
                }
                break;

                case u'd': {
                    // Print INT32; printf("%d", number_int32)
                    INT32 number = va_arg(args, INT32);
                    //print_int(number);
                    eprint_number(number, 10, TRUE);
                }
                break;

                case u'x': {
                    // Print hex UINTN; printf("%x", number_uintn)
                    UINTN number = va_arg(args, UINTN);
                    //print_hex(number);
                    eprint_number(number, 16, FALSE);
                }
                break;

                default:
                    cerr->OutputString(cerr, u"Invalid format specifier: %");
                    charstr[0] = fmt[i];
                    cerr->OutputString(cerr, charstr);
                    cerr->OutputString(cerr, u"\r\n");
                    result = false;
                    goto end;
                    break;
            }
        } else {
            // Not formatted string, print next character
            charstr[0] = fmt[i];
            cerr->OutputString(cerr, charstr);
        }
    }

end:
    va_end(args);

    return result;
}

// ===================================
// Print formatted strings to stdout
// ===================================
bool printf(CHAR16 *fmt, ...) {
    bool result = true;
    CHAR16 charstr[2];    // TODO: Replace initializing this with memset and use = { } initializer
    va_list args;

    va_start(args, fmt);

    // Initialize buffers
    charstr[0] = u'\0', charstr[1] = u'\0';

    // Print formatted string values
    for (UINTN i = 0; fmt[i] != u'\0'; i++) {
        if (fmt[i] == u'%') {
            i++;

            // Grab next argument type from input args, and print it
            switch (fmt[i]) {
                case u's': {
                    // Print CHAR16 string; printf("%s", string)
                    CHAR16 *string = va_arg(args, CHAR16*);
                    cout->OutputString(cout, string);
                }
                break;

                case u'd': {
                    // Print INT32; printf("%d", number_int32)
                    INT32 number = va_arg(args, INT32);
                    //print_int(number);
                    print_number(number, 10, TRUE);
                }
                break;

                case u'x': {
                    // Print hex UINTN; printf("%x", number_uintn)
                    UINTN number = va_arg(args, UINTN);
                    //print_hex(number);
                    print_number(number, 16, FALSE);
                }
                break;

                case u'u': {
                    // Print UINT32; printf("%u", number_uint32)
                    UINT32 number = va_arg(args, UINT32);
                    print_number(number, 10, FALSE);
                }
                break;

                default:
                    cout->OutputString(cout, u"Invalid format specifier: %");
                    charstr[0] = fmt[i];
                    cout->OutputString(cout, charstr);
                    cout->OutputString(cout, u"\r\n");
                    result = false;
                    goto end;
                    break;
            }
        } else {
            // Not formatted string, print next character
            charstr[0] = fmt[i];
            cout->OutputString(cout, charstr);
        }
    }

end:
    va_end(args);

    return result;
}

// ====================
// Get key from user
// ====================
EFI_INPUT_KEY get_key(void) {
    EFI_EVENT events[1];
    EFI_INPUT_KEY key;

    key.ScanCode = 0;
    key.UnicodeChar = u'\0';

    events[0] = cin->WaitForKey;
    UINTN index = 0;
    bs->WaitForEvent(1, events, &index);

    if (index == 0) cin->ReadKeyStroke(cin, &key);

    return key;
}

// ====================
// Set Text Mode
// ====================
EFI_STATUS set_text_mode(void) {
    // Overall screen loop
    while (true) {
        cout->ClearScreen(cout);

        // Write String
        cout->OutputString(cout, u"Text mode information:\r\n");
        UINTN max_cols = 0, max_rows = 0;

        // Get current text mode's column and row counts
        cout->QueryMode(cout, cout->Mode->Mode, &max_cols, &max_rows);

        printf(u"Max Mode: %d\r\n"
               u"Current Mode: %d\r\n"
               u"Attribute: %x\r\n" 
               u"CursorColumn: %d\r\n"
               u"CursorRow: %d\r\n"
               u"CursorVisible: %d\r\n"
               u"Columns: %d\r\n"
               u"Rows: %d\r\n\r\n",
               cout->Mode->MaxMode,
               cout->Mode->Mode,
               cout->Mode->Attribute,
               cout->Mode->CursorColumn,
               cout->Mode->CursorRow,
               cout->Mode->CursorVisible,
               max_cols,
               max_rows);

        cout->OutputString(cout, u"Available text modes:\r\n");

        // Print other text mode infos
        const INT32 max = cout->Mode->MaxMode;
        for (INT32 i = 0; i < max; i++) {
            cout->QueryMode(cout, i, &max_cols, &max_rows);
            printf(u"Mode #: %d, %dx%d\r\n", i, max_cols, max_rows);
        }

        // Get number from user
        while (true) {
            static UINTN current_mode = 0;
            current_mode = cout->Mode->Mode;

            for (UINTN i = 0; i < 79; i++) printf(u" ");
            printf(u"\rSelect Text Mode # (0-%d): %d", max, current_mode);

            // Move cursor left by 1, to overwrite the mode #
            cout->SetCursorPosition(cout, cout->Mode->CursorColumn-1, cout->Mode->CursorRow);

            EFI_INPUT_KEY key = get_key();

            // Get key info
            CHAR16 cbuf[2];
            cbuf[0] = key.UnicodeChar;
            cbuf[1] = u'\0';
            //printf(u"Scancode: %x, Unicode Char: %s\r", key.ScanCode, cbuf);

            // Process keystroke
            printf(u"%s ", cbuf);

            if (key.ScanCode == SCANCODE_ESC) {
                // Go back to main menu
                return EFI_SUCCESS;
            }

            // Choose text mode & redraw screen
            current_mode = key.UnicodeChar - u'0';
            EFI_STATUS status = cout->SetMode(cout, current_mode);
            if (EFI_ERROR(status)) {
                // Handle errors
                if (status == EFI_DEVICE_ERROR) { 
                    eprintf(u"ERROR: %x; Device Error", status);
                } else if (status == EFI_UNSUPPORTED) {
                    eprintf(u"ERROR: %x; Mode # is invalid", status);
                }
                eprintf(u"\r\nPress any key to select again", status);
                get_key();

            } 

            // Set new mode, redraw screen from outer loop
            break;
        }
    }

    return EFI_SUCCESS;
}

// ====================
// Set Graphics Mode
// ====================
EFI_STATUS set_graphics_mode(void) {
    // Get GOP protocol via LocateProtocol()
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID; 
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info = NULL;
    UINTN mode_info_size = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    EFI_STATUS status = 0;

    status = bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);
    if (EFI_ERROR(status)) {
        eprintf(u"\r\nERROR: %x; Could not locate GOP! :(\r\n", status);
        return status;
    }

    // Overall screen loop
    while (true) {
        cout->ClearScreen(cout);

        // Write String
        printf(u"Graphics mode information:\r\n");

        // Get current GOP mode information
        status = gop->QueryMode(gop, 
                                gop->Mode->Mode, 
                                &mode_info_size, 
                                &mode_info);

        if (EFI_ERROR(status)) {
            eprintf(u"\r\nERROR: %x; Could not Query GOP Mode %u\r\n", status, gop->Mode->Mode);
            return status;
        }

        // Print info
        printf(u"Max Mode: %d\r\n"
               u"Current Mode: %d\r\n"
               u"WidthxHeight: %ux%u\r\n"
               u"Framebuffer address: %x\r\n"
               u"Framebuffer size: %u\r\n"
               u"PixelFormat: %d\r\n"
               u"PixelsPerScanLine: %u\r\n",
               gop->Mode->MaxMode,
               gop->Mode->Mode,
               mode_info->HorizontalResolution, mode_info->VerticalResolution,
               gop->Mode->FrameBufferBase,
               gop->Mode->FrameBufferSize,
               mode_info->PixelFormat,
               mode_info->PixelsPerScanLine);

        cout->OutputString(cout, u"\r\nAvailable GOP modes:\r\n");

        // Print other text mode infos
        const UINT32 max = gop->Mode->MaxMode;
        for (UINT32 i = 0; i < max; i++) {
            gop->QueryMode(gop, i, &mode_info_size, &mode_info);

            printf(u"Mode #: %d, %dx%d\r\n", 
                   i, mode_info->HorizontalResolution, mode_info->VerticalResolution);
        }

        // TODO: DEBUGGING
        //while (1) ;

        // Get input from user 
        while (true) {
            //static UINTN current_mode = 0;

            EFI_INPUT_KEY key = get_key();

            // Get key info
            CHAR16 cbuf[2];
            cbuf[0] = key.UnicodeChar;
            cbuf[1] = u'\0';
            //printf(u"Scancode: %x, Unicode Char: %s\r", key.ScanCode, cbuf);

            // Process keystroke
            printf(u"%s ", cbuf);

            if (key.ScanCode == SCANCODE_ESC) {
                // Go back to main menu
                return EFI_SUCCESS;
            }

            // Choose text mode & redraw screen
            //current_mode = key.UnicodeChar - u'0';
            //EFI_STATUS status = cout->SetMode(cout, current_mode);
            //if (EFI_ERROR(status)) {
            //    // Handle errors
            //    if (status == EFI_DEVICE_ERROR) { 
            //        eprintf(u"ERROR: %x; Device Error", status);
            //    } else if (status == EFI_UNSUPPORTED) {
            //        eprintf(u"ERROR: %x; Mode # is invalid", status);
            //    }
            //    eprintf(u"\r\nPress any key to select again", status);
            //    get_key();

            //} 

            // Set new mode, redraw screen from outer loop
            break;
        }
    }

    return EFI_SUCCESS;
}

// ====================
// Entry Point
// ====================
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Initialize global variables
    init_global_variables(ImageHandle, SystemTable);

    // Reset Console Inputs/Outputs
    cin->Reset(cin, FALSE);
    cout->Reset(cout, FALSE);
    cout->Reset(cerr, FALSE);

    // Set text colors - foreground, background
    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));

    // Screen loop
    bool running = true;
    while (running) {
        // Menu text on screen
        const CHAR16 *menu_choices[] = {
            u"Set Text Mode",
            u"Set Graphics Mode",
        };

        // Functions to call for each menu option
        EFI_STATUS (*menu_funcs[])(void) = {
            set_text_mode,
            set_graphics_mode,
        };

        // Clear console output; clear screen to background color and
        //   set cursor to 0,0
        cout->ClearScreen(cout);

        // Get current text mode ColsxRows values
        UINTN cols = 0, rows = 0;
        cout->QueryMode(cout, cout->Mode->Mode, &cols, &rows);

        // Print keybinds at bottom of screen
        cout->SetCursorPosition(cout, 0, rows-3);
        printf(u"Up/Down Arrow = Move cursor\r\n"
               u"Enter = Select\r\n"
               u"Escape = Shutdown");

        // Print menu choices
        // Highlight first choice as initial choice
        cout->SetCursorPosition(cout, 0, 0);
        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
        printf(u"%s", menu_choices[0]);

        // Print rest of choices
        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
        for (UINTN i = 1; i < ARRAY_SIZE(menu_choices); i++)
            printf(u"\r\n%s", menu_choices[i]);

        // Get cursor row boundaries
        INTN min_row = 0, max_row = cout->Mode->CursorRow;

        // Input loop
        cout->SetCursorPosition(cout, 0, 0);
        bool getting_input = true;
        while (getting_input) {
            INTN current_row = cout->Mode->CursorRow;
            EFI_INPUT_KEY key = get_key();

            // Process input
            switch (key.ScanCode) {
                case SCANCODE_UP_ARROW:
                    if (current_row-1 >= min_row) {
                        // De-highlight current row, move up 1 row, highlight new row
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                        printf(u"%s\r", menu_choices[current_row]);

                        current_row--;
                        cout->SetCursorPosition(cout, 0, current_row);
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"%s\r", menu_choices[current_row]);

                        // Reset colors
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    }
                    break;

                case SCANCODE_DOWN_ARROW:
                    if (current_row+1 <= max_row) {
                        // De-highlight current row, move down 1 row, highlight new row
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                        printf(u"%s\r", menu_choices[current_row]);

                        current_row++;
                        cout->SetCursorPosition(cout, 0, current_row);
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"%s\r", menu_choices[current_row]);

                        // Reset colors
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    }
                    break;

                case SCANCODE_ESC:
                    // Escape key: power off
                    rs->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

                    // !NOTE!: This should not return, system should power off
                    break;

                default:
                    if (key.UnicodeChar == u'\r') {
                        // Enter key, select choice
                        EFI_STATUS return_status = menu_funcs[current_row]();
                        if (EFI_ERROR(return_status)) {
                            eprintf(u"ERROR %x\r\n; Press any key to go back...", return_status);
                            get_key();
                        }

                        // Will leave input loop and reprint main menu
                        getting_input = false; 
                    }
                    break;
            }
        }
    }

    // End program
    return EFI_SUCCESS;
}
