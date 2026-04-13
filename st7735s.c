#include "st7735s.h"
#include "fonts.h"



#define ST7735_SWRESET          0x01
#define ST7735S_SLPOUT          0x11
#define ST7735S_DISPOFF         0x28
#define ST7735S_DISPON          0x29
#define ST7735S_CASET           0x2a
#define ST7735S_RASET           0x2b
#define ST7735S_RAMWR           0x2c
#define ST7735S_MADCTL          0x36
#define ST7735S_COLMOD          0x3a
#define ST7735S_FRMCTR1         0xb1
#define ST7735S_FRMCTR2         0xb2
#define ST7735S_FRMCTR3         0xb3
#define ST7735S_INVCTR          0xb4
#define ST7735S_PWCTR1          0xc0
#define ST7735S_PWCTR2          0xc1
#define ST7735S_PWCTR3          0xc2
#define ST7735S_PWCTR4          0xc3
#define ST7735S_PWCTR5          0xc4
#define ST7735S_VMCTR1          0xc5
#define ST7735S_GAMCTRP1        0xe0
#define ST7735S_GAMCTRN1        0xe1
#define ST7735S_INVOFF          0x20  
#define ST7735S_INVON           0x21  

#define CMD(x)                  ((x) | 0x100)
#define BACKGROUND_COLOR        ST7735S_BLACK

#define SPI_CHUNK_SIZE          256 
#define ST7735S_FONT_WIDTH        5  
#define ST7735S_FONT_HEIGHT       7  
#define ST7735S_CHAR_SPACING      0


// MADCTL Bits
#define ST7735S_MADCTL_MY       0x80 // Bottom to top
#define ST7735S_MADCTL_MX       0x40 // Right to left
#define ST7735S_MADCTL_MV       0x20 // Reverse Mode (X and Y swapped)
#define ST7735S_MADCTL_ML       0x10 // LCD refresh Bottom to top
#define ST7735S_MADCTL_RGB      0x00 // Red-Green-Blue pixel order
#define ST7735S_MADCTL_BGR      0x08 // Blue-Green-Red pixel order 
#define ST7735S_MADCTL_MH       0x04 // LCD refresh right to left





/*
static const uint16_t init_table[] = {
  CMD(ST7735S_FRMCTR1), 0x01, 0x2c, 0x2d,
  CMD(ST7735S_FRMCTR2), 0x01, 0x2c, 0x2d,
  CMD(ST7735S_FRMCTR3), 0x01, 0x2c, 0x2d, 0x01, 0x2c, 0x2d,
  CMD(ST7735S_INVCTR), 0x07,
  CMD(ST7735S_PWCTR1), 0xa2, 0x02, 0x84,
  CMD(ST7735S_PWCTR2), 0xc5,
  CMD(ST7735S_PWCTR3), 0x0a, 0x00,
  CMD(ST7735S_PWCTR4), 0x8a, 0x2a,
  CMD(ST7735S_PWCTR5), 0x8a, 0xee,
  CMD(ST7735S_VMCTR1), 0x0e,
                        
  CMD(0xf0), 0x01,
  CMD(0xf6), 0x00,
  CMD(ST7735S_COLMOD), 0x05, // 16-bit color
  
  // Color Order (BGR instead of RGB)
  CMD(ST7735S_MADCTL), 0xa0, 
  
  // Hardware Inversion OFF (Fixes the polarization effect)
  CMD(ST7735S_INVOFF),
};
*/

