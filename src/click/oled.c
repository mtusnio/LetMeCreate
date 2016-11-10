#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "letmecreate/click/oled.h"
#include "letmecreate/click/common.h"
#include "letmecreate/core.h"

/* Command and data registers */
const uint16_t oled_cmd_addr = 0x3C;
const uint16_t oled_data_addr = 0x3D;

/* The default monospace font lookup table */
static const uint8_t oled_char_table[][22] = {
    {0x0, 0x0, 0x0, 0x0, 0xfe, 0xfe, 0xfe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x33, 0x33, 0x33, 0x0, 0x0, 0x0, 0x0},               /* ! */
    {0x0, 0x0, 0x0, 0x3e, 0x3e, 0x0, 0x0, 0x3e, 0x3e, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},                 /* " */
    {0x60, 0x6c, 0xfc, 0xf0, 0x60, 0x7c, 0xfc, 0xe0, 0x60, 0x60, 0x0, 0x0, 0x6, 0x6, 0xf, 0x3f, 0x36, 0x6, 0xf, 0x3f, 0x36, 0x6},       /* # */
    {0x0, 0x0, 0x0, 0x18, 0x8c, 0xff, 0xcc, 0xfc, 0xf8, 0x0, 0x0, 0x0, 0x0, 0x1e, 0x3f, 0x33, 0xff, 0x31, 0x31, 0x18, 0x0, 0x0},        /* $ */
    {0x0, 0x0, 0x40, 0x40, 0x80, 0xbc, 0xfe, 0x42, 0x42, 0x7e, 0x3c, 0x0, 0x1e, 0x3f, 0x21, 0x21, 0x3f, 0x1e, 0x1, 0x1, 0x1, 0x0},      /* % */
    {0x80, 0x80, 0x0, 0x4, 0xc6, 0xfe, 0xfe, 0xdc, 0x80, 0x0, 0x0, 0x23, 0x3f, 0x3c, 0x1f, 0x37, 0x31, 0x38, 0x3f, 0x1f, 0xf, 0x0},     /* & */
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x3e, 0x3e, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},                   /* ' */
    {0x0, 0x0, 0x0, 0x1, 0xf, 0xfe, 0xf8, 0xe0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0xf0, 0x7f, 0x1f, 0x7, 0x0, 0x0, 0x0},              /* ( */
    {0x0, 0x0, 0x0, 0xe0, 0xf8, 0xfe, 0xf, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7, 0x1f, 0x7f, 0xf0, 0x80, 0x0, 0x0, 0x0},              /* ) */
    {0xcc, 0xcc, 0x78, 0x78, 0xfe, 0xfe, 0x78, 0x78, 0xcc, 0xcc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0},           /* * */
    {0x0, 0x80, 0x80, 0x80, 0x80, 0xf8, 0xf8, 0x80, 0x80, 0x80, 0x80, 0x0, 0x1, 0x1, 0x1, 0x1, 0x1f, 0x1f, 0x1, 0x1, 0x1, 0x1},         /* + */
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x38, 0xf8, 0xf8, 0x80, 0x0, 0x0, 0x0},                 /* , */
    {0x0, 0x0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x0, 0x0, 0x0},               /* - */
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x38, 0x38, 0x38, 0x0, 0x0, 0x0, 0x0},                  /* . */
    {0x0, 0x2, 0xe, 0x3c, 0xf0, 0xc0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x7, 0x1f, 0x78, 0xe0, 0x80, 0x0},              /* / */
    {0x0, 0xf0, 0xfc, 0xfe, 0xe, 0xc6, 0xe, 0xfe, 0xfc, 0xf0, 0x0, 0x0, 0x7, 0x1f, 0x3f, 0x38, 0x30, 0x38, 0x3f, 0x1f, 0x7, 0x0},       /* 0 */
    {0x0, 0x0, 0x0, 0x0, 0xfe, 0xfe, 0xfe, 0x6, 0xc, 0xc, 0x0, 0x0, 0x30, 0x30, 0x30, 0x3f, 0x3f, 0x3f, 0x30, 0x30, 0x30, 0x0},         /* 1 */
    {0x0, 0x38, 0x7c, 0xfe, 0xce, 0x6, 0x6, 0x6, 0x6, 0xc, 0x0, 0x0, 0x30, 0x30, 0x30, 0x31, 0x33, 0x36, 0x3c, 0x38, 0x30, 0x0},        /* 2 */
    {0x0, 0x38, 0xbc, 0xfe, 0xc6, 0xc6, 0xc6, 0xc6, 0x6, 0xc, 0x0, 0x0, 0xf, 0x1f, 0x3f, 0x39, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0},      /* 3 */
    {0x0, 0x0, 0xfe, 0xfe, 0xfe, 0x1c, 0x78, 0xe0, 0xc0, 0x0, 0x0, 0x0, 0x6, 0x3f, 0x3f, 0x3f, 0x6, 0x6, 0x6, 0x7, 0x7, 0x0},           /* 4 */
    {0x0, 0x80, 0xc6, 0xe6, 0xe6, 0x66, 0x66, 0x7e, 0x7e, 0xfe, 0x0, 0x0, 0xf, 0x1f, 0x1f, 0x38, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0},    /* 5 */
    {0x0, 0x80, 0xcc, 0xe6, 0x66, 0x66, 0x4e, 0xfc, 0xfc, 0xf0, 0x0, 0x0, 0xf, 0x1f, 0x3f, 0x30, 0x30, 0x30, 0x3f, 0x1f, 0x7, 0x0},     /* 6 */
    {0x0, 0xe, 0x3e, 0xfe, 0xfe, 0xc6, 0x6, 0x6, 0x6, 0x6, 0x0, 0x0, 0x0, 0x0, 0x1, 0x7, 0x1f, 0x3f, 0x3c, 0x20, 0x0, 0x0},             /* 7 */
    {0x0, 0x38, 0x3c, 0xfe, 0xc6, 0xc6, 0xc6, 0xfe, 0x3c, 0x38, 0x0, 0x0, 0xf, 0x1f, 0x3f, 0x30, 0x30, 0x30, 0x3f, 0x1f, 0xf, 0x0},     /* 8 */
    {0x0, 0xf0, 0xfc, 0xfe, 0x6, 0x6, 0x6, 0xfe, 0xfc, 0xf8, 0x0, 0x0, 0x7, 0x1f, 0x1f, 0x39, 0x33, 0x33, 0x33, 0x19, 0x0, 0x0},        /* 9 */
    {0x0, 0x0, 0x0, 0x0, 0xe0, 0xe0, 0xe0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x38, 0x38, 0x38, 0x0, 0x0, 0x0, 0x0},               /* : */
    {0x0, 0x0, 0x0, 0x0, 0x38, 0x38, 0x38, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xe, 0x3e, 0x7e, 0x60, 0x0, 0x0, 0x0},               /* ; */
    {0x0, 0x18, 0x30, 0x30, 0x30, 0x60, 0x60, 0x40, 0xc0, 0xc0, 0x0, 0x0, 0xc, 0x6, 0x6, 0x6, 0x3, 0x3, 0x1, 0x1, 0x1, 0x0},            /* < */
    {0x0, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x0, 0x0, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x0},            /* = */
    {0x0, 0xc0, 0xc0, 0x40, 0x60, 0x60, 0x30, 0x30, 0x30, 0x18, 0x0, 0x0, 0x1, 0x1, 0x1, 0x3, 0x3, 0x6, 0x6, 0x6, 0xc, 0x0},            /* > */
    {0x0, 0x0, 0x1c, 0x3e, 0x7e, 0xc6, 0x86, 0x6, 0xc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x37, 0x37, 0x37, 0x0, 0x0, 0x0, 0x0},             /* ? */
    {0x0, 0xf8, 0xfc, 0x6e, 0x66, 0xe6, 0xc6, 0xc, 0x3c, 0xf8, 0xe0, 0x0, 0x4f, 0xef, 0xcc, 0xcc, 0xcf, 0xc7, 0x60, 0x78, 0x3f, 0xf},   /* @ */
    {0x0, 0x0, 0x0, 0xf0, 0xfe, 0xe, 0xfe, 0xf0, 0x0, 0x0, 0x0, 0x0, 0x30, 0x3f, 0x3f, 0x7, 0x6, 0x7, 0x3f, 0x3f, 0x30, 0x0},           /* A */
    {0x0, 0x3c, 0xfc, 0xfe, 0xc6, 0xc6, 0xc6, 0xfe, 0xfe, 0xfe, 0x0, 0x0, 0x1f, 0x1f, 0x3f, 0x30, 0x30, 0x30, 0x3f, 0x3f, 0x3f, 0x0},   /* B */
    {0x0, 0xc, 0x6, 0x6, 0x6, 0x6, 0x1e, 0xfc, 0xf8, 0xf0, 0x0, 0x0, 0x18, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x1f, 0xf, 0x7, 0x0},          /* C */
    {0x0, 0xf0, 0xfc, 0xfc, 0xe, 0x6, 0x6, 0xfe, 0xfe, 0xfe, 0x0, 0x0, 0x7, 0x1f, 0x1f, 0x38, 0x30, 0x30, 0x3f, 0x3f, 0x3f, 0x0},       /* D */
    {0x0, 0x6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xfe, 0xfe, 0xfe, 0x0, 0x0, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3f, 0x3f, 0x3f, 0x0},    /* E */
    {0x0, 0x6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xfe, 0xfe, 0xfe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x0},          /* F */
    {0x0, 0x8c, 0x86, 0x86, 0x86, 0x86, 0x1e, 0xfc, 0xf8, 0xf0, 0x0, 0x0, 0x1f, 0x3f, 0x3f, 0x31, 0x31, 0x38, 0x1f, 0xf, 0x7, 0x0},     /* G */
    {0x0, 0xfe, 0xfe, 0xfe, 0xc0, 0xc0, 0xc0, 0xfe, 0xfe, 0xfe, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x0, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x0},      /* H */
    {0x0, 0x6, 0x6, 0x6, 0xfe, 0xfe, 0xfe, 0x6, 0x6, 0x6, 0x0, 0x0, 0x30, 0x30, 0x30, 0x3f, 0x3f, 0x3f, 0x30, 0x30, 0x30, 0x0},         /* I */
    {0x0, 0xfe, 0xfe, 0xfe, 0x6, 0x6, 0x6, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0x1f, 0x3f, 0x38, 0x30, 0x30, 0x30, 0x38, 0x18, 0x0},          /* J */
    {0x2, 0x6, 0xe, 0x9c, 0xf8, 0xf0, 0xc0, 0xfe, 0xfe, 0xfe, 0x0, 0x20, 0x38, 0x3e, 0x1f, 0x7, 0x1, 0x1, 0x3f, 0x3f, 0x3f, 0x0},       /* K */
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xfe, 0xfe, 0xfe, 0x0, 0x0, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3f, 0x3f, 0x3f, 0x0},         /* L */
    {0x0, 0xfe, 0xfe, 0xfe, 0x70, 0x80, 0x70, 0xfe, 0xfe, 0xfe, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x0, 0x1, 0x0, 0x3f, 0x3f, 0x3f, 0x0},      /* M */
    {0x0, 0xfe, 0xfe, 0xfe, 0x0, 0xe0, 0x7e, 0xfe, 0xfe, 0xfe, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x1f, 0x3, 0x0, 0x3f, 0x3f, 0x3f, 0x0},      /* N */
    {0x0, 0xf0, 0xfc, 0xfe, 0xe, 0x6, 0xe, 0xfe, 0xfc, 0xf0, 0x0, 0x0, 0x7, 0x1f, 0x3f, 0x38, 0x30, 0x38, 0x3f, 0x1f, 0x7, 0x0},        /* O */
    {0x0, 0x78, 0xfc, 0xfe, 0x86, 0x86, 0x86, 0xfe, 0xfe, 0xfe, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x1, 0x1, 0x3f, 0x3f, 0x3f, 0x0},         /* P */
    {0x0, 0xf0, 0xfc, 0xfe, 0xe, 0x6, 0xe, 0xfe, 0xfc, 0xf0, 0x0, 0x0, 0x7, 0x5f, 0xff, 0x78, 0x30, 0x38, 0x3f, 0x1f, 0x7, 0x0},        /* Q */
    {0x0, 0x78, 0xfc, 0xfe, 0x86, 0x86, 0x86, 0xfe, 0xfe, 0xfe, 0x0, 0x20, 0x38, 0x3e, 0x1f, 0x7, 0x1, 0x1, 0x3f, 0x3f, 0x3f, 0x0},     /* R */
    {0x0, 0x0, 0x8c, 0xce, 0xc6, 0xc6, 0xe6, 0xfe, 0x7c, 0x38, 0x0, 0x0, 0xf, 0x1f, 0x3f, 0x31, 0x30, 0x30, 0x30, 0x38, 0x1c, 0x0},     /* S */
    {0x0, 0x6, 0x6, 0x6, 0xfe, 0xfe, 0xfe, 0x6, 0x6, 0x6, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x0, 0x0, 0x0, 0x0},               /* T */
    {0x0, 0xfe, 0xfe, 0xfe, 0x0, 0x0, 0x0, 0xfe, 0xfe, 0xfe, 0x0, 0x0, 0xf, 0x1f, 0x3f, 0x30, 0x30, 0x30, 0x3f, 0x1f, 0xf, 0x0},        /* U */
    {0x0, 0x6, 0xfe, 0xfe, 0xc0, 0x0, 0xc0, 0xfe, 0xfe, 0x6, 0x0, 0x0, 0x0, 0x0, 0xf, 0x3f, 0x38, 0x3f, 0xf, 0x0, 0x0, 0x0},            /* V */
    {0x1e, 0xfe, 0xf0, 0x0, 0xf0, 0x70, 0xf0, 0x0, 0xf8, 0xfe, 0x1e, 0x0, 0x3f, 0x3f, 0x3f, 0xf, 0x0, 0xf, 0x3e, 0x3f, 0x1f, 0x0},      /* W */
    {0x0, 0x2, 0xe, 0x3e, 0xf8, 0xe0, 0xf8, 0x3e, 0xe, 0x2, 0x0, 0x0, 0x20, 0x38, 0x3e, 0xf, 0x3, 0xf, 0x3e, 0x38, 0x20, 0x0},          /* X */
    {0x2, 0xe, 0x3e, 0xfc, 0xe0, 0x80, 0xe0, 0xfc, 0x3e, 0xe, 0x2, 0x0, 0x0, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x0, 0x0, 0x0, 0x0},           /* Y */
    {0x0, 0xe, 0x1e, 0x7e, 0xf6, 0xe6, 0x86, 0x6, 0x6, 0x6, 0x0, 0x0, 0x30, 0x30, 0x30, 0x30, 0x33, 0x37, 0x3f, 0x3c, 0x38, 0x0},       /* Z */
    {0x0, 0x0, 0x3, 0x3, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xc0, 0xc0, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0},             /* [ */
    {0x0, 0x0, 0x0, 0x0, 0x0, 0xc0, 0xf0, 0x3c, 0xe, 0x2, 0x0, 0x0, 0x80, 0xe0, 0x78, 0x1e, 0x7, 0x0, 0x0, 0x0, 0x0, 0x0},              /* \ */
    {0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0x3, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xc0, 0xc0, 0x0, 0x0, 0x0},             /* ] */
    {0x20, 0x30, 0x38, 0x1c, 0xe, 0xe, 0x1c, 0x38, 0x30, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},             /* ^ */
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0},          /* _ */
    {0x0, 0x0, 0x0, 0x0, 0x4, 0x6, 0x3, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},                     /* ` */
    {0x0, 0xc0, 0xe0, 0xf0, 0x30, 0x30, 0x30, 0x30, 0x60, 0x0, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x13, 0x33, 0x33, 0x3f, 0x3e, 0x1e, 0x0},    /* a */
    {0x0, 0xc0, 0xe0, 0xf0, 0x70, 0x30, 0x60, 0xff, 0xff, 0xff, 0x0, 0x0, 0xf, 0x1f, 0x3f, 0x38, 0x30, 0x18, 0x3f, 0x3f, 0x3f, 0x0},    /* b */
    {0x0, 0x60, 0x30, 0x30, 0x30, 0x30, 0x70, 0xe0, 0xe0, 0x80, 0x0, 0x0, 0x18, 0x30, 0x30, 0x30, 0x30, 0x38, 0x1f, 0x1f, 0x7, 0x0},    /* c */
    {0x0, 0xff, 0xff, 0xff, 0x60, 0x30, 0x70, 0xf0, 0xe0, 0xc0, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x18, 0x30, 0x38, 0x3f, 0x1f, 0xf, 0x0},    /* d */
    {0x0, 0xc0, 0xe0, 0xf0, 0x30, 0x30, 0x30, 0xf0, 0xe0, 0xc0, 0x0, 0x0, 0x1b, 0x33, 0x33, 0x33, 0x33, 0x3b, 0x3f, 0x1f, 0xf, 0x0},    /* e */
    {0x0, 0x33, 0x33, 0x33, 0xff, 0xff, 0xfe, 0x30, 0x30, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x0, 0x0, 0x0, 0x0},          /* f */
    {0x0, 0xfc, 0xfc, 0xfc, 0x18, 0xc, 0x1c, 0xfc, 0xf8, 0xf0, 0x0, 0x0, 0x3f, 0x7f, 0xff, 0xe6, 0xcc, 0xce, 0xcf, 0x67, 0x3, 0x0},     /* g */
    {0x0, 0xe0, 0xf0, 0xf0, 0x30, 0x30, 0x60, 0xff, 0xff, 0xff, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x0, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x0},      /* h */
    {0x0, 0x0, 0x0, 0x0, 0xf3, 0xf3, 0xf3, 0x30, 0x30, 0x30, 0x0, 0x0, 0x30, 0x30, 0x30, 0x3f, 0x3f, 0x3f, 0x30, 0x30, 0x30, 0x0},      /* i */
    {0x0, 0x0, 0x0, 0x0, 0xf7, 0xf7, 0x30, 0x30, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7f, 0xff, 0xc0, 0xc0, 0xc0, 0x0, 0x0},            /* j */
    {0x0, 0x0, 0x10, 0x30, 0xf0, 0xc0, 0x80, 0xff, 0xff, 0xff, 0x0, 0x0, 0x20, 0x38, 0x3c, 0x1f, 0x7, 0x3, 0x3f, 0x3f, 0x3f, 0x0},      /* k */
    {0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0x3, 0x3, 0x3, 0x0, 0x30, 0x30, 0x30, 0x30, 0x3f, 0x1f, 0xf, 0x0, 0x0, 0x0},            /* l */
    {0x0, 0xe0, 0xf0, 0x30, 0x30, 0xe0, 0xf0, 0x30, 0x30, 0xf0, 0xf0, 0x0, 0x3f, 0x3f, 0x0, 0x0, 0x3f, 0x3f, 0x0, 0x0, 0x3f, 0x3f},     /* m */
    {0x0, 0xe0, 0xf0, 0xf0, 0x30, 0x30, 0x60, 0xf0, 0xf0, 0xf0, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x0, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x0},      /* n */
    {0x0, 0xc0, 0xe0, 0xf0, 0x70, 0x30, 0x70, 0xf0, 0xe0, 0xc0, 0x0, 0x0, 0xf, 0x1f, 0x3f, 0x38, 0x30, 0x38, 0x3f, 0x1f, 0xf, 0x0},     /* o */
    {0x0, 0xc0, 0xe0, 0xf0, 0x70, 0x30, 0x60, 0xf0, 0xf0, 0xf0, 0x0, 0x0, 0xf, 0x1f, 0x3f, 0x38, 0x30, 0x18, 0xff, 0xff, 0xff, 0x0},    /* p */
    {0x0, 0xf0, 0xf0, 0xf0, 0x60, 0x30, 0x70, 0xf0, 0xe0, 0xc0, 0x0, 0x0, 0xff, 0xff, 0xff, 0x18, 0x30, 0x38, 0x3f, 0x1f, 0xf, 0x0},    /* q */
    {0x60, 0x30, 0x30, 0x30, 0x60, 0xf0, 0xf0, 0xf0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x0, 0x0, 0x0},          /* r */
    {0x0, 0x0, 0x60, 0x30, 0x30, 0x30, 0xb0, 0xf0, 0xe0, 0xe0, 0x0, 0x0, 0x1e, 0x1e, 0x3f, 0x33, 0x33, 0x33, 0x33, 0x31, 0x18, 0x0},    /* s */
    {0x0, 0x0, 0x30, 0x30, 0x30, 0xfe, 0xfe, 0xfe, 0x30, 0x30, 0x30, 0x0, 0x0, 0x30, 0x30, 0x30, 0x3f, 0x3f, 0x1f, 0x0, 0x0, 0x0},      /* t */
    {0x0, 0xf0, 0xf0, 0xf0, 0x0, 0x0, 0x0, 0xf0, 0xf0, 0xf0, 0x0, 0x0, 0x3f, 0x3f, 0x3f, 0x18, 0x30, 0x30, 0x3f, 0x3f, 0x1f, 0x0},      /* u */
    {0x0, 0x30, 0xf0, 0xf0, 0x0, 0x0, 0x0, 0xf0, 0xf0, 0x30, 0x0, 0x0, 0x0, 0x1, 0xf, 0x3f, 0x30, 0x3f, 0xf, 0x1, 0x0, 0x0},            /* v */
    {0x70, 0xf0, 0x80, 0x0, 0x80, 0x80, 0x80, 0x0, 0x80, 0xf0, 0x70, 0x0, 0x7, 0x3f, 0x38, 0x1f, 0x1, 0x1f, 0x38, 0x3f, 0x7, 0x0},      /* w */
    {0x0, 0x10, 0x30, 0xf0, 0xe0, 0x80, 0xe0, 0xf0, 0x30, 0x10, 0x0, 0x0, 0x20, 0x30, 0x3c, 0x1f, 0x7, 0x1f, 0x3c, 0x30, 0x20, 0x0},    /* x */
    {0x0, 0x30, 0xf0, 0xf0, 0x80, 0x0, 0xc0, 0xf0, 0xf0, 0x10, 0x0, 0x0, 0x0, 0x1, 0xf, 0x7f, 0xfe, 0xdf, 0x7, 0x0, 0x0, 0x0},          /* y */
    {0x0, 0x70, 0xf0, 0xf0, 0xb0, 0x30, 0x30, 0x30, 0x30, 0x30, 0x0, 0x0, 0x30, 0x30, 0x30, 0x31, 0x33, 0x36, 0x3c, 0x3c, 0x38, 0x0},   /* z */
    {0x0, 0x2, 0x2, 0x2, 0x7e, 0xfe, 0xfe, 0x80, 0x80, 0x80, 0x0, 0x0, 0x80, 0x80, 0x80, 0xfe, 0xff, 0xff, 0x3, 0x1, 0x1, 0x0},         /* { */
    {0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0},                 /* | */
    {0x0, 0x80, 0x80, 0x80, 0xfe, 0xfe, 0x7e, 0x2, 0x2, 0x2, 0x0, 0x0, 0x1, 0x1, 0x3, 0xff, 0xff, 0xfe, 0x80, 0x80, 0x80, 0x0},         /* } */
    {0x0, 0x80, 0x0, 0x0, 0x0, 0x80, 0x80, 0x80, 0x80, 0x0, 0x0, 0x0, 0x1, 0x3, 0x3, 0x3, 0x1, 0x1, 0x1, 0x1, 0x3, 0x0}                 /* ~ */
};

