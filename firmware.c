/* SnackyMini Keyboard Firmware
   By @huydotnet
   You must select Keyboard from the "Tools > USB Type" menu.
*/

#include <Keyboard.h>

const int ROWS = 4;
const int COLS = 12;
const int REGISTER_DELAY = 25;
const int REPEAT_DELAY = 80;
const int MAXIMUM_STROKES = 10;
const int SUPPORTED_STROKES = 6;

int frameSkipCount = 0;
unsigned long lastFrame = 0;

char refCode[ROWS][COLS] = {
  { 0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23 },
  { 30, 31, 32, 40, 41, 42, 43, 50, 51, 52, 60, 61 },
  { 62, 63, 70, 71, 72, 80, 81, 82, 83, 90, 91, 92 },
  { 93, 100, 101, 102, 255, 103, 255, 110, 255, 111, 112, 113 }
};

#define ARROW_UP_KEY 91
#define ARROW_DOWN_KEY 112
#define ARROW_LEFT_KEY 111
#define ARROW_RIGHT_KEY 113
#define SHIFT_KEY 62 // trigger layout change
#define MENU_KEY 93 // trigger layout change
#define FN_KEY 110 // trigger layout change
// non combinable modifiers
#define BACKSPACE_KEY 61
#define ENTER_KEY 102
#define TAB_KEY 103
// combinable modifiers
#define SUPER_KEY 101
#define CTRL_KEY 30
#define ALT_KEY 100

int released = 0;

char layout[ROWS][COLS] = {
  { KEY_TILDE, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_BACKSLASH },
  { ' ', KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, ' ' },
  { ' ', KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, ' ', KEY_SLASH },
  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' }
};

char layout_shift[ROWS][COLS] = {
  { KEY_TILDE, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_BACKSLASH },
  { ' ', KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, ' ' },
  { ' ', KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, ' ', KEY_SLASH },
  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' }
};

char layout_fn[ROWS][COLS] = {
  { KEY_TAB, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS },
  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', KEY_LEFT_BRACE, KEY_RIGHT_BRACE, ' ' },
  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', KEY_QUOTE, ' ', KEY_EQUAL },
  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' }
};

int rowPins[ROWS] = { 23, 22, 21, 20 };
int colPins[COLS] = { 19, 18, 17, 16, 15, 12, 11, 10, 9, 8, 7, 6 };

struct Key {
  int row;
  int col;
  int code;
};

struct Point {
  int r;
  int c;
};

struct Point keyToPoint(int code) {
  struct Point p;
  p.r = -1; p.c = -1;
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      if (refCode[r][c] == code) {
        p.r = r; p.c = c;
      }
    }
  }
  return p;
}

struct Key* readKey() {
  struct Key* result = (Key*)malloc(MAXIMUM_STROKES * sizeof(struct Key));
  for (int i = 0; i < MAXIMUM_STROKES; i++) {
    result[i].col = -1;
    result[i].row = -1;
    result[i].code = -1;
  }
  int cur = 0;

  for (int row = 0; row < ROWS; row++) {
    pinMode(rowPins[row], OUTPUT);
    digitalWrite(rowPins[row], LOW);
    for (int col = 0; col < COLS; col++) {
      if (digitalRead(colPins[col]) == LOW) {
        result[cur].row = row;
        result[cur].col = col;
        result[cur].code = col * 10 + row;
        if (cur < MAXIMUM_STROKES) cur++;
        digitalWrite(colPins[col], HIGH);
      }
    }
    pinMode(rowPins[row], INPUT);
    digitalWrite(rowPins[row], LOW);
  }

  return result;
}

void setup()
{
  Serial.begin(9600);

  for (int i = 0; i < ROWS; i++) {
    pinMode(rowPins[i], INPUT);
    digitalWrite(rowPins[i], HIGH);
  }
  
  for (int i = 0; i < COLS; i++) {
    pinMode(colPins[i], INPUT);
    digitalWrite(colPins[i], HIGH);
  }
}

void holdKey(int code) {
  Keyboard.press(code);
}

void pressKey(int code) {
  Keyboard.press(code);
  Keyboard.release(code);
}

void submitLayout(struct Key* keys, char layout[ROWS][COLS]) {
  int modifiers = 0;
  Keyboard.set_modifier(0);
  Keyboard.set_key1(0);
  for (int i = 0; i < SUPPORTED_STROKES; i++) {
    // non combinable
    if (keys[i].code == BACKSPACE_KEY) {
      Keyboard.set_key1(KEY_BACKSPACE);
    }
    else if (keys[i].code == TAB_KEY) {
      Keyboard.set_key1(KEY_SPACE);
    }
    else if (keys[i].code == ENTER_KEY) {
      Keyboard.set_key1(KEY_ENTER);
    }
    // combinable
    else if (keys[i].code == SUPER_KEY) {
      modifiers |= MODIFIERKEY_GUI;
    }
    else if (keys[i].code == CTRL_KEY) {
      modifiers |= MODIFIERKEY_CTRL;
    }
    else if (keys[i].code == ALT_KEY) {
      modifiers |= MODIFIERKEY_ALT;
    }
    else if (keys[i].code == SHIFT_KEY) {
      modifiers |= MODIFIERKEY_SHIFT;
    }
    else if (keys[i].code == ARROW_UP_KEY) {
      Keyboard.set_key1(KEY_UP);
    }
    else if (keys[i].code == ARROW_DOWN_KEY) {
      Keyboard.set_key1(KEY_DOWN);
    }
    else if (keys[i].code == ARROW_LEFT_KEY) {
      Keyboard.set_key1(KEY_LEFT);
    }
    else if (keys[i].code == ARROW_RIGHT_KEY) {
      Keyboard.set_key1(KEY_RIGHT);
    }
    else {
      if (keys[i].code != -1) {
        struct Point pos = keyToPoint(keys[i].code);
        char c = layout[pos.r][pos.c];
        if (c != ' ') {
          Keyboard.set_key1(c);
        }
      }
    }
  }
  if (modifiers) {
    Keyboard.set_modifier(modifiers);
  }
  Keyboard.send_now();
  delay(REGISTER_DELAY);
}

void keySubmit(struct Key* keys) {
  int layoutId = 0;
  for (int i = 0; i < SUPPORTED_STROKES; i++) {
    if (keys[i].code == SHIFT_KEY) {
      Serial.println("SHIFT LAYOUT");
      layoutId = 1; break;
    } else if (keys[i].code == MENU_KEY || keys[i].code == FN_KEY) {
      Serial.println("FN LAYOUT");
      layoutId = 2; break;
    }
  }
  if (layoutId == 0) {
    submitLayout(keys, layout);
  } else if (layoutId == 1) {
    submitLayout(keys, layout_shift);
  } else if (layoutId == 2) {
    submitLayout(keys, layout_fn);
  }
}

void loop()
{
  unsigned long timeNow = millis();
  // read every 1 millisecond
  if (timeNow != lastFrame) {
    frameSkipCount++;
    if (frameSkipCount == REGISTER_DELAY) {
      // Begin process
      struct Key* keys = readKey();
      if (keys[0].row != -1 && keys[0].col != -1) {
        keySubmit(keys);
      } else {
        frameSkipCount = 0;
        Keyboard.releaseAll();
      }
      free(keys);
    }
    if (frameSkipCount == REPEAT_DELAY) {
      frameSkipCount = 0;
      Keyboard.releaseAll();
    }
    // record last key state and frame
    lastFrame = timeNow;
  }
}
