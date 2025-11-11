#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- Pin Definitions ---
// Stepper Motor Pins
#define STEP_PIN 12   // STEP -> A4988 STEP
#define DIR_PIN  13   // DIR  -> A4988 DIR

// Rotary Encoder Pins
#define CLK_PIN 14
#define DT_PIN 27
#define SW_PIN 26

// --- LCD I2C Setup ---
#define LCD_I2C_ADDRESS 0x27 // Change if your address is different
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, 16, 2);

// --- Encoder and State Variables ---
int lastClkState;
unsigned long lastInputTime = 0;
int menuState = 0; // 0 = main menu, 1 = editing a value

// --- Menu Items ---
const char* mainMenuItems[] = {"Steps", "Speed (delay)", "Start Motor"};
const int mainMenuItemCount = 3;
int currentMenuItem = 0;

// --- Value Storage ---
long stepsToSet = 400;   // Default number of steps
int speedDelay = 1000;   // Default delay in microseconds (lower value = faster)

void setup() {
  Serial.begin(115200);

  // --- Initialize LCD ---
  lcd.init();
  lcd.backlight();

  // --- Initialize Pin Modes ---
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);

  // Read initial encoder state
  lastClkState = digitalRead(CLK_PIN);

  // Draw the initial menu screen
  drawMenu();
}

void loop() {
  // The program is entirely driven by user input from the encoder
  handleEncoder();
}

void drawMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);

  if (menuState == 0) { // --- In the Main Menu ---
    lcd.print("Stepper Control");
    lcd.setCursor(0, 1);
    lcd.print(">");
    lcd.print(mainMenuItems[currentMenuItem]);

  } else if (menuState == 1) { // --- In Editing Mode ---
    lcd.print("Set ");
    lcd.print(mainMenuItems[currentMenuItem]);
    lcd.setCursor(0, 1);
    lcd.print("Value: ");

    // Display the value being edited
    if (currentMenuItem == 0) lcd.print(stepsToSet);
    if (currentMenuItem == 1) lcd.print(speedDelay);
  }
}

void handleEncoder() {
  unsigned long currentTime = millis();
  // Debounce delay for encoder inputs, 150ms is a good starting point
  if (currentTime - lastInputTime < 150) {
    return;
  }

  // --- Handle Button Press ---
  if (digitalRead(SW_PIN) == LOW) {
    if (menuState == 0) { // If in the main menu...
      if (currentMenuItem == 2) { // "Start Motor" is selected
        lcd.clear();
        lcd.print("Motor Running...");
        lcd.setCursor(0,1);
        lcd.print("Steps: " + String(stepsToSet));
        
        // Set direction based on the sign of stepsToSet
        digitalWrite(DIR_PIN, (stepsToSet > 0) ? HIGH : LOW);
        stepMotor(abs(stepsToSet), speedDelay); // Move the motor
        
        delay(500); // Pause briefly after the motor stops
      } else {
        // Enter editing mode for "Steps" or "Speed"
        menuState = 1;
      }
    } else { // If in editing mode...
      // Exit back to the main menu
      menuState = 0;
    }
    
    drawMenu(); // Redraw the menu after any action
    lastInputTime = currentTime + 300; // Add extra delay to prevent accidental double press
    return; // Exit function immediately after handling the click
  }

  // --- Handle Rotation ---
  int currentClkState = digitalRead(CLK_PIN);
  if (currentClkState != lastClkState) {
    bool movedRight = (digitalRead(DT_PIN) != currentClkState);
    
    if (menuState == 0) { // Navigate the main menu
      if (movedRight) {
        currentMenuItem = (currentMenuItem + 1) % mainMenuItemCount;
      } else {
        currentMenuItem = (currentMenuItem - 1 + mainMenuItemCount) % mainMenuItemCount;
      }
    } else { // Change values in edit mode
      if (currentMenuItem == 0) { // Editing "Steps"
        if (movedRight) stepsToSet += 10;
        else stepsToSet -= 10;
      }
      if (currentMenuItem == 1) { // Editing "Speed (delay)"
        if (movedRight) {
          speedDelay -= 100; // DECREASE delay to go FASTER
        } else {
          speedDelay += 100; // INCREASE delay to go SLOWER
        }
        // Add constraints to prevent stall or extremely slow speed
        speedDelay = max(400, speedDelay);   // Max speed (400us is very fast)
        speedDelay = min(10000, speedDelay); // Min speed
      }
    }

    drawMenu();
    lastInputTime = currentTime;
  }
  lastClkState = currentClkState;
}

// Modified stepMotor function now accepts a speed delay value
void stepMotor(long steps, int delayVal) {
  for (long i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(delayVal);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(delayVal);
  }
}