st7735s_gpio_pins st7735s_gpio_init(GPIO_TypeDef *LCD_RST_GPIO_Port, uint16_t LCD_RST_Pin, 
                                GPIO_TypeDef *LCD_CLK_GPIO_Port, uint16_t LCD_CLK_Pin, 
                                GPIO_TypeDef *LCD_DC_GPIO_Port, uint16_t LCD_DC_Pin,
                                GPIO_TypeDef *LCD_CS_GPIO_Port, uint16_t LCD_CS_Pin)
{
    st7735s_gpio_pins pins;
    pins.ST7735S_RST_GPIO_Port = LCD_RST_GPIO_Port;
    pins.ST7735S_RST_Pin = LCD_RST_Pin;
    pins.ST7735S_CLK_GPIO_Port = LCD_CLK_GPIO_Port;
    pins.ST7735S_CLK_Pin = LCD_CLK_Pin;
    pins.ST7735S_DC_GPIO_Port = LCD_DC_GPIO_Port;
    pins.ST7735S_DC_Pin = LCD_DC_Pin;
    pins.ST7735S_CS_GPIO_Port = LCD_CS_GPIO_Port;
    pins.ST7735S_CS_Pin = LCD_CS_Pin;

    return pins;
}

void st7735s_tim_init(st7735s *lcd, TIM_HandleTypeDef *htim, uint32_t tim_channel)
{
    lcd->htim = htim;
    lcd->tim_channel = tim_channel;

    HAL_TIM_Base_Start(lcd->htim);
    HAL_TIM_PWM_Start(lcd->htim, lcd->tim_channel);
}

bool st7735s_isbusy(st7735s *lcd)
{
    return (HAL_SPI_GetState(lcd->hspi) == HAL_SPI_STATE_BUSY);
}

void st7735s_isdone(st7735s *lcd)
{
    HAL_GPIO_WritePin(lcd->gpio_pins->ST7735S_CS_GPIO_Port, lcd->gpio_pins->ST7735S_CS_Pin, GPIO_PIN_SET);
}

