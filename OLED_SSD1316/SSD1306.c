/*
 * SSD1306.c
 *
 *  Created on: Mar 25, 2020
 *      Author: Appendix_Pi
 */

#include "SSD1306.h"
#include "freertos/FreeRTOS.h"
#include "octo_s_config.h"
#include "SSD1316_img.h"

uint32_t D_offset;
uint32_t cntrl_GD_return;
float Gravity_effect_X, Gravity_effect_Y,Gravity_effect_Z;
uint8_t SSD1306_Gravity_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT];


const unsigned char Avatar [] = {
		0x00,0x18,0x18,0x38,0x7C,0xFF,0xFF,0x7E,0x7C,0x74,0x74,0x78,0x38,0x30,0x10,0x00
};





extern spi_device_handle_t spi2;

void ssd1306_write_data(uint8_t *value, uint32_t lenght ) {
    spi_transaction_t t = {
        .tx_buffer = value,
        .length = lenght * 8
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi2, &t));
}

void ssd1306_Reset(void) {
	// CS = High (not selected)
	gpio_set_level(CS_PIN,1);

	// Reset the OLED
	gpio_set_level(D_RESET_PIN, 0);
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(D_RESET_PIN, 1);
	vTaskDelay(10 / portTICK_PERIOD_MS);
}

// Send a byte to the command register
void ssd1306_WriteCommand(uint8_t byte) {
    gpio_set_level(CS_PIN,0);
    gpio_set_level(D_DC_PIN, 0);
    ssd1306_write_data(&byte,1);
    gpio_set_level(CS_PIN,1);
}

// Send data
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size) {

    gpio_set_level(CS_PIN,0);
    gpio_set_level(D_DC_PIN, 1);
    ssd1306_write_data(buffer,buff_size);
    gpio_set_level(CS_PIN,1);
}