uint8_t *
oled_get_char(char c)
{
    if ((c >= '!') && (c <= '~')) {
        return oled_char_table[c - '!'];
    }

    return NULL;
}

/*
 * Translate character into raster font graphics.
 */
int oled_cmd(uint8_t cmd)
{
    return i2c_write_register(oled_cmd_addr, 0b0000000, cmd);
}

 /*
  * Write data to a page.
  */
int oled_data(uint8_t data)
{
    return i2c_write_register(oled_cmd_addr, 0b1100000, data);
}

/*
 * Initialize the oled display.
 */
int oled_init(void)
{
    if (oled_cmd(SSD1306_DISPLAYOFF) < 0                /* 0xAE Set OLED Display Off */
    ||  oled_cmd(SSD1306_SETDISPLAYCLOCKDIV) < 0        /* 0xD5 Set Display Clock Divide Ratio/Oscillator Frequency */
    ||  oled_cmd(0x80) < 0
    ||  oled_cmd(SSD1306_SETMULTIPLEX) < 0              /* 0xA8 Set Multiplex Ratio */
    ||  oled_cmd(0x27) < 0
    ||  oled_cmd(SSD1306_SETDISPLAYOFFSET) < 0          /* 0xD3 Set Display Offset */
    ||  oled_cmd(0x00) < 0
    ||  oled_cmd(SSD1306_SETSTARTLINE) < 0              /* 0x40 Set Display Start Line */
    ||  oled_cmd(SSD1306_CHARGEPUMP) < 0                /* 0x8D Set Charge Pump */
    ||  oled_cmd(0x14) < 0                              /* 0x14 Enable Charge Pump */
    ||  oled_cmd(SSD1306_COMSCANDEC) < 0                /* 0xC8 Set COM Output Scan Direction */
    ||  oled_cmd(SSD1306_SETCOMPINS) < 0                /* 0xDA Set COM Pins Hardware Configuration */
    ||  oled_cmd(0x12) < 0
    ||  oled_cmd(SSD1306_SETCONTRAST) < 0               /* 0x81 Set Contrast Control */
    ||  oled_cmd(0xAF) < 0
    ||  oled_cmd(SSD1306_SETPRECHARGE) < 0              /* 0xD9 Set Pre-Charge Period */
    ||  oled_cmd(0x25) < 0
    ||  oled_cmd(SSD1306_SETVCOMDETECT) < 0             /* 0xDB Set VCOMH Deselect Level */
    ||  oled_cmd(0x20) < 0
    ||  oled_cmd(SSD1306_DISPLAYALLON_RESUME) < 0       /* 0xA4 Set Entire Display On/Off */
    ||  oled_cmd(SSD1306_NORMALDISPLAY) < 0             /* 0xA6 Set Normal/Inverse Display */
    ||  oled_cmd(SSD1306_DISPLAYON) < 0)                /* 0xAF Set OLED Display On */
        return -1;

    return 0;
}