void st7735s_cmd(st7735s *lcd, uint8_t cmd)
{
    HAL_GPIO_WritePin(lcd->gpio_pins->ST7735S_DC_GPIO_Port, lcd->gpio_pins->ST7735S_DC_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(lcd->gpio_pins->ST7735S_CS_GPIO_Port, lcd->gpio_pins->ST7735S_CS_Pin, GPIO_PIN_RESET);
    
    HAL_SPI_Transmit(lcd->hspi, &cmd, 1, HAL_MAX_DELAY);
    
    HAL_GPIO_WritePin(lcd->gpio_pins->ST7735S_CS_GPIO_Port, lcd->gpio_pins->ST7735S_CS_Pin, GPIO_PIN_SET);
}

void st7735s_data(st7735s *lcd, uint8_t *data, uint16_t size)
{
    HAL_GPIO_WritePin(lcd->gpio_pins->ST7735S_DC_GPIO_Port, lcd->gpio_pins->ST7735S_DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(lcd->gpio_pins->ST7735S_CS_GPIO_Port, lcd->gpio_pins->ST7735S_CS_Pin, GPIO_PIN_RESET);
    
    if (size > 16) 
    {
        if (HAL_SPI_Transmit_DMA(lcd->hspi, data, size) == HAL_OK) {
            while (HAL_SPI_GetState(lcd->hspi) != HAL_SPI_STATE_READY) {}
        }
    } 
    else 
    {
        HAL_SPI_Transmit(lcd->hspi, data, size, HAL_MAX_DELAY);
    }
}



void st7735s_setAddrWindow(st7735s *lcd, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    st7735s_cmd(lcd, ST7735S_CASET);
    uint8_t data_col[] = { (x0 >> 8) & 0xFF, x0 & 0xFF, (x1 >> 8) & 0xFF, x1 & 0xFF };
    st7735s_data(lcd, data_col, sizeof(data_col));

    st7735s_cmd(lcd, ST7735S_RASET);
    uint8_t data_row[] = { (y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF };
    st7735s_data(lcd, data_row, sizeof(data_row));
    st7735s_cmd(lcd, ST7735S_RAMWR);
}

void st7735s_fillScreen(st7735s *lcd, uint16_t color) 
{
    st7735s_setAddrWindow(lcd, 0, 0, ST7735S_WIDTH+3, ST7735S_HEIGHT+3);
    
    uint16_t swapped_color = (color >> 8) | (color << 8);
    uint16_t chunk_buf[SPI_CHUNK_SIZE];

    for (int i = 0; i < SPI_CHUNK_SIZE; i++) 
    {
        chunk_buf[i] = swapped_color;
    }
    
    uint32_t total_pixels = (ST7735S_WIDTH + 3) * (ST7735S_HEIGHT + 3);
    
    while (total_pixels > 0) 
    {
        uint16_t send_pixels = (total_pixels > SPI_CHUNK_SIZE) ? SPI_CHUNK_SIZE : total_pixels;
        st7735s_data(lcd, (uint8_t*)chunk_buf, send_pixels * 2);
        total_pixels -= send_pixels;
    }
}

void st7735s_fillBgScreen(st7735s *lcd)
{
    st7735s_setAddrWindow(lcd, 0, 0, ST7735S_WIDTH + 3, ST7735S_HEIGHT + 3);

    uint16_t swapped_color = (BACKGROUND_COLOR >> 8) | (BACKGROUND_COLOR << 8);
    uint16_t chunk_buf[SPI_CHUNK_SIZE];

    for (int i = 0; i < SPI_CHUNK_SIZE; i++) 
    {
        chunk_buf[i] = swapped_color;
    }

    uint32_t total_pixels = (ST7735S_WIDTH + 3) * (ST7735S_HEIGHT + 3);

    while (total_pixels > 0) 
    {
        uint16_t send_pixels = (total_pixels > SPI_CHUNK_SIZE) ? SPI_CHUNK_SIZE : total_pixels;
        st7735s_data(lcd, (uint8_t*)chunk_buf, send_pixels * 2);
        total_pixels -= send_pixels;
    }
}

void st7735s_init(st7735s *lcd, SPI_HandleTypeDef *hspi, st7735s_gpio_pins* gpio_pins)
{
    lcd->hspi = hspi;
    lcd->gpio_pins = gpio_pins;

    HAL_GPIO_WritePin(lcd->gpio_pins->ST7735S_RST_GPIO_Port, lcd->gpio_pins->ST7735S_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(120);
    HAL_GPIO_WritePin(lcd->gpio_pins->ST7735S_RST_GPIO_Port, lcd->gpio_pins->ST7735S_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100);

    st7735s_cmd(lcd, ST7735_SWRESET);
    HAL_Delay(120);

    
    //for (uint16_t i = 0; i < sizeof(init_table) / sizeof(uint16_t); i++) {
    //    st7735s_cmd(lcd, init_table[i]);
    //}

    
    st7735s_cmd(lcd, ST7735S_SLPOUT);
    HAL_Delay(250);

    st7735s_cmd(lcd, ST7735S_COLMOD);
    uint8_t colmod_data = 0x05;
    st7735s_data(lcd, &colmod_data, 1); 


    st7735s_cmd(lcd, ST7735S_DISPON);
    HAL_Delay(50);
    st7735s_fillBgScreen(lcd);
}

void st7735s_drawImage(st7735s *lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    st7735s_setAddrWindow(lcd, x, y, x + w - 1, y + h - 1);
    
    uint8_t chunk_buf[SPI_CHUNK_SIZE * 2]; 
    
    uint32_t total_pixels = w * h;
    uint32_t data_idx = 0;

    while (total_pixels > 0) 
    {
        uint16_t chunk_pixels = (total_pixels > SPI_CHUNK_SIZE) ? SPI_CHUNK_SIZE : total_pixels;
        
        for (uint16_t i = 0; i < chunk_pixels; i++) 
        {
            uint16_t color = data[data_idx++];
            chunk_buf[i * 2]     = (color >> 8) & 0xFF; 
            chunk_buf[i * 2 + 1] = color & 0xFF;        
        }
        
        st7735s_data(lcd, chunk_buf, chunk_pixels * 2);
        total_pixels -= chunk_pixels;
    }
}

void st7735s_drawPixel(st7735s *lcd, uint16_t x, uint16_t y, uint16_t color)
{
    st7735s_setAddrWindow(lcd, x, y, x, y);
    uint8_t data[] = { (color >> 8) & 0xFF, color & 0xFF };
    st7735s_data(lcd, data, sizeof(data));
}

void st7735s_drawRect(st7735s *lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    st7735s_setAddrWindow(lcd, x, y, x + w - 1, y + h - 1);
    
    uint16_t swapped_color = (color >> 8) | (color << 8);
    uint16_t chunk_buf[SPI_CHUNK_SIZE]; 
    
    for (uint16_t i = 0; i < SPI_CHUNK_SIZE; i++) 
    {
        chunk_buf[i] = swapped_color;
    }    

    uint32_t total_pixels = w * h;
    
    while (total_pixels > 0) 
    {
        uint16_t send_pixels = (total_pixels > SPI_CHUNK_SIZE) ? SPI_CHUNK_SIZE : total_pixels;
        st7735s_data(lcd, (uint8_t*)chunk_buf, send_pixels * 2);
        total_pixels -= send_pixels;
    }
}

void st7735s_drawLine(st7735s *lcd, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    int16_t _x0 = (int16_t)x0;
    int16_t _y0 = (int16_t)y0;
    int16_t _x1 = (int16_t)x1;
    int16_t _y1 = (int16_t)y1;

    int16_t dx = abs(_x1 - _x0), sx = _x0 < _x1 ? 1 : -1;
    int16_t dy = -abs(_y1 - _y0), sy = _y0 < _y1 ? 1 : -1;
    int16_t err = dx + dy, e2; 

    while (1) {
        st7735s_drawPixel(lcd, (uint16_t)_x0, (uint16_t)_y0, color);
        if (_x0 == _x1 && _y0 == _y1) break;
        e2 = 2 * err;
        if (e2 >= dy) 
        { 
            err += dy;
            _x0 += sx; 
        }
        if (e2 <= dx) 
        { 
            err += dx; 
            _y0 += sy; 
        }
    }
}


void st7735s_drawChar(st7735s *lcd, uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg) {
    if (c < 32 || c > 126) return;
    
    uint8_t font_idx = c - 32;
    uint16_t char_buf[ST7735S_FONT_WIDTH * ST7735S_FONT_HEIGHT]; // ex. 5 width * 7 height = 35 pixels
    
    for (uint8_t col = 0; col < ST7735S_FONT_WIDTH; col++) {
        uint8_t line = font_5x7[font_idx * ST7735S_FONT_WIDTH + col];
        for (uint8_t row = 0; row < ST7735S_FONT_HEIGHT; row++) 
        {
            uint16_t pixel_color = (line & 0x01) ? ((color >> 8) | (color << 8)) : ((bg >> 8) | (bg << 8));
                
            char_buf[row * ST7735S_FONT_WIDTH + col] = pixel_color;
            
            line >>= 1;
        }
    }
    
   
    st7735s_setAddrWindow(lcd, x, y, x + ST7735S_FONT_WIDTH - 1, y + ST7735S_FONT_HEIGHT - 1);
    st7735s_data(lcd, (uint8_t*)char_buf, ST7735S_FONT_WIDTH * ST7735S_FONT_HEIGHT * 2);
}


// Function to draw an entire string
void st7735s_drawString(st7735s *lcd, uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg) 
{
    while (*str) 
    {
        st7735s_drawChar(lcd, x, y, *str, color, bg);
        x += ST7735S_FONT_WIDTH + ST7735S_CHAR_SPACING;
        str++;
    }
}


void st7735s_drawOption(st7735s *lcd, uint16_t x, uint16_t y, int option_number, uint16_t color, uint16_t bg) 
{

    char buffer[32]; 
    snprintf(buffer, sizeof(buffer), "%d", option_number);
    
    
    st7735s_drawString(lcd, x, y, buffer, color, bg);
}


void st7735s_drawBoundingBox(st7735s *lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) 
{
    st7735s_drawLine(lcd, x, y, x + w - 1, y, color);             
    st7735s_drawLine(lcd, x, y + h - 1, x + w - 1, y + h - 1, color); 
    st7735s_drawLine(lcd, x, y, x, y + h - 1, color);             
    st7735s_drawLine(lcd, x + w - 1, y, x + w - 1, y + h - 1, color); 
}


void st7735s_drawCircle(st7735s *lcd, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
    int16_t dx = 0;
    int16_t dy = r;
    int16_t d = 3 - 2 * r;

    while (dy >= dx) 
    {
        st7735s_drawPixel(lcd, x0 + dx, y0 + dy, color);
        st7735s_drawPixel(lcd, x0 - dx, y0 + dy, color);
        st7735s_drawPixel(lcd, x0 + dx, y0 - dy, color);
        st7735s_drawPixel(lcd, x0 - dx, y0 - dy, color);
        st7735s_drawPixel(lcd, x0 + dy, y0 + dx, color);
        st7735s_drawPixel(lcd, x0 - dy, y0 + dx, color);
        st7735s_drawPixel(lcd, x0 + dy, y0 - dx, color);
        st7735s_drawPixel(lcd, x0 - dy, y0 - dx, color);

        if (d < 0) 
        {
            d += 4 * dx + 6;
        } else {
            d += 4 * (dx - dy) + 10;
            dy--;
        }
        dx++;
    }
}

void st7735s_drawFilledCircle(st7735s *lcd, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
    int16_t dx = 0;
    int16_t dy = r;
    int16_t d = 3 - 2 * r;

    while (dy >= dx) 
    {
        st7735s_drawLine(lcd, x0 - dx, y0 - dy, x0 + dx, y0 - dy, color);
        st7735s_drawLine(lcd, x0 - dx, y0 + dy, x0 + dx, y0 + dy, color);
        st7735s_drawLine(lcd, x0 - dy, y0 - dx, x0 + dy, y0 - dx, color);
        st7735s_drawLine(lcd, x0 - dy, y0 + dx, x0 + dy, y0 + dx, color);

        if (d < 0) 
        {
            d += 4 * dx + 6;
        } else 
        {
            d += 4 * (dx - dy) + 10;
            dy--;
        }
        dx++;
    }
}


void st7735s_drawFloat(st7735s *lcd, uint16_t x, uint16_t y, float value, const char* str1, const char* str2, uint16_t color, uint16_t bg) 
{
    
    char buffer[32];
    if (str1 && !str2) 
    {
        snprintf(buffer, sizeof(buffer), "%s %.2f", str1, value);
    } 
    else if(str2 && !str1)
    {
        snprintf(buffer, sizeof(buffer), "%.2f %s", value, str2);
    }
    else if (str1 && str2) 
    {
        snprintf(buffer, sizeof(buffer), "%s %.2f %s", str1, value, str2);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "%.2f", value);
    }
    
    st7735s_drawString(lcd, x, y, buffer, color, bg);
}





void st7735s_setRotation(st7735s *lcd, uint8_t m) {
    uint8_t madctl = 0;

    uint8_t color_order = ST7735S_MADCTL_RGB; 

    switch (m % 4) {
        case 0: // 0 Degrees 
            madctl = ST7735S_MADCTL_MX | ST7735S_MADCTL_MY | color_order;
            // _width  = ST7735S_WIDTH;
            // _height = ST7735S_HEIGHT;
            break;
        case 1: // 90 Degrees 
            madctl = ST7735S_MADCTL_MY | ST7735S_MADCTL_MV | color_order;
            // _width  = ST7735S_HEIGHT;
            // _height = ST7735S_WIDTH;
            break;
        case 2: // 180 Degrees 
            madctl = color_order;
            // _width  = ST7735S_WIDTH;
            // _height = ST7735S_HEIGHT;
            break;
        case 3: // 270 Degrees
            madctl = ST7735S_MADCTL_MX | ST7735S_MADCTL_MV | color_order;
            // _width  = ST7735S_HEIGHT;
            // _height = ST7735S_WIDTH;
            break;
    }

    
    st7735s_cmd(lcd, ST7735S_MADCTL);
    st7735s_data(lcd, &madctl, 1);
}