// Screenbuffer
uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
uint8_t SSD1306_Buffer_Avatar[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
uint8_t SSD1306_Buffer_Scenery[SSD1306_WIDTH * SSD1306_HEIGHT / 8];


// Screen object
static SSD1306_t SSD1306;

// Initialize the oled screen
void ssd1306_Init(void) {
	// Reset OLED
	ssd1306_Reset();

    // Wait for the screen to boot
	vTaskDelay(100 / portTICK_PERIOD_MS);

    // Init OLED
    ssd1306_WriteCommand(0xAE); //display off

    ssd1306_WriteCommand(0x20); //Set Memory Addressing Mode
    ssd1306_WriteCommand(0x00); // 00b,Horizontal Addressing Mode; 01b,Vertical Addressing Mode;
                                // 10b,Page Addressing Mode (RESET); 11b,Invalid

    ssd1306_WriteCommand(0xB0); //Set Page Start Address for Page Addressing Mode,0-7

#ifdef SSD1306_MIRROR_VERT
    ssd1306_WriteCommand(0xC0); // Mirror vertically
#else
    ssd1306_WriteCommand(0xC8); //Set COM Output Scan Direction
#endif

    ssd1306_WriteCommand(0x00); //---set low column address
    ssd1306_WriteCommand(0x10); //---set high column address

    ssd1306_WriteCommand(0x40); //--set start line address - CHECK

    ssd1306_WriteCommand(0x81); //--set contrast control register - CHECK
    ssd1306_WriteCommand(0xFF);

#ifdef SSD1306_MIRROR_HORIZ
    ssd1306_WriteCommand(0xA0); // Mirror horizontally
#else
    ssd1306_WriteCommand(0xA1); //--set segment re-map 0 to 127 - CHECK
#endif

#ifdef SSD1306_INVERSE_COLOR
    ssd1306_WriteCommand(0xA7); //--set inverse color
#else
    ssd1306_WriteCommand(0xA6); //--set normal color
#endif

    ssd1306_WriteCommand(0xA8); //--set multiplex ratio(1 to 64) - CHECK
#if (SSD1306_HEIGHT == 32)
    ssd1306_WriteCommand(0x1F); //
#elif (SSD1306_HEIGHT == 64)
    ssd1306_WriteCommand(0x3F); //
#else
#error "Only 32 or 64 lines of height are supported!"
#endif

    ssd1306_WriteCommand(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content

    ssd1306_WriteCommand(0xD3); //-set display offset - CHECK
    ssd1306_WriteCommand(0x00); //-not offset

    ssd1306_WriteCommand(0xD5); //--set display clock divide ratio/oscillator frequency
    ssd1306_WriteCommand(0xF0); //--set divide ratio

    ssd1306_WriteCommand(0xD9); //--set pre-charge period
    ssd1306_WriteCommand(0x22); //

    ssd1306_WriteCommand(0xDA); //--set com pins hardware configuration - CHECK
#if (SSD1306_HEIGHT == 32)
    ssd1306_WriteCommand(0x02);
#elif (SSD1306_HEIGHT == 64)
    ssd1306_WriteCommand(0x12);
#else
#error "Only 32 or 64 lines of height are supported!"
#endif

    ssd1306_WriteCommand(0xDB); //--set vcomh
    ssd1306_WriteCommand(0x20); //0x20,0.77xVcc

    ssd1306_WriteCommand(0x8D); //--set DC-DC enable
    ssd1306_WriteCommand(0x14); //
    ssd1306_WriteCommand(0xAF); //--turn on SSD1306 panel

    // Clear screen
    ssd1306_Fill(Black);

    // Flush buffer to screen
    ssd1306_UpdateScreen();

    // Set default values for screen object
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;

    SSD1306.Initialized = 1;
}

void ssd1306_gradFill(SSD1306_COLOR color, uint16_t w_delay){
    /* Set memory */
    uint32_t i,j;

    for(i = 0; i < sizeof(SSD1306_Buffer)/8; i++) {
    	for(j=0; j < 8; j++){
    		SSD1306_Buffer[i+j*128] = (color == Black) ? 0x00 : 0xFF;
    	}

        ssd1306_UpdateScreen();
        vTaskDelay(w_delay / portTICK_PERIOD_MS);
    }
}

// Fill the whole screen with the given color
void ssd1306_Fill(SSD1306_COLOR color) {
    /* Set memory */
    uint32_t i;

    for(i = 0; i < sizeof(SSD1306_Buffer); i++) {
        SSD1306_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
    }
}

void ssd1306_PrintLogo(const unsigned char *oled_array){
	D_offset = 0;
	for(int j = 0; j < 8; j++){
		for(int y = 0; y < 128; y++){
			if((D_offset+y)<128){
				SSD1306_Buffer[(j*128)+y]=oled_array[(j*128)+(D_offset+y)];
			}else{
				SSD1306_Buffer[(j*128)+y]=oled_array[(j*128)+(D_offset+y-128)];
			}
		}
	}
	ssd1306_UpdateScreen();
}


uint8_t ssd1306_PrintAvatar(uint8_t x,uint8_t y){
	int16_t p_height, p_width, p_bit;
	uint8_t data_up, data_down;
	uint8_t result = 0;						//
	uint32_t i_copy;

	if(y > (SSD1306_HEIGHT-8)){
		y = SSD1306_HEIGHT-8;
	}
	if(x >= (SSD1306_WIDTH-16)){
		x = (SSD1306_WIDTH-16);
	}
    for(i_copy = 0; i_copy < sizeof(SSD1306_Buffer_Avatar); i_copy++) {
    	SSD1306_Buffer_Avatar[i_copy] = 0x00;
    }
	for(int y_s = 0; y_s < 16; y_s++){
		SSD1306_Buffer_Avatar[ SSD1306_WIDTH*(y/8) +y_s + x]=Avatar[(y_s)];
	}

	for(int x_s = 0; x_s < y%8; x_s ++){
		for(p_height = (y/8)+1; p_height >= y/8; p_height --  ){
			for(p_width = ((x+16)); p_width >= x; p_width --){
				if(p_height != ((SSD1306_HEIGHT/8)-1)){
					data_down = 1;
					data_up = 0x01<<7;
					data_down &= SSD1306_Buffer_Avatar[p_width + SSD1306_WIDTH*(p_height+1)];
					data_up &= SSD1306_Buffer_Avatar[p_width + SSD1306_WIDTH*p_height];
					if((data_down == 0)&&(data_up != 0)){
						SSD1306_Buffer_Avatar[p_width + SSD1306_WIDTH*p_height] &= ~(0x01<<(8-1));
						SSD1306_Buffer_Avatar[p_width + SSD1306_WIDTH*(p_height+1)] |= (0x01);
						result = 1;
					}
				}
				for(p_bit = (8-1) ; p_bit > 0; p_bit --){
					data_down = 0x01<<p_bit;
					data_up = 0x01<< (p_bit - 1);
					data_down &= SSD1306_Buffer_Avatar[p_width + SSD1306_WIDTH*p_height];
					data_up &= SSD1306_Buffer_Avatar[p_width + SSD1306_WIDTH*p_height];
					if((data_down == 0)&&(data_up != 0)){
						SSD1306_Buffer_Avatar[p_width + SSD1306_WIDTH*p_height] &= ~(0x01<<(p_bit-1));
						SSD1306_Buffer_Avatar[p_width + SSD1306_WIDTH*p_height] |= (0x01<<(p_bit));
						result = 1;
					}
				}

			}
		}
	}

    for(i_copy = 0; i_copy < sizeof(SSD1306_Buffer_Avatar); i_copy++) {
		if(((SSD1306_Buffer[i_copy])&(SSD1306_Buffer_Avatar[i_copy])) != 0){
			SSD1306_Buffer[i_copy] |= SSD1306_Buffer_Avatar[i_copy];
			result = 2;
		}else{
			SSD1306_Buffer[i_copy] |= SSD1306_Buffer_Avatar[i_copy];
		}
    }
	return result;
}



void ssd1306_FillFromTo(uint16_t from_, uint16_t to_,SSD1306_COLOR color){
    for(int i = from_; i < to_; i++) {
        SSD1306_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
    }
}

void ssd1306_OldStyleTVOFF(void){
	int16_t p_height, p_width, p_bit;

	ssd1306_Fill(White);
	ssd1306_UpdateScreen();
	vTaskDelay(50 / portTICK_PERIOD_MS);
	uint32_t time_effect = 15;

	for(p_height = 0; p_height < (SSD1306_HEIGHT/8)/2; p_height ++  ){
		for(p_bit = 0 ; p_bit < 8; p_bit ++){
			for(p_width = 0; p_width < SSD1306_WIDTH; p_width ++){
				if((p_height == ((SSD1306_HEIGHT/8)/2 -1))&&(p_bit == 7)){
					SSD1306_Buffer[p_width + SSD1306_WIDTH*p_height] &= ~(0x01<<(p_bit));
					SSD1306_Buffer[p_width + SSD1306_WIDTH*((SSD1306_HEIGHT/8)-(p_height+1))] &= ~(0x01<<(7-p_bit));
					SSD1306_Buffer[(SSD1306_WIDTH - (p_width+1)) + SSD1306_WIDTH*p_height] &= ~(0x01<<(p_bit));
					SSD1306_Buffer[(SSD1306_WIDTH - (p_width+1)) + SSD1306_WIDTH*((SSD1306_HEIGHT/8)-(p_height+1))] &= ~(0x01<<(7-p_bit));
					ssd1306_UpdateScreen();
					vTaskDelay(time_effect / portTICK_PERIOD_MS);
					if(p_width == (SSD1306_WIDTH/2-1)){
						p_width = SSD1306_WIDTH;
					}
				}else{
					SSD1306_Buffer[p_width + SSD1306_WIDTH*p_height] &= ~(0x01<<(p_bit));
					SSD1306_Buffer[p_width + SSD1306_WIDTH*((SSD1306_HEIGHT/8)-(p_height+1))] &= ~(0x01<<(7-p_bit));
				}
			}
			if(!((p_height == ((SSD1306_HEIGHT/8)/2 -1))&&(p_bit == 7))){
				ssd1306_UpdateScreen();
				vTaskDelay(time_effect / portTICK_PERIOD_MS);
			}

		}
	}
}


/*
 * To obtain the melt effect: iterate until it return 0
 * */
uint8_t ssd1306_melt_effect(uint8_t direction){
	int16_t p_height, p_width, p_bit;
	uint8_t data_up, data_down;
	uint8_t result = 0;

	switch(direction){
	case 1:
		for(p_height = (SSD1306_HEIGHT/8)-1; p_height >= 0; p_height --  ){
			for(p_width = SSD1306_WIDTH-1; p_width >= 0; p_width --){
				if(p_height != ((SSD1306_HEIGHT/8)-1)){
					data_down = 1;
					data_up = 0x01<<7;
					data_down &= SSD1306_Buffer[p_width + SSD1306_WIDTH*(p_height+1)];
					data_up &= SSD1306_Buffer[p_width + SSD1306_WIDTH*p_height];
					if((data_down == 0)&&(data_up != 0)){
						SSD1306_Buffer[p_width + SSD1306_WIDTH*p_height] &= ~(0x01<<(8-1));
						SSD1306_Buffer[p_width + SSD1306_WIDTH*(p_height+1)] |= (0x01);
						result = 1;
					}
				}
				for(p_bit = (8-1) ; p_bit > 0; p_bit --){
					data_down = 0x01<<p_bit;
					data_up = 0x01<< (p_bit - 1);
					data_down &= SSD1306_Buffer[p_width + SSD1306_WIDTH*p_height];
					data_up &= SSD1306_Buffer[p_width + SSD1306_WIDTH*p_height];
					if((data_down == 0)&&(data_up != 0)){
						SSD1306_Buffer[p_width + SSD1306_WIDTH*p_height] &= ~(0x01<<(p_bit-1));
						SSD1306_Buffer[p_width + SSD1306_WIDTH*p_height] |= (0x01<<(p_bit));
						result = 1;
					}
				}

			}
		}
		break;
	case 2:
		for(p_width = SSD1306_WIDTH-1; p_width > 0; p_width --){
			for(p_height = 0; p_height < (SSD1306_HEIGHT/8); p_height ++){
				for(p_bit = (8) ; p_bit > 0; p_bit --){
					data_down = 0x01<< (p_bit - 1);
					data_up = 0x01<< (p_bit - 1);
					data_down &= SSD1306_Buffer[p_width + SSD1306_WIDTH*p_height];
					data_up &= SSD1306_Buffer[p_width-1 + SSD1306_WIDTH*p_height];
					if((data_down == 0)&&(data_up != 0)){
						SSD1306_Buffer[p_width-1 + SSD1306_WIDTH*p_height] &= ~(0x01<<(p_bit-1));
						SSD1306_Buffer[p_width + SSD1306_WIDTH*p_height] |= (0x01<<(p_bit-1));
						result = 1;
					}
				}

			}
		}
		break;


	}
	return result;
}



// Write the screenbuffer with changed to the screen
void ssd1306_UpdateScreen(void) {
    uint8_t i;
    for(i = 0; i < 8; i++) {
        ssd1306_WriteCommand(0xB0 + i);
        ssd1306_WriteCommand(0x00);
        ssd1306_WriteCommand(0x10);
        ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH*i],SSD1306_WIDTH);
    }
}

//    Draw one pixel in the screenbuffer
//    X => X Coordinate
//    Y => Y Coordinate
//    color => Pixel color
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        // Don't write outside the buffer
        return;
    }

    // Check if pixel should be inverted
    if(SSD1306.Inverted) {
        color = (SSD1306_COLOR)!color;
    }

    // Draw in the right color
    if(color == White) {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    } else {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

// Draw 1 char to the screen buffer
// ch       => char om weg te schrijven
// Font     => Font waarmee we gaan schrijven
// color    => Black or White
char ssd1306_WriteChar(char ch, FontDef Font, SSD1306_COLOR color) {
    uint32_t i, b, j;

    // Check if character is valid
    if (ch < 32 || ch > 126)
        return 0;

    // Check remaining space on current line
    if (SSD1306_WIDTH < (SSD1306.CurrentX + Font.FontWidth) ||
        SSD1306_HEIGHT < (SSD1306.CurrentY + Font.FontHeight))
    {
        // Not enough space on current line
        return 0;
    }

    // Use the font to write
    for(i = 0; i < Font.FontHeight; i++) {
        b = Font.data[(ch - 32) * Font.FontHeight + i];
        for(j = 0; j < Font.FontWidth; j++) {
            if((b << j) & 0x8000)  {
                ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR) color);
            } else {
                ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
            }
        }
    }

    // The current space is now taken
    SSD1306.CurrentX += Font.FontWidth;

    // Return written char for validation
    return ch;
}

