void setup() {
  Serial.begin(9600); // Инициализация сериал-коммуникации для Bluetooth

}

void loop() {
   if (Serial.available()) { // Проверяем, есть ли доступные данные в последовательном порту
    String input = Serial.readStringUntil('\n'); // Читаем строку до конца строки
    Serial.println(input);
  }
}