#include <LiquidCrystal.h>
#include "pitches.h"
#include "ascii.h"

// initialize the LCD with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// passive buzzer pin
#define BUZZER 5

// pins of left, center and right buttons
#define L_BUTTON 2
#define C_BUTTON 3
#define R_BUTTON 4

// ui ascii art
#define MUSICALNOTE 0
#define CHECKMARK 1
#define HEART 2
#define CANCEL 3

// duration of buzzer alarm
#define ALARM_DURATION 1000
#define CLICK_DURATION 100

#define REFRESH_DELTA 1000

// states of the three buttons
struct Button {
  // Initialize the states as HIGH, meaning the buttons were never pressed.
  int state = HIGH, previous_state = HIGH, pin;
  bool pressed = false;
} l_button, c_button, r_button;

// tone of alarm
int alarm_tone = NOTE_C5;
int click_tone = NOTE_C6;

/*
 * MENU_STATE:
 * 0 - choosing the study time
 * 1 - ask for start
 * 2 - studying
 * 3 - ask for break
 * 4 - break
 * Then go to 1.
 */
int MENU_STATE = 0;

// don't force the display uselessly.
bool DISPLAY_NEEDS_REFRESH = false;

// flag for blinking
bool BLINK_ART = false;

// the default is 25 minutes
unsigned long study_amount = 25;
unsigned long break_amount = 5;

// timers
unsigned long start_time = 0;

// call to check if buttons were pressed
void init_buttons() {
  init_button(l_button);
  init_button(c_button);
  init_button(r_button);
}

void init_button(Button& button) {
  button.state = digitalRead(button.pin);
  // If the current state is LOW, then we got a press.
  button.pressed = (button.state == LOW && button.previous_state == HIGH);
}

// call to clean up the states
void clean_buttons() {
  clean_button(l_button);
  clean_button(c_button);
  clean_button(r_button);
}

void clean_button(Button& button) {
  button.previous_state = button.state;
  button.pressed = false;
}

void playAlarmSound(int pitch) {
  tone(BUZZER, pitch, ALARM_DURATION);
}

void playClickSound(int pitch) {
  tone(BUZZER, pitch, CLICK_DURATION);
}

void intro() {
  lcd.print("    Pomodoro!");
  delay(1000 * 2);
  lcd.setCursor(0, 1);
  lcd.print("   by extremq");
  delay(1000 * 2);
  
  DISPLAY_NEEDS_REFRESH = true;
}

void time_set_ui(int minutes) {
  lcd.setCursor(0, 0);
  String temp = "Set time: " + String(minutes, DEC) + ":00";
  lcd.print(temp);
  lcd.setCursor(0, 1);
  lcd.print("-");
  lcd.setCursor(7, 1);
  lcd.write(byte(CHECKMARK));
  lcd.setCursor(15, 1);
  lcd.print("+");

  DISPLAY_NEEDS_REFRESH = false;
}

void ask_study_ui() {
  lcd.setCursor(0, 0);
  lcd.print(" Begin session?");
  lcd.setCursor(7, 1);
  lcd.write(byte(CHECKMARK));

  DISPLAY_NEEDS_REFRESH = false;
}

unsigned long LAST_TIMER_REFRESH = 0;
void study_timer_ui() {
  lcd.setCursor(0, 0);
  lcd.print("   Studying");
  lcd.setCursor(12, 0);
  
  if (BLINK_ART == true)
    lcd.write(byte(HEART));
  else
    lcd.print(" ");

  BLINK_ART = !BLINK_ART;
  
  unsigned long current_time = millis();
  unsigned long delta_seconds = (current_time - start_time) / 1000;
  unsigned long remaining_seconds = study_amount * 60 - delta_seconds;
  unsigned long remaining_minutes = remaining_seconds / 60;

  String temp = "";
  if (remaining_minutes < 10)
    temp = "0";

  temp = temp + String(remaining_minutes, DEC) + ":";

  if (remaining_seconds % 60 < 10)
    temp = temp + "0";

  temp = temp + String(remaining_seconds % 60, DEC);

  lcd.setCursor(0, 1);
  lcd.write(byte(CANCEL));
  lcd.setCursor(5, 1);
  lcd.print(temp);
  lcd.setCursor(15, 1);
  lcd.print(">");

  LAST_TIMER_REFRESH = current_time;
  DISPLAY_NEEDS_REFRESH = false;
}

void ask_break_ui() {
  lcd.setCursor(0, 0);
  lcd.print("  Begin break?");
  lcd.setCursor(7, 1);
  lcd.write(byte(CHECKMARK));

  DISPLAY_NEEDS_REFRESH = false;
}