/*
 * This function write ° character in Font_7x10
 * */
char ssd1306_WriteSpecialChar(FontDef Font, SSD1306_COLOR color){
	uint32_t i, b, j;
    // Check remaining space on current line
    if (SSD1306_WIDTH < (SSD1306.CurrentX + Font.FontWidth) ||
        SSD1306_HEIGHT < (SSD1306.CurrentY + Font.FontHeight))
    {
        // Not enough space on current line
        return 0;
    }

    // Use the font to write
    for(i = 0; i < Font.FontHeight; i++) {
        b = Font.data[(95) * Font.FontHeight + i];
        for(j = 0; j < Font.FontWidth; j++) {
            if((b << j) & 0x8000)  {
                ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR) color);
            } else {
                ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
            }
        }
    }

    // The current space is now taken
    SSD1306.CurrentX += Font.FontWidth;

    // Return written char for validation
    return 1;
}

// Write full string to screenbuffer
char ssd1306_SlowWriteString(char* str, FontDef Font, SSD1306_COLOR color, uint16_t delay) {
    // Write until null-byte
    while (*str) {
        if (ssd1306_WriteChar(*str, Font, color) != *str) {
            // Char could not be written
            return *str;
        }
        ssd1306_UpdateScreen();
        vTaskDelay(delay / portTICK_PERIOD_MS);
        // Next char
        str++;
    }

    // Everything ok
    return *str;
}

// Write full string to screenbuffer
char ssd1306_WriteString(char* str, FontDef Font, SSD1306_COLOR color) {
    // Write until null-byte
    while (*str) {
        if (ssd1306_WriteChar(*str, Font, color) != *str) {
            // Char could not be written
            return *str;
        }

        // Next char
        str++;
    }

    // Everything ok
    return *str;
}

// Position the cursor
void ssd1306_SetCursor(uint8_t x, uint8_t y) {
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}
