#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <XPT2046_Touchscreen.h> // Library for XPT2046 touch controller
#include <SPI.h>

// The display object
TFT_eSPI tft = TFT_eSPI();

// Touch screen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

// Touch screen object
SPIClass touchscreenSPI(VSPI); 
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// Keypad start position, key size, and spacing
#define KEY_X 20
#define KEY_Y 70
#define KEY_W 45
#define KEY_H 40
#define KEY_SPACING_X 10
#define KEY_SPACING_Y 10
#define KEY_TEXTSIZE 2

// Using two fonts since numbers are nice in proportionally spaced fonts
#define LABEL1_FONT &FreeSansBold12pt7b // Key label font 1
#define LABEL2_FONT &FreeSerif24pt7b // Key label font 2

// Numeric display box size and location
#define DISP_X 10
#define DISP_Y 10
#define DISP_W 220
#define DISP_H 50
#define DISP_TSIZE 3
#define DISP_TCOLOR TFT_CYAN

// Number length, buffer for storing input
#define NUM_LEN 32
char numberBuffer[NUM_LEN + 1] = ""; // Zero terminate

// We have 5 rows of 4 keys
char keyLabel[5][4] = {
  {'7','8','9','/'},
  {'4','5','6','*'},
  {'1','2','3','-'},
  {'0','.','%','+'},
  {'C','(',')','='}
};

void drawKeypad() {
  // Draw the keys
  for (uint8_t row = 0; row < 5; row++) {
    for (uint8_t col = 0; col < 4; col++) {
      // Draw the key outline
      tft.fillRect(KEY_X + col * (KEY_W + KEY_SPACING_X), KEY_Y + row * (KEY_H + KEY_SPACING_Y), KEY_W, KEY_H, TFT_BLUE);
      // Draw the key label
      tft.setTextColor(TFT_WHITE);
      tft.setFreeFont(LABEL2_FONT);
      tft.drawCentreString(String(keyLabel[row][col]), KEY_X + col * (KEY_W + KEY_SPACING_X) + KEY_W / 2, KEY_Y + row * (KEY_H + KEY_SPACING_Y) + 10, 2);
    }
  }
}

void setup() {
  // Initialize display
  tft.init();

  // Initialize touch screen SPI
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(0); // Set touchscreen to portrait mode
  tft.setRotation(0); // Set display to portrait mode

  tft.fillScreen(TFT_BLACK);

  // Draw number display box
  tft.fillRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_WHITE);
  
  // Draw keypad
  drawKeypad();
}

float evaluateExpression(String expr) {
  float result = 0;
  float num = 0;
  char lastOperator = '+';
  String number = "";

  for (int i = 0; i < expr.length(); i++) {
    char c = expr.charAt(i);

    if (isdigit(c) || c == '.') {
      number += c; // Build the number string
    } else {
      num = number.toFloat(); // Convert the string to a float
      number = ""; // Reset the number string

      switch (lastOperator) {
        case '+': result += num; break;
        case '-': result -= num; break;
        case '*': result *= num; break;
        case '/': result /= num; break;
        case '%': result = (int)result % (int)num; break;
      }

      lastOperator = c; // Update the operator
    }
  }

  // Apply the last pending operation
  num = number.toFloat(); // Convert the last number
  switch (lastOperator) {
    case '+': result += num; break;
    case '-': result -= num; break;
    case '*': result *= num; break;
    case '/': result /= num; break;
    case '%': result = (int)result % (int)num; break;
  }

  return result;
}

void loop() {
  // Check if the display is touched
  if (touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    // Map from touch coordinates to screen coordinates
    p.x = map(p.x, 200, 3700, 0, 240); // Map for portrait mode
    p.y = map(p.y, 240, 3800, 0, 320); // Map for portrait mode

    // Check which key is pressed
    for (uint8_t row = 0; row < 5; row++) {
      for (uint8_t col = 0; col < 4; col++) {
        if ((p.x > KEY_X + col * (KEY_W + KEY_SPACING_X)) && (p.x < KEY_X + col * (KEY_W + KEY_SPACING_X) + KEY_W) &&
            (p.y > KEY_Y + row * (KEY_H + KEY_SPACING_Y)) && (p.y < KEY_Y + row * (KEY_H + KEY_SPACING_Y) + KEY_H)) {
          char key = keyLabel[row][col];
          if (key == 'C') {
            // Clear the number buffer
            numberBuffer[0] = '\0';
          } else if (key == '=') {
            // Evaluate the expression and display the result
            float result = evaluateExpression(String(numberBuffer));
            snprintf(numberBuffer, sizeof(numberBuffer), "%.2f", result); // Show result with 2 decimal places
          } else if (strlen(numberBuffer) < NUM_LEN) {
            // Add the key to the number buffer
            numberBuffer[strlen(numberBuffer) + 1] = '\0';
            numberBuffer[strlen(numberBuffer)] = key;
          }
          // Update the display
          tft.fillRect(DISP_X + 2, DISP_Y + 2, DISP_W - 4, DISP_H - 4, TFT_BLACK);
          tft.setTextColor(DISP_TCOLOR);
          tft.drawCentreString(numberBuffer, DISP_X + DISP_W / 2, DISP_Y + 10, 4);
          delay(300); // Debounce delay
        }
      }
    }
  }
}
