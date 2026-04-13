#ifndef ST7735S_H
#define ST7735S_H

#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_spi.h"
#include "stm32l4xx_hal_dma.h"
#include "stm32l4xx_hal_tim.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


#define ST7735S_WIDTH 128
#define ST7735S_HEIGHT 160


#define	ST7735S_BLACK   0x0000
#define	ST7735S_BLUE    0x001F
#define	ST7735S_RED     0xF800
#define	ST7735S_GREEN   0x07E0
#define ST7735S_CYAN    0x07FF
#define ST7735S_MAGENTA 0xF81F
#define ST7735S_YELLOW  0xFFE0
#define ST7735S_WHITE   0xFFFF
#define ST7735S_RGBTOBGR(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))


typedef struct 
{
    GPIO_TypeDef *ST7735S_RST_GPIO_Port;
    uint16_t ST7735S_RST_Pin;
    GPIO_TypeDef *ST7735S_CLK_GPIO_Port;
    uint16_t ST7735S_CLK_Pin;
    GPIO_TypeDef *ST7735S_DC_GPIO_Port;
    uint16_t ST7735S_DC_Pin;
    GPIO_TypeDef *ST7735S_CS_GPIO_Port;
    uint16_t ST7735S_CS_Pin;
}st7735s_gpio_pins;


typedef struct {
    SPI_HandleTypeDef *hspi;
    st7735s_gpio_pins* gpio_pins; 
    TIM_HandleTypeDef *htim;
    uint32_t tim_channel;
    
} st7735s;

st7735s_gpio_pins st7735s_gpio_init(GPIO_TypeDef *ST7735S_RST_GPIO_Port, uint16_t ST7735S_RST_Pin, 
                                    GPIO_TypeDef *ST7735S_CLK_GPIO_Port, uint16_t ST7735S_CLK_Pin, 
                                    GPIO_TypeDef *ST7735S_DC_GPIO_Port, uint16_t ST7735S_DC_Pin,
                                    GPIO_TypeDef *ST7735S_CS_GPIO_Port, uint16_t ST7735S_CS_Pin);
void st7735s_tim_init(st7735s *lcd, TIM_HandleTypeDef *htim, uint32_t tim_channel);
void st7735s_init(st7735s *lcd, SPI_HandleTypeDef *hspi, st7735s_gpio_pins* gpio_pins);
bool st7735s_isbusy(st7735s *lcd);
void st7735s_isdone(st7735s *lcd);
void st7735s_cmd(st7735s *lcd, uint8_t cmd);
void st7735s_data(st7735s *lcd, uint8_t *data, uint16_t size);
void st7735s_setAddrWindow(st7735s *lcd, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void st7735s_fillScreen(st7735s *lcd, uint16_t color);
void st7735s_fillBgScreen(st7735s *lcd);
void st7735s_drawImage(st7735s *lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
void st7735s_drawPixel(st7735s *lcd, uint16_t x, uint16_t y, uint16_t color);
void st7735s_drawLine(st7735s *lcd, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void st7735s_drawRect(st7735s *lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void st7735s_drawChar(st7735s *lcd, uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg);
void st7735s_drawString(st7735s *lcd, uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg);
void st7735s_drawOption(st7735s *lcd, uint16_t x, uint16_t y, int option_number, uint16_t color, uint16_t bg);
void st7735s_drawFloat(st7735s *lcd, uint16_t x, uint16_t y, float value, const char* str1, const char* str2, uint16_t color, uint16_t bg);
void st7735s_drawBoundingBox(st7735s *lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void st7735s_drawCircle(st7735s *lcd, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void st7735s_drawFilledCircle(st7735s *lcd, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void st7735s_setRotation(st7735s *lcd, uint8_t m);




#endif /* ST7735S_H */