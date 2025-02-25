/*
 * NOTE: void* fields in structs = not implemented!!
 */

// __has_include is clang/gcc defined; But should be in C standard C2X
#if __has_include(<uchar.h>) 
#include <uchar.h>
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // NULL

#ifndef _UCHAR_H
typedef uint_least16_t char16_t;
#endif

// Common UEFI Data Types: UEFI Spec 2.10 section 2.3.1
typedef uint8_t     UINT8;
typedef uint16_t    UINT16;
typedef uint32_t    UINT32;
typedef uint64_t    UINT64;
typedef uint64_t    UINTN;
typedef char16_t    CHAR16;	// UTF-16, but should use UCS-2 code points 0x0000-0xFFFF
typedef void        VOID;

typedef struct {
    UINT32 TimeLow;
    UINT16 TimeMid;
    UINT16 TimeHighAndVersion;
    UINT8  ClockSeqHighAndReserved;
    UINT8  ClockSeqLow;
    UINT8  Node[6];
} __attribute__ ((packed)) EFI_GUID;

typedef UINTN       EFI_STATUS;
typedef VOID*       EFI_HANDLE;

// Taken from EDKII at
// https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Base.h
#define IN
#define OUT
#define OPTIONAL
#define CONST const

// Taken from gnu-efi at
// https://github.com/vathpela/gnu-efi/blob/master/inc/x86_64/efibind.h
#define EFIAPI __attribute__((ms_abi))  // x86_64 Microsoft calling convention

// EFI_STATUS codes: UEFI Spec 2.10 Appendix D
#define EFI_SUCCESS 0

// EFI Simple Text Input Protocol: UEFI Spec 2.10 section 12.3
// Forward declare struct for this to work and compile
typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef struct {
    UINT16  ScanCode;
    CHAR16  UnicodeChar;
} EFI_INPUT_KEY;

typedef 
EFI_STATUS 
(EFIAPI *EFI_INPUT_READ_KEY) (
 IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This, 
 OUT EFI_INPUT_KEY                   *Key
);

typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    void*               Reset;
    EFI_INPUT_READ_KEY  ReadKeyStroke;
    void*               WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

// EFI Simple Text Output Protocol: UEFI Spec 2.10 section 12.4
// Forward declare struct for this to work and compile
typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

// Text attributes: UEFI Spec 2.10 section 12.4.7
#define EFI_BLACK  0x00
#define EFI_BLUE   0x01
#define EFI_GREEN  0x02
#define EFI_CYAN   0x03
#define EFI_RED    0x04
#define EFI_YELLOW 0x0E
#define EFI_WHITE  0x0F

// Only use 0x00-0x07 for background with this macro!
#define EFI_TEXT_ATTR(Foreground,Background) \
    ((Foreground) | ((Background) << 4))

typedef 
EFI_STATUS 
(EFIAPI *EFI_TEXT_STRING) (
 IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
 IN CHAR16                          *String
);

typedef 
EFI_STATUS 
(EFIAPI *EFI_TEXT_SET_ATTRIBUTE) (
 IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
 IN UINTN                           Attribute
);

