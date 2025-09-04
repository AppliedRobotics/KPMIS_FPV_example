#include <Adafruit_NeoPixel.h>

#define PIN 48        // Пин подключения данных
#define NUMPIXELS 1   // Количество диодов

Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();           // Инициализация NeoPixel
  strip.show();            // Обновление для выключения всех светодиодов
}

void loop() {
  // Красный
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();
    delay(1000);

  // Зеленый
  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
    delay(1000);

  // Синий
  strip.setPixelColor(0, strip.Color(0, 0, 255));
  strip.show();
    delay(1000);

  // Белый (мигание)
  strip.setPixelColor(0, strip.Color(255,255,255));
  strip.show();
    delay(500);
  
  // Выключить
  strip.setPixelColor(0, strip.Color(0,0,0));
  strip.show();
    delay(500);
}