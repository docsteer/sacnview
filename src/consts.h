// Copyright 2016 Tom Barthel-Steer
// http://www.tomsteer.net
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CONSTS_H
#define CONSTS_H

#include <QColor>

#define APP_NAME        "sACNView"
#define AUTHOR          "Tom Barthel-Steer\r\nMarcus Birkin\r\nHans Hinrichsen\r\nMatt Kerr"

// If this is a full release, only show the newer version message for other full releases
// If this is prerelease, show all newer versions
#define PRERELEASE      false

#define nNumOfSecPerHour 3600
#define nNumberOfSecPerMin 60
#define nNumOfMinPerHour 60

#define MIN_SACN_UNIVERSE 1
#define MAX_SACN_UNIVERSE 63999
#define MIN_DMX_ADDRESS 1
#define MAX_DMX_ADDRESS 512
#define PRESET_COUNT 8

#define MIN_SACN_PRIORITY 0
#define MAX_SACN_PRIORITY 200
#define DEFAULT_SACN_PRIORITY 100

#define MIN_SACN_LEVEL 0
#define MAX_SACN_LEVEL 255

#define MAX_SACN_TRANSMIT_TIME_SEC  1000000

#define DEFAULT_SOURCE_NAME "sACNView"
#define MAX_SOURCE_NAME_LEN 63


enum PriorityMode
{
    pmPER_SOURCE_PRIORITY,
    pmPER_ADDRESS_PRIORITY
};

static const QStringList PriorityModeStrings = { "Per-Source", "Per-Address" };

// A table of values to draw a 0-255 sinewave
static const unsigned char sinetable[] = {
    127,130,133,136,140,143,146,149,152,155,158,161,164,167,170,173,176,179,182,185,187,190,193,
    195,198,201,203,206,208,211,213,215,217,220,222,224,226,228,230,232,233,235,237,238,240,241,
    242,244,245,246,247,248,249,250,251,252,252,253,253,254,254,254,254,254,254,254,254,254,254,
    253,253,252,252,251,250,250,249,248,247,246,244,243,242,240,239,237,236,234,232,231,229,227,
    225,223,221,219,216,214,212,209,207,204,202,199,197,194,191,189,186,183,180,177,175,172,169,
    166,163,160,157,154,150,147,144,141,138,135,132,129,125,122,119,116,113,110,107,104,100, 97,
     94, 91, 88, 85, 82, 79, 77, 74, 71, 68, 65, 63, 60, 57, 55, 52, 50, 47, 45, 42, 40, 38, 35,
     33, 31, 29, 27, 25, 23, 22, 20, 18, 17, 15, 14, 12, 11, 10,  8,  7,  6,  5,  4,  4,  3,  2,
      2,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  2,  2,  3,  4,  5,  6,  7,  8,
      9, 10, 12, 13, 14, 16, 17, 19, 21, 22, 24, 26, 28, 30, 32, 34, 37, 39, 41, 43, 46, 48, 51,
     53, 56, 59, 61, 64, 67, 69, 72, 75, 78, 81, 84, 87, 90, 93, 96, 99,102,105,108,111,114,118,
    121,124,127
};