typedef 
EFI_STATUS 
(EFIAPI *EFI_TEXT_CLEAR_SCREEN) (
 IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
);

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void*                           Reset;
    EFI_TEXT_STRING                 OutputString;
    void*                           TestString;
    void*                           QueryMode;
    void*                           SetMode;
    EFI_TEXT_SET_ATTRIBUTE          SetAttribute;
    EFI_TEXT_CLEAR_SCREEN           ClearScreen;
    void*                           SetCursorPosition;
    void*                           EnableCursor;
    void*                           Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef enum {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

typedef 
VOID 
(EFIAPI *EFI_RESET_SYSTEM) (
   IN EFI_RESET_TYPE ResetType,      
   IN EFI_STATUS     ResetStatus,   
   IN UINTN          DataSize,     
   IN VOID           *ResetData OPTIONAL
);

// EFI Table Header: UEFI Spec 2.10 section 4.2
typedef struct {
    UINT64  Signature;
    UINT32  Revision;
    UINT32  HeaderSize;
    UINT32  CRC32;
    UINT32  Reserved;
} EFI_TABLE_HEADER;

typedef struct {
    EFI_TABLE_HEADER Hdr;

    // Time Services
    void*                           GetTime; 
    void*                           SetTime; 
    void*                           GetWakeupTime; 
    void*                           SetWakeupTime; 

    // Virtual Memory Services
    void*                           SetVirtualAddressMap;
    void*                           ConvertPointer;

    // Variable Services
    void*                           GetVariable;
    void*                           GetNextVariableName;
    void*                           SetVariable;

    // Miscellaneous Services
    void*                           GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM                ResetSystem;

    // UEFI 2.0 Capsule Services
    void*                           UpdateCapsule;
    void*                           QueryCapsuleCapabilities;

    // Miscellaneous UEFI 2.0 Service
    void*                           QueryVariableInfo; 
} EFI_RUNTIME_SERVICES;

// EFI System Table: UEFI Spec 2.10 section 4.3
typedef struct {
    EFI_TABLE_HEADER                Hdr;

    void*                           FirmwareVendor;
    UINT32                          FirmwareRevision;
    void*                           ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL 	*ConIn;
	void*                           ConsoleOutHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
	void*                           StandardErrorHandle;
	void*                           StdErr;
	EFI_RUNTIME_SERVICES            *RuntimeServices;
	void*                           BootServices;
	UINTN                           NumberOfTableEntries;
	void*                           ConfigurationTable;
} EFI_SYSTEM_TABLE;

// 34.8.1 EFI_HII_DATABASE_PROTOCOL
#define EFI_HII_DATABASE_PROTOCOL_GUID \
{0xef9fc172, 0xa1b2, 0x4693,\
0xb3, 0x27, {0x6d, 0x32, 0xfc, 0x41, 0x60, 0x42}}

// HII PACKAGE HEADER: UEFI Spec 2.10A section 33.3.1.1
typedef struct {
    UINT32 Length:24;
    UINT32 Type:8;
    // UINT8 Data[...];
} EFI_HII_PACKAGE_HEADER;

// HII PACKAGE TYPES
#define EFI_HII_PACKAGE_TYPE_ALL          0x00
#define EFI_HII_PACKAGE_TYPE_GUID         0x01
#define EFI_HII_PACKAGE_FORMS             0x02
#define EFI_HII_PACKAGE_STRINGS           0x04
#define EFI_HII_PACKAGE_FONTS             0x05
#define EFI_HII_PACKAGE_IMAGES            0x06
#define EFI_HII_PACKAGE_SIMPLE_FONTS      0x07
#define EFI_HII_PACKAGE_DEVICE_PATH       0x08
#define EFI_HII_PACKAGE_KEYBOARD_LAYOUT   0x09
#define EFI_HII_PACKAGE_ANIMATIONS        0x0A
#define EFI_HII_PACKAGE_END               0xDF
#define EFI_HII_PACKAGE_TYPE_SYSTEM_BEGIN 0xE0
#define EFI_HII_PACKAGE_TYPE_SYSTEM_END   0xFF

// HII PACKAGE LIST HEADER: UEFI Spec 2.10A section 33.3.1.2
typedef struct {
    EFI_GUID PackageListGuid;
    UINT32   PackagLength;
} EFI_HII_PACKAGE_LIST_HEADER;

// HII SIMPLE FONT PACKAGE HEADER: UEFI Spec 2.10A section 33.3.2
typedef struct {
    EFI_HII_PACKAGE_HEADER Header;
    UINT16                 NumberOfNarrowGlyphs;
    UINT16                 NumberOfWideGlyphs;
    // EFI_NARROW_GLYPH NarrowGlyphs[];
    // EFI_WIDE_GLYPH WideGlyphs[];
} EFI_HII_SIMPLE_FONT_PACKAGE_HDR;

// Contents of EFI_NARROW_GLYPH.Attributes
#define EFI_GLYPH_NON_SPACING 0x01
#define EFI_GLYPH_WIDE        0x02
#define EFI_GLYPH_HEIGHT      19
#define EFI_GLYPH_WIDTH       8

// EFI NARROW GLYPH: UEFI Spec 2.10A section 33.3.2.2   
// should be 8x19 monospaced bitmap font
typedef struct {
    CHAR16 UnicodeWeight;
    UINT8  Attributes;
    UINT8  GlyphCol1[EFI_GLYPH_HEIGHT];
} EFI_NARROW_GLYPH;

// EFI WIDE GLYPH: UEFI Spec 2.10A section 33.3.2.3   
// should be 16x19 monospaced bitmap font
typedef struct {
    CHAR16 UnicodeWeight;
    UINT8  Attributes;
    UINT8  GlyphCol1[EFI_GLYPH_HEIGHT];
    UINT8  GlyphCol2[EFI_GLYPH_HEIGHT];
    UINT8  Pad[3];
} EFI_WIDE_GLYPH;

// NOTE: This is _not_ an EFI_HANDLE!
typedef void *EFI_HII_HANDLE;

// Forward declare for function declarations to work
typedef struct EFI_HII_DATABASE_PROTOCOL EFI_HII_DATABASE_PROTOCOL;

// EFI_HII_DATABASE_LIST_PACKS: UEFI Spec 2.10A section 34.8.5
typedef
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_LIST_PACKS) (
    IN CONST EFI_HII_DATABASE_PROTOCOL *This,
    IN UINT8                           PackageType,
    IN CONST EFI_GUID                  *PackageGuid,
    IN OUT UINTN                       *HandleBufferLength,
    OUT EFI_HII_HANDLE                 *Handle
);

