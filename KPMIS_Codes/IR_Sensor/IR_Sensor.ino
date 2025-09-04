#include <IRremote.h>  //подключаем библиотеку для работу с ИК датчиком

const int IR_RECEIVE_PIN = 3; // пин D3
IRrecv irrecv(IR_RECEIVE_PIN); // Инициализация пина на котором установлен ИК датчик
decode_results results; //инициализация переменной хранящую значения ИК датчика

// Функция setup() выполняется один раз при запуске Arduino
void setup() {
  Serial.begin(9600);  // Инициализация последовательного порта
  irrecv.enableIRIn(); // запуск приёма IR
}


// Функция loop() выполняется бесконечно после setup()
void loop() {
  if (irrecv.decode(&results)) { //проверка инициализации приёма ИК датчиком
    unsigned long receivedCode = results.value; //переменная хранящая данные вывода
    // Выводим код в HEX формате
    Serial.print("Код кнопки: 0x"); // указатель на вывод
    Serial.println(receivedCode, HEX); //выводим входящие Шестнацетиричные значения
    irrecv.resume(); // подготовка к следующему приему
  }
}