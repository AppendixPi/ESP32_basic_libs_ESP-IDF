# SSD1306 - 128x64 OLED display C library for ESP32

128x64 OLED display C library for ESP32 compatible with ESP32 using spi2 interface and FreeRTOS.

The library contains some standard functions plus some custom ones, comments in SSD1306.c file

## Description:

before using the library, you need to initialize the GPIOs and SPI. In my case, I use the following PINs: 

    #define CLK_PIN       18
    #define MOSI_PIN      26
    #define CS_PIN        19
    #define D_RESET_PIN   21
    #define D_DC_PIN	  22

    gpio_set_direction(D_RESET_PIN, GPIO_MODE_OUTPUT);
    gpio_intr_disable(D_RESET_PIN);
    gpio_pulldown_dis(D_RESET_PIN);
    gpio_set_level(D_RESET_PIN, 0);
    gpio_set_direction(D_DC_PIN, GPIO_MODE_OUTPUT);
    gpio_intr_disable(D_DC_PIN);
    gpio_pulldown_dis(D_DC_PIN);
    gpio_set_level(D_DC_PIN, 0);
    gpio_set_direction(CS_PIN, GPIO_MODE_OUTPUT);
    gpio_intr_disable(CS_PIN);
    gpio_pulldown_dis(CS_PIN);
    gpio_set_level(CS_PIN, 0);
    spi_init();

After whthat you can call the function ssd1306_Init() to initialize the Display.

You can test the display calling the function:

    ssd1306_TestAll();