// EFI_HII_DATABASE_EXPORT_PACKS: UEFI Spec 2.10A section 34.8.6
typedef 
EFI_STATUS
(EFIAPI *EFI_HII_DATABASE_EXPORT_PACKS) (
    IN CONST EFI_HII_DATABASE_PROTOCOL *This,
    IN EFI_HII_HANDLE                  Handle,
    IN OUT UINTN                       *BufferSize,
    OUT EFI_HII_PACKAGE_LIST_HEADER    *Buffer
);

// HII DATABASE PROTOCOL: UEFI Spec 2.10A section 34.8.1
typedef struct EFI_HII_DATABASE_PROTOCOL {
    //EFI_HII_DATABASE_NEW_PACK          NewPackageList;
    //EFI_HII_DATABASE_REMOVE_PACK       RemovePackageList;
    //EFI_HII_DATABASE_UPDATE_PACK       UpdatePackageList;
    void                               *NewPackageList;
    void                               *RemovePackageList;
    void                               *UpdatePackageList;

    EFI_HII_DATABASE_LIST_PACKS        ListPackageLists;
    EFI_HII_DATABASE_EXPORT_PACKS      ExportPackageLists;

    //EFI_HII_DATABASE_REGISTER_NOTIFY   RegisterPackageNotify;
    //EFI_HII_DATABASE_UNREGISTER_NOTIFY UnregisterPackageNotify;
    //EFI_HII_FIND_KEYBOARD_LAYOUTS      FindKeyboardLayouts;
    //EFI_HII_GET_KEYBOARD_LAYOUT        GetKeyboardLayout;
    //EFI_HII_SET_KEYBOARD_LAYOUT        SetKeyboardLayout;
    //EFI_HII_DATABASE_GET_PACK_HANDLE   GetPackageListHandle;
    void                               *RegisterPackageNotify;
    void                               *UnregisterPackageNotify;
    void                               *FindKeyboardLayouts;
    void                               *GetKeyboardLayout;
    void                               *SetKeyboardLayout;
    void                               *GetPackageListHandle;
} EFI_HII_DATABASE_PROTOCOL;
