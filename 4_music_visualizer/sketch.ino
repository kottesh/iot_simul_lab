#include <LedControl.h>

LedControl matrix = LedControl(11, 13, 10, 1);

const int ledPins[] = {2, 3, 4, 5, 6, 7, A0, A1, A2, A3};
const int ledCount = 10;
const int buzzerPin = 12;
const int btnNext = 8;
const int btnPrev = 9;

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST 0

struct Note {
  int frequency;
  int duration;
};

struct Song {
  const char* name;
  const char* album;
  Note* notes;
  int length;
};

Note marioTheme[] = {
  {NOTE_E5, 150}, {NOTE_E5, 150}, {REST, 150}, {NOTE_E5, 150},
  {REST, 150}, {NOTE_C5, 150}, {NOTE_E5, 150}, {REST, 150},
  {NOTE_G5, 150}, {REST, 450}, {NOTE_G4, 150}, {REST, 450},
  
  {NOTE_C5, 150}, {REST, 300}, {NOTE_G4, 150}, {REST, 300},
  {NOTE_E4, 150}, {REST, 300}, {NOTE_A4, 150}, {REST, 150},
  {NOTE_B4, 150}, {REST, 150}, {NOTE_AS4, 150}, {NOTE_A4, 150},
  {REST, 150},
  
  {NOTE_G4, 120}, {NOTE_E5, 120}, {NOTE_G5, 120}, {NOTE_A5, 150},
  {REST, 150}, {NOTE_F5, 150}, {NOTE_G5, 150}, {REST, 150},
  {NOTE_E5, 150}, {REST, 150}, {NOTE_C5, 150}, {NOTE_D5, 150},
  {NOTE_B4, 150}, {REST, 300}
};

Note marioUnderworld[] = {
  {NOTE_C4, 150}, {NOTE_C5, 150}, {NOTE_A3, 150}, {NOTE_A4, 150},
  {NOTE_AS3, 150}, {NOTE_AS4, 150}, {REST, 300},
  
  {NOTE_C4, 150}, {NOTE_C5, 150}, {NOTE_A3, 150}, {NOTE_A4, 150},
  {NOTE_AS3, 150}, {NOTE_AS4, 150}, {REST, 300},
  
  {NOTE_F3, 150}, {NOTE_F4, 150}, {NOTE_D3, 150}, {NOTE_D4, 150},
  {NOTE_DS3, 150}, {NOTE_DS4, 150}, {REST, 300},
  
  {NOTE_F3, 150}, {NOTE_F4, 150}, {NOTE_D3, 150}, {NOTE_D4, 150},
  {NOTE_DS3, 150}, {NOTE_DS4, 150}, {REST, 300},
  
  {NOTE_DS4, 225}, {NOTE_CS4, 225}, {NOTE_D4, 225},
  {NOTE_CS4, 150}, {NOTE_DS4, 150}, {NOTE_DS4, 150}, {NOTE_GS3, 150},
  {NOTE_G3, 150}, {NOTE_CS4, 150}
};

Note marioPowerUp[] = {
  {NOTE_G4, 100}, {NOTE_B4, 100}, {NOTE_D5, 100}, {NOTE_G5, 100},
  {NOTE_B5, 100}, {NOTE_G5, 100}, {NOTE_B5, 100}, {NOTE_D6, 100},
  {NOTE_G6, 100}, {NOTE_B6, 100}, {NOTE_G6, 100}, {NOTE_B6, 100},
  {NOTE_D7, 100}, {REST, 200}
};

Note marioCoin[] = {
  {NOTE_B5, 100}, {NOTE_E6, 400}, {REST, 200}
};

Note marioGameOver[] = {
  {NOTE_C5, 150}, {NOTE_G4, 150}, {NOTE_E4, 150},
  {NOTE_A4, 225}, {NOTE_B4, 225}, {NOTE_A4, 225},
  {NOTE_GS4, 225}, {NOTE_AS4, 225}, {NOTE_GS4, 225},
  {NOTE_G4, 150}, {NOTE_D4, 150}, {NOTE_E4, 150}, {REST, 300}
};

Note marioJump[] = {
  {NOTE_C6, 75}, {NOTE_E6, 75}, {NOTE_G6, 75}, {NOTE_C7, 75}
};

Note zeldaTheme[] = {
  {NOTE_AS4, 200}, {NOTE_F4, 75}, {NOTE_F4, 75}, {NOTE_AS4, 150},
  {NOTE_GS4, 200}, {NOTE_FS4, 200}, {NOTE_GS4, 200},
  {NOTE_AS4, 300}, {NOTE_FS4, 150}, {NOTE_GS4, 150},
  {NOTE_AS4, 200}, {NOTE_FS4, 75}, {NOTE_FS4, 75},
  
  {NOTE_AS4, 150}, {NOTE_A4, 200}, {NOTE_F4, 200},
  {NOTE_AS4, 400}, {REST, 200},
  
  {NOTE_AS4, 200}, {NOTE_F4, 75}, {NOTE_F4, 75}, {NOTE_AS4, 150},
  {NOTE_GS4, 200}, {NOTE_FS4, 200}, {NOTE_GS4, 200},
  {NOTE_AS4, 300}, {NOTE_FS4, 150}, {NOTE_GS4, 150},
  {NOTE_AS4, 200}, {NOTE_FS4, 75}, {NOTE_FS4, 75},
  
  {NOTE_AS4, 150}, {NOTE_CS5, 200}, {NOTE_DS5, 200},
  {NOTE_E5, 200}, {NOTE_FS5, 150}, {NOTE_E5, 150},
  {NOTE_DS5, 200}, {NOTE_CS5, 150}, {NOTE_DS5, 150},
  {NOTE_E5, 200}, {NOTE_CS5, 150}, {NOTE_DS5, 150},
  
  {NOTE_E5, 150}, {NOTE_CS5, 150}, {NOTE_C5, 150}, {NOTE_CS5, 150},
  {NOTE_D5, 200}, {NOTE_E5, 200}, {NOTE_FS5, 150},
  {NOTE_E5, 150}, {NOTE_DS5, 200}, {NOTE_CS5, 150},
  {NOTE_DS5, 150}, {NOTE_E5, 200}, {NOTE_CS5, 150},
  
  {NOTE_DS5, 150}, {NOTE_E5, 150}, {NOTE_CS5, 150},
  {NOTE_C5, 150}, {NOTE_CS5, 150}, {NOTE_D5, 200},
  {NOTE_E5, 200}, {NOTE_FS5, 150}, {NOTE_E5, 150},
  {NOTE_DS5, 200}, {NOTE_CS5, 150}, {NOTE_DS5, 150},
  
  {NOTE_E5, 200}, {NOTE_CS5, 150}, {NOTE_C5, 200},
  {NOTE_CS5, 600}, {REST, 200}
};

