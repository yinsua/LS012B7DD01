#include <stdlib.h>
#include <memory.h>
#include "fonts.h"
#include "mlcd_test.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;       //SPI interface for display

MLCD mlcd;

void mlcd_Init(void) {
  mlcd_Clear();
  HAL_Delay(1000);
  HAL_GPIO_WritePin(DISP_DISPLAY_GPIO_Port, DISP_DISPLAY_Pin, GPIO_PIN_SET);
  mlcd.CurrentX = 0;
  mlcd.CurrentY = 0;
  mlcd.Inverted = FALSE;
  mlcd.Initialized = TRUE;

  mlcd.BufferSize = LCD_WIDTH * LCD_HEIGHT / 8;
  mlcd.Buffer = (uint8_t *) malloc(mlcd.BufferSize);
}

void mlcd_Clear(void) {
  uint8_t clear_data[2] = {BIT_CLEAR, 0x00};

  HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_SET);
  HAL_SPI_Transmit(&hspi1, &clear_data[0], 1, HAL_MAX_DELAY);
  HAL_SPI_Transmit(&hspi1, &clear_data[1], 1, HAL_MAX_DELAY);
  HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_RESET);

  mlcd.CurrentX = 0;
  mlcd.CurrentY = 0;

  memset(mlcd.Buffer, mlcd.Inverted ? 0x00 : 0xFF, mlcd.BufferSize);
}

void mlcd_DrawPixel(uint8_t x, uint8_t y, LCD_COLOR color) {
  uint16_t buff_addr = x / 8 + (LCD_WIDTH / 8) * y;
  uint8_t bit_to_set = 1 << ((x % 8));

  if (x >= LCD_WIDTH || y >= LCD_HEIGHT) {
    return;
  }
  if (mlcd.Inverted) {
    color = (LCD_COLOR) !color;
  }
  if (color == White) {
    mlcd.Buffer[buff_addr] |= bit_to_set;
  } else {
    mlcd.Buffer[buff_addr] &= ~bit_to_set;
  }
}

char mlcd_WriteChar(char ch, FontType Font, LCD_COLOR color) {
  uint32_t i, b, j;
  if (ch < 32 || ch > 126)
    return 0;
  if (LCD_WIDTH < (mlcd.CurrentX + Font.FontWidth)) {
    if (LCD_HEIGHT > (mlcd.CurrentY + 2 * Font.FontHeight - 2)) {        // -2 => Margin of the character
      mlcd.CurrentX = 0;
      mlcd.CurrentY = mlcd.CurrentY + Font.FontHeight - 1;        //-1 => Margin of the character fort
    } else {
      return 0;
    }
  }

  for (i = 0; i < Font.FontHeight; i++) {
    b = Font.data[(ch - 32) * Font.FontHeight + i];
    for (j = 0; j < Font.FontWidth; j++) {
      if ((b << j) & 0x8000) {
        mlcd_DrawPixel(mlcd.CurrentX + j, (mlcd.CurrentY + i), (LCD_COLOR) color);
      } else {
        mlcd_DrawPixel(mlcd.CurrentX + j, (mlcd.CurrentY + i), (LCD_COLOR) !color);
      }
    }
  }
  mlcd.CurrentX += Font.FontWidth;
  return ch;
}

void mlcd_Refresh(void) {
  uint8_t cmd_data = BIT_WRITECMD;
  uint8_t adr_data = 0x01;
  uint8_t dmy_data = 0x00;

  HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_SET);

  HAL_SPI_Transmit(&hspi1, &cmd_data, 1, HAL_MAX_DELAY);

  for (uint16_t s_add = 1; s_add <= LCD_HEIGHT; s_add++) {
    adr_data = s_add;
    if (s_add != 1)
      HAL_SPI_Transmit(&hspi1, &dmy_data, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, &adr_data, 1, HAL_MAX_DELAY);
    uint16_t step = LCD_WIDTH / 8;
    for (uint16_t s_data = 0; s_data < LCD_WIDTH / 8; s_data++) {
      uint16_t index = s_data + (s_add - 1) * step;
      HAL_SPI_Transmit(&hspi1, &mlcd.Buffer[index], 1, HAL_MAX_DELAY);
    }
    HAL_SPI_Transmit(&hspi1, &dmy_data, 1, HAL_MAX_DELAY);
  }

  HAL_SPI_Transmit(&hspi1, &dmy_data, 1, HAL_MAX_DELAY);
  HAL_SPI_Transmit(&hspi1, &dmy_data, 1, HAL_MAX_DELAY);

  HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_RESET);
}
