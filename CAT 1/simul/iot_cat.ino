#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <LedControl.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
LedControl matrix = LedControl(11, 13, 10, 1);

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int joyX = A0;
int joyY = A1;

int boxX = 3;
int boxY = 3;
int craneX = 0;
int craneY = 0;

bool pickupMode = false;
bool boxPickedUp = false;

void setup()
{
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Crane Ready");

  matrix.shutdown(0, false);
  matrix.setIntensity(0, 8);
  matrix.clearDisplay(0);

  pinMode(joyX, INPUT);
  pinMode(joyY, INPUT);

  updateMatrix();
}

void loop()
{
  handleJoystick();
  handleKeyPress();
  delay(200);
}

void handleJoystick()
{
  int xValue = analogRead(joyX);
  int yValue = analogRead(joyY);

  String direction = "";
  bool moved = false;

  if (xValue < 300)
  {
    craneX = ((craneX - 1) + 8) % 8;
    direction = "Left";
    moved = true;
  }
  else if (xValue > 700)
  {
    craneX = (craneX + 1) % 8;
    direction = "Right";
    moved = true;
  }

  if (yValue < 300)
  {
    craneY = (craneY + 1) % 8;
    direction = "Down";
    moved = true;
  }
  else if (yValue > 700)
  {
    craneY = ((craneY - 1) + 8) % 8;
    direction = "Up";
    moved = true;
  }

  if (moved)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Move: ");
    lcd.print(direction);
    lcd.setCursor(0, 1);
    lcd.print("Pos: ");
    lcd.print(craneX);
    lcd.print(",");
    lcd.print(craneY);

    if (boxPickedUp)
    {
      boxX = craneX;
      boxY = craneY;
    }

    updateMatrix();
  }
}

void handleKeyPress()
{
  char key = keypad.getKey();

  if (key)
  {
    if (key == '1')
    {
      pickupMode = true;
      if (craneX == boxX && craneY == boxY && !boxPickedUp)
      {
        boxPickedUp = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Mode: Pickup");
        lcd.setCursor(0, 1);
        lcd.print("Box Picked Up!");
      }
      else if (boxPickedUp)
      {
        boxPickedUp = false;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Mode: Drop");
        lcd.setCursor(0, 1);
        lcd.print("Box Dropped!");
      }
      updateMatrix();
    }
    else if (key == '2')
    {
      pickupMode = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mode: Move");
      lcd.setCursor(0, 1);
      lcd.print("Use Joystick");
    }
  }
}

void updateMatrix()
{
  matrix.clearDisplay(0);

  if (!boxPickedUp)
  {
    matrix.setLed(0, boxY, boxX, true);
  }

  matrix.setLed(0, craneY, craneX, true);

  if (craneX == boxX && craneY == boxY)
  {
    for (int i = 0; i < 5; i++)
    {
      matrix.setLed(0, craneY, craneX, false);
      delay(100);
      matrix.setLed(0, craneY, craneX, true);
      delay(100);
    }
  }
}