/*
 * Set the current page address.
 */
int oled_set_page_addr(uint8_t add)
{
    return oled_cmd(0xb0 | add);
}

/*
 * Write a picture saved in returned raster graphics to the oled
 * display.
 */
int oled_write_pic(uint8_t *pic)
{
    unsigned char i, j;
    unsigned char num=0;
    int ret = 0;

    for (i = 0; i < 0x05; i++) {
        oled_set_page_addr(i);
        oled_cmd(0x10);
        oled_cmd(0x40);
        for (j = 0; j < 0x60; j++){
            if ((ret = oled_data(pic[i * 0x60 + j])) < 0 ) {
                printf("Error: Cannot write to oled display\n");
                return ret;
            }
        }
    }

    return ret;
}

/*
 * Write a text string to the oled display.
 * The display is divided with the default monospace font into two lines a
 * 8 characters per line. The whole (visible) set of ASCII characters are
 * available.
 */
void oled_write_text(char *str)
{
    unsigned char i,j;
    uint8_t data;
    uint8_t *ch;
    int ch_addr;
    int ch_num;
    int start, end;
    int width = 11;
    int col_offset = -6;
    int char_per_line = 8;

    int str_len = strlen(str);

    printf("Writing: %s (length: %i)\n", str, str_len);

    for (i = 0 ; i < 5; ++i) {
        oled_set_page_addr(i);
        oled_cmd(0x10);
        oled_cmd(0x40);

        data = 0x00;
        ch_num = char_per_line - 1;
        for (j = 0; j < 96; ++j) {
            /* Line 1 */
            if (i == 0 || i == 1) {
                start = (char_per_line - ch_num) * width + col_offset;
                end = (char_per_line - ch_num) * width + width - 1 + col_offset;
                if (ch_num < str_len && ch_num >= 0) {
                    if (j >= start && j <= end) {
                        ch_addr = (j - start) + width * (i - 0);
                        if (str[ch_num] == ' ') {
                            data = 0x00;
                        } else {
                            ch = oled_get_char(str[ch_num]);
                            if (ch == NULL) {
                                data = 0x00;
                            } else {
                                data = ch[ch_addr];
                            }
                        }
                    } else {
                        data = 0x00;
                    }
                } else {
                    data = 0x00;
                }
                if (j == end) {
                    --ch_num;
                }
            /* Line 2 */
            } else if ((i == 2 || i == 3) && ((str_len - char_per_line) > 0)) {
                start = (char_per_line - ch_num) * width + col_offset;
                end = (char_per_line - ch_num) * width + width - 1 + col_offset;
                if (ch_num < (str_len - char_per_line) && ch_num >= 0) {
                    if (j >= start && j <= end) {
                        ch_addr = (j - start) + width * (i - 2);
                        if (str[ch_num + char_per_line] == ' ') {
                            data = 0x00;
                        } else {
                            ch = oled_get_char(str[ch_num + char_per_line]);
                            if (ch == NULL) {
                                data = 0x00;
                            } else {
                                data = ch[ch_addr];
                            }
                        }
                    } else {
                        data = 0x00;
                    }
                } else {
                    data = 0x00;
                }
                if (j == end) {
                    --ch_num;
                }
            } else {
                data = 0x00;
            }

            oled_data(data);
        }
    }
}