// Conversion table to convert 0-255 to percent
static const unsigned char HTOPT[] =
{
    0,  1,  1,  1,  2,  2,  2,  3,  3,  4,  4,  4,  5,  5,  5,  6, /* 00 - 0f */
    6,  7,  7,  7,  8,  8,  9,  9,  9, 10, 10, 11, 11, 11, 12, 12, /* 10 - 1f */
    13, 13, 13, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 18, 18, 18, /* 20 - 2f */
    19, 19, 20, 20, 20, 21, 21, 22, 22, 22, 23, 23, 24, 24, 24, 25, /* 30 - 3f */
    25, 25, 26, 26, 27, 27, 27, 28, 28, 29, 29, 29, 30, 30, 31, 31, /* 40 - 4f */
    31, 32, 32, 33, 33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, /* 50 - 5f */
    38, 38, 38, 39, 39, 40, 40, 40, 41, 41, 42, 42, 42, 43, 43, 44, /* 60 - 6f */
    44, 44, 45, 45, 45, 46, 46, 47, 47, 47, 48, 48, 49, 49, 49, 50, /* 70 - 7f */
    50, 51, 51, 51, 52, 52, 53, 53, 53, 54, 54, 55, 55, 55, 56, 56, /* 80 - 8f */
    56, 57, 57, 58, 58, 58, 59, 59, 60, 60, 60, 61, 61, 62, 62, 62, /* 90 - 9f */
    63, 63, 64, 64, 64, 65, 65, 65, 66, 66, 67, 67, 67, 68, 68, 69, /* a0 - af */
    69, 69, 70, 70, 71, 71, 71, 72, 72, 73, 73, 73, 74, 74, 75, 75, /* b0 - bf */
    75, 76, 76, 76, 77, 77, 78, 78, 78, 79, 79, 80, 80, 80, 81, 81, /* c0 - cf */
    82, 82, 82, 83, 83, 84, 84, 84, 85, 85, 85, 86, 86, 87, 87, 87, /* d0 - df */
    88, 88, 89, 89, 89, 90, 90, 91, 91, 91, 92, 92, 93, 93, 93, 94, /* e0 - ef */
    94, 95, 95, 95, 96, 96, 96, 97, 97, 98, 98, 98, 99, 99, 99,100  /* f0 - ff */
};

// Conversion table to convert percent to 0-255 values
static const unsigned char PTOHT[] =
{
    0x00, 0x03, 0x05, 0x08, 0x0a, 0x0d, 0x0f, 0x12, 0x14, 0x17,  /*  0 -  9 */
    0x1a, 0x1c, 0x1f, 0x21, 0x24, 0x26, 0x29, 0x2b, 0x2e, 0x30,  /* 10 - 19 */
    0x33, 0x36, 0x38, 0x3b, 0x3d, 0x40, 0x42, 0x45, 0x47, 0x4a,  /* 20 - 29 */
    0x4d, 0x4f, 0x52, 0x54, 0x57, 0x59, 0x5c, 0x5e, 0x61, 0x63,  /* 30 - 39 */
    0x66, 0x69, 0x6b, 0x6e, 0x70, 0x73, 0x75, 0x78, 0x7a, 0x7d,  /* 40 - 49 */
    0x80, 0x82, 0x85, 0x87, 0x8a, 0x8c, 0x8f, 0x91, 0x94, 0x96,  /* 50 - 59 */
    0x99, 0x9c, 0x9e, 0xa1, 0xa3, 0xa6, 0xa8, 0xab, 0xad, 0xb0,  /* 60 - 69 */
    0xb3, 0xb5, 0xb8, 0xba, 0xbd, 0xbf, 0xc2, 0xc4, 0xc7, 0xc9,  /* 70 - 79 */
    0xcc, 0xcf, 0xd1, 0xd4, 0xd6, 0xd9, 0xdb, 0xde, 0xe0, 0xe3,  /* 80 - 89 */
    0xe6, 0xe8, 0xeb, 0xed, 0xf0, 0xf2, 0xf5, 0xf7, 0xfa, 0xfc,  /* 90 - 99 */
    0xff
};


// Fade rates for the fade fx
static const QList<qreal> FX_FADE_RATES({
                                         0.5,
                                         1,
                                         2,
                                         5,
                                         10,
                                         100,
                                         500
                                     });



static const QColor flickerHigherColor  = QColor::fromRgb(0x8d, 0x32, 0xfd);
static const QColor flickerLowerColor   = QColor::fromRgb(0x04, 0xfd, 0x44);
static const QColor flickerChangedColor = QColor::fromRgb(0xfb, 0x09, 0x09);


#endif // CONSTS_H