Song playlist[] = {
  {"Main Theme", "Super Mario Bros", marioTheme, 38},
  {"Underworld", "Super Mario Bros", marioUnderworld, 43},
  {"Power Up", "Super Mario Bros", marioPowerUp, 14},
  {"Coin Sound", "Super Mario Bros", marioCoin, 3},
  {"Game Over", "Super Mario Bros", marioGameOver, 13},
  {"Jump Sound", "Super Mario Bros", marioJump, 4},
  {"Main Theme", "Legend of Zelda", zeldaTheme, 70}
};

int currentSong = 0;
int totalSongs = 7;
int noteIndex = 0;
unsigned long lastNote = 0;
bool isPlaying = false;
int barHeights[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int targetHeights[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void setup() {
  for (int i = 0; i < ledCount; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
  pinMode(btnNext, INPUT_PULLUP);
  pinMode(btnPrev, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  
  matrix.shutdown(0, false);
  matrix.setIntensity(0, 8);
  matrix.clearDisplay(0);
  
  isPlaying = true;
}

void loop() {
  if (digitalRead(btnNext) == LOW) {
    stopSong();
    currentSong = (currentSong + 1) % totalSongs;
    resetSong();
    delay(300);
  }
  
  if (digitalRead(btnPrev) == LOW) {
    stopSong();
    currentSong = (currentSong - 1 + totalSongs) % totalSongs;
    resetSong();
    delay(300);
  }
  
  if (isPlaying) {
    Song* song = &playlist[currentSong];
    
    if (millis() - lastNote >= song->notes[noteIndex].duration) {
      playCurrentNote();
      noteIndex++;
      
      if (noteIndex >= song->length) {
        noteIndex = 0;
      }
      lastNote = millis();
    }
  }
  
  updateMatrixBars();
}

void playCurrentNote() {
  Song* song = &playlist[currentSong];
  int freq = song->notes[noteIndex].frequency;
  
  if (freq == REST) {
    noTone(buzzerPin);
    updateLEDBar(0);
    for (int i = 0; i < 8; i++) {
      targetHeights[i] = 0;
    }
  } else {
    tone(buzzerPin, freq);
    
    int level = map(freq, 31, 4978, 1, ledCount);
    level = constrain(level, 1, ledCount);
    updateLEDBar(level);
    
    updateMatrixTargets(freq, level);
  }
}

void updateMatrixTargets(int freq, int level) {
  int baseHeight = map(level, 0, ledCount, 0, 8);
  
  for (int i = 0; i < 8; i++) {
    int variance = random(-2, 3);
    targetHeights[i] = constrain(baseHeight + variance, 0, 8);
  }
  
  int harmonic1 = map(freq % 500, 0, 500, 0, 8);
  int harmonic2 = map((freq * 2) % 800, 0, 800, 0, 8);
  
  targetHeights[1] = constrain(targetHeights[1] + harmonic1 / 2, 0, 8);
  targetHeights[4] = constrain(targetHeights[4] + harmonic2 / 2, 0, 8);
  targetHeights[6] = constrain(targetHeights[6] + harmonic1 / 3, 0, 8);
}

void updateMatrixBars() {
  matrix.clearDisplay(0);
  
  for (int col = 0; col < 8; col++) {
    if (barHeights[col] < targetHeights[col]) {
      barHeights[col]++;
    } else if (barHeights[col] > targetHeights[col]) {
      barHeights[col]--;
    }
    
    for (int row = 0; row < 8; row++) {
      if (row < barHeights[col]) {
        matrix.setLed(0, 7 - row, col, true);
      }
    }
  }
}

void updateLEDBar(int level) {
  for (int i = 0; i < ledCount; i++) {
    if (i < level) {
      digitalWrite(ledPins[i], HIGH);
    } else {
      digitalWrite(ledPins[i], LOW);
    }
  }
}

void stopSong() {
  noTone(buzzerPin);
  for (int i = 0; i < ledCount; i++) {
    digitalWrite(ledPins[i], LOW);
  }
  for (int i = 0; i < 8; i++) {
    targetHeights[i] = 0;
    barHeights[i] = 0;
  }
  matrix.clearDisplay(0);
}

void resetSong() {
  noteIndex = 0;
  matrix.clearDisplay(0);
  isPlaying = true;
  for (int i = 0; i < 8; i++) {
    targetHeights[i] = 0;
    barHeights[i] = 0;
  }
}
