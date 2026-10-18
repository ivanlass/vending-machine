#include <LiquidCrystal.h>

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(22, 21, 18, 19, 23, 5);

const int coinPin = 27;
const uint32_t minPulseIntervalUs = 20000;
const uint32_t coinQuietTimeUs = 700000;

volatile uint16_t pendingPulseCount = 0;
volatile uint32_t lastPulseTime = 0;
volatile bool hasSeenPulse = false;

void IRAM_ATTR coinPulse() {
  const uint32_t now = micros();

  if (hasSeenPulse &&
      static_cast<uint32_t>(now - lastPulseTime) < minPulseIntervalUs) {
    return;
  }

  if (pendingPulseCount < UINT16_MAX) {
    pendingPulseCount++;
  }
  lastPulseTime = now;
  hasSeenPulse = true;
}

void printAmount(uint32_t amountCents) {
  lcd.print(amountCents / 100);
  lcd.print('.');
  if (amountCents % 100 < 10) {
    lcd.print('0');
  }
  lcd.print(amountCents % 100);
  lcd.print(" KM");
}

void printSerialAmount(uint32_t amountCents) {
  Serial.print(amountCents / 100);
  Serial.print('.');
  if (amountCents % 100 < 10) {
    Serial.print('0');
  }
  Serial.print(amountCents % 100);
  Serial.print(" KM");
}

void showCoin(uint32_t coinCents, uint32_t totalCents) {
  lcd.setCursor(0, 0);
  lcd.print("Credit:         ");
  lcd.setCursor(8, 0);
  printAmount(totalCents);

  lcd.setCursor(0, 1);
  lcd.print("Coin:           ");
  lcd.setCursor(6, 1);
  printAmount(coinCents);
}

void setup() {
  Serial.begin(115200);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Credit: 0.00 KM");
  lcd.setCursor(0, 1);
  lcd.print("Insert coin");

  // Signal mora imati odgovarajuci pull-up
  pinMode(coinPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(coinPin), coinPulse, FALLING);
}

void loop() {
  static uint32_t totalCreditCents = 0;
  static uint16_t displayedPulseCount = 0;
  uint16_t coinPulses = 0;
  uint16_t currentPulseCount = 0;

  noInterrupts();
  const uint32_t now = micros();
  currentPulseCount = pendingPulseCount;
  if (pendingPulseCount > 0 &&
      static_cast<uint32_t>(now - lastPulseTime) >= coinQuietTimeUs) {
    coinPulses = pendingPulseCount;
    pendingPulseCount = 0;
    currentPulseCount = 0;
  }
  interrupts();

  if (currentPulseCount > 0 && currentPulseCount != displayedPulseCount) {
    displayedPulseCount = currentPulseCount;
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("Pulses: ");
    lcd.print(currentPulseCount);
  }

  if (coinPulses > 0) {
    displayedPulseCount = 0;
    uint32_t coinCents = 0;
    switch (coinPulses) {
      case 1: coinCents = 500; break;
      case 2: coinCents = 200; break;
      case 3: coinCents = 100; break;
      case 4: coinCents = 50; break;
      default: break;
    }

    if (coinCents != 0) {
      totalCreditCents += coinCents;
      showCoin(coinCents, totalCreditCents);
      Serial.print("Coin pulses: ");
      Serial.print(coinPulses);
      Serial.print(", accepted: ");
      printSerialAmount(coinCents);
      Serial.print(", total: ");
      printSerialAmount(totalCreditCents);
      Serial.println();
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Unknown pulses  ");
      Serial.print("Unknown coin pulse count: ");
      Serial.println(coinPulses);
    }
  }

  delay(20);
}
