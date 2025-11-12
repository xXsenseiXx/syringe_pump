#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- PHYSICAL CALIBRATION CONSTANT (CRITICAL! ADJUST FOR YOUR SETUP!) ---
// This example is for:
// - A 50mL syringe with a ~30mm inner diameter.
// - An A4988 driver in FULL-STEP mode (MS pins disconnected).
// - A 200 step/rev motor.
// - An 8mm pitch leadscrew.
// Calculation: (200 steps_per_rev / 8 mm_per_rev) * (1.414 mm_plunger_travel_per_mL) = 35.35
#define STEPS_PER_ML 35.35 

// --- Pin Definitions ---
#define STEP_PIN 33
#define DIR_PIN  32
#define CLK_PIN  14
#define DT_PIN   27
#define SW_PIN   26

// --- LCD I2C Setup ---
#define LCD_I2C_ADDRESS 0x27
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, 16, 2);

// --- Encoder and State Variables ---
int lastClkState;
unsigned long lastInputTime = 0;
int menuState = 0; // 0 = main menu, 1 = editing a value

// --- Menu Items (Simplified) ---
const char* mainMenuItems[] = {"Volume (mL)", "Time (min)", "Start Infusion"};
const int mainMenuItemCount = 3;
int currentMenuItem = 0;

// --- Value Storage ---
float volumeToSet_mL = 30.0; // Default volume
int   timeToSet_min = 1;     // Default time

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);

  lastClkState = digitalRead(CLK_PIN);
  drawMenu();
}

void loop() {
  handleEncoder();
}

void drawMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);

  if (menuState == 0) { // In the Main Menu
    lcd.print("Syringe Pump");
    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(mainMenuItems[currentMenuItem]);
  } else if (menuState == 1) { // In Editing Mode
    lcd.print("Set ");
    lcd.print(mainMenuItems[currentMenuItem]);
    lcd.setCursor(0, 1);
    lcd.print("Value: ");
    if (currentMenuItem == 0) lcd.print(volumeToSet_mL, 1);
    if (currentMenuItem == 1) lcd.print(timeToSet_min);
  }
}

void handleEncoder() {
  unsigned long currentTime = millis();
  if (currentTime - lastInputTime < 400) return;

  if (digitalRead(SW_PIN) == LOW) {
    if (menuState == 0) {
      if (currentMenuItem == 2) { // "Start Infusion" is selected
        runInfusion();
      } else {
        menuState = 1; // Enter editing mode for Volume or Time
      }
    } else {
      menuState = 0; // Exit editing mode
    }
    drawMenu();
    lastInputTime = currentTime + 300;
    return;
  }

  int currentClkState = digitalRead(CLK_PIN);
  if (currentClkState != lastClkState) {
    bool movedRight = (digitalRead(DT_PIN) != currentClkState);
    if (menuState == 0) {
      if (movedRight) currentMenuItem = (currentMenuItem + 1) % mainMenuItemCount;
      else currentMenuItem = (currentMenuItem - 1 + mainMenuItemCount) % mainMenuItemCount;
    } else {
      if (currentMenuItem == 0) {
        if (movedRight) volumeToSet_mL += 1;
        else volumeToSet_mL -= 1;
        if (volumeToSet_mL < 0) volumeToSet_mL = 0;
      }
      if (currentMenuItem == 1) {
        if (movedRight) timeToSet_min += 1;
        else timeToSet_min -= 1;
        if (timeToSet_min < 1) timeToSet_min = 1;
      }
    }
    drawMenu();
    lastInputTime = currentTime;
  }
  lastClkState = currentClkState;
}

void runInfusion() {
  long totalSteps = round(volumeToSet_mL * STEPS_PER_ML);
  unsigned long totalTime_us = (unsigned long)timeToSet_min * 60 * 1000000;
  
  if (totalSteps <= 0 || totalTime_us <= 0) {
    lcd.clear(); lcd.print("Invalid values!"); delay(2000); return;
  }
  
  int speedDelay = totalTime_us / (totalSteps * 2);

  lcd.clear();
  lcd.print("Infusing...");
  lcd.setCursor(0, 1);
  lcd.print(String(volumeToSet_mL, 1) + "mL in " + String(timeToSet_min) + "min");

  digitalWrite(DIR_PIN, LOW); // Set direction to dispense fluid
  
  stepMotor(totalSteps, speedDelay);

  lcd.clear(); lcd.print("Infusion"); lcd.setCursor(0,1); lcd.print("Complete!"); delay(3000);
}

void stepMotor(long steps, int delayVal) {
  for (long i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(delayVal);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(delayVal);
  }
}