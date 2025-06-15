#ifndef CONFIG_H
#define CONFIG_H

// Khai báo liên quan tới Buf Size màn hình
#define DRAW_BUF_LINES 20
#define DRAW_BUF_SIZE (SCREEN_WIDTH * DRAW_BUF_LINES * (LV_COLOR_DEPTH / 8))

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS
#define TFT_BL 21  // Chân PWM cho backlight

// Khai báo kích thước màn hình
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define LV_FONT_BRIGHTNESS "\xEE\xA7\x96"

// Các thứ liên quan tới EEPROM
#define EEPROM_SIZE 64
#define BRIGHTNESS_ADDR 0
#define SIGNATURE_ADDR 2
#define SIGNATURE 0x0620
#define SCALE_FACTOR 1000    // lưu 3 chữ số thập phân

#endif