void break_timer_ui() {
  lcd.setCursor(0, 0);
  lcd.print("   Relaxing");
  lcd.setCursor(12, 0);

  if (BLINK_ART == true)
    lcd.write(byte(MUSICALNOTE));
  else
    lcd.print(" ");

  BLINK_ART = !BLINK_ART;
  
  unsigned long current_time = millis();
  unsigned long delta_seconds = (current_time - start_time) / 1000;
  unsigned long remaining_seconds = break_amount * 60 - delta_seconds;
  unsigned long remaining_minutes = remaining_seconds / 60;

  String temp = "";
  if (remaining_minutes < 10)
    temp = "0";

  temp = temp + String(remaining_minutes, DEC) + ":";

  if (remaining_seconds % 60 < 10)
    temp = temp + "0";

  temp = temp + String(remaining_seconds % 60, DEC);

  lcd.setCursor(0, 1);
  lcd.write(byte(CANCEL));
  lcd.setCursor(5, 1);
  lcd.print(temp);
  lcd.setCursor(15, 1);
  lcd.print(">");

  LAST_TIMER_REFRESH = current_time;
  DISPLAY_NEEDS_REFRESH = false;
}

void setup() {
  Serial.begin(9600);
  
  pinMode(BUZZER, OUTPUT);
  pinMode(L_BUTTON, INPUT_PULLUP);
  pinMode(C_BUTTON, INPUT_PULLUP);
  pinMode(R_BUTTON, INPUT_PULLUP);

  // set lcd size
  lcd.begin(16, 2);
  lcd.createChar(MUSICALNOTE, Sound);
  lcd.createChar(CHECKMARK, Check);
  lcd.createChar(HEART, Heart);
  lcd.createChar(CANCEL, Cancel);
  lcd.clear();
  
  l_button.pin = L_BUTTON;
  c_button.pin = C_BUTTON;
  r_button.pin = R_BUTTON;

  intro();
}

void loop() {
  init_buttons();

  // some actions request an lcd redraw.
  if (DISPLAY_NEEDS_REFRESH == true)  {
    lcd.clear();
  }
  
  switch(MENU_STATE) {
    case 0:
      // only redraw the lcd if it has been cleared
      if (DISPLAY_NEEDS_REFRESH == true)
        time_set_ui(study_amount);

      // decrease study minutes by 1
      if (l_button.pressed) {
        study_amount = (study_amount == 1 ? 55 : study_amount - 1);
      }
      // confirm the selection
      else if (c_button.pressed) {
        MENU_STATE = 1;
      }
      // increase study minutes by 1
      else if (r_button.pressed) {
        study_amount = (study_amount + 1) % 56;
        study_amount = (study_amount == 0 ? 1 : study_amount);
      }

      // for this menu, any button press requires an lcd update.
      if (l_button.pressed || c_button.pressed || r_button.pressed) {
        playClickSound(click_tone);
        DISPLAY_NEEDS_REFRESH = true;
      }
      break;
    case 1:
      // ask to begin.
      if (DISPLAY_NEEDS_REFRESH == true)
        ask_study_ui();

      if (c_button.pressed) {
        playClickSound(click_tone);
        MENU_STATE = 2;
        start_time = millis();
        DISPLAY_NEEDS_REFRESH = true;
      }
      break;
    case 2:
      // Send back to settings
      if (l_button.pressed) {
        playClickSound(click_tone);
        MENU_STATE = 0;
        DISPLAY_NEEDS_REFRESH = true;
      }
      // This marks the end of a session
      if ((millis() - start_time >= study_amount * 60 * 1000) || r_button.pressed) {
        playAlarmSound(NOTE_C5);
        MENU_STATE = 3;
        DISPLAY_NEEDS_REFRESH = true;
      }
      
      // Let's not force the lcd to refresh a lot
      if (millis() - LAST_TIMER_REFRESH > REFRESH_DELTA) {
        study_timer_ui();
      }
      break;
    case 3:
      // ask for break.
      if (DISPLAY_NEEDS_REFRESH == true)
        ask_break_ui();

      if (c_button.pressed) {
        playClickSound(click_tone);
        MENU_STATE = 4;
        start_time = millis();
        DISPLAY_NEEDS_REFRESH = true;
      }
      break;
    case 4:
      // Send back to settings
      if (l_button.pressed) {
        playClickSound(click_tone);
        MENU_STATE = 0;
        DISPLAY_NEEDS_REFRESH = true;
      }
      
      // This marks the end of a break
      if ((millis() - start_time >= break_amount * 60 * 1000) || r_button.pressed) {
        playAlarmSound(NOTE_C5);
        MENU_STATE = 1;
        DISPLAY_NEEDS_REFRESH = true;
      }
      
      // Let's not force the lcd to refresh a lot
      if (millis() - LAST_TIMER_REFRESH > REFRESH_DELTA) {
        break_timer_ui();
      }
      break;
  }
  
  clean_buttons();
}
