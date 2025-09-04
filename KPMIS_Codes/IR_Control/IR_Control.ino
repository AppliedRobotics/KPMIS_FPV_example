#include <IRremote.h> //подключаем библиотеку для работу с ИК датчиком

// Определяем пин для ИК-приемника
const int irPin = 3;

// Создаем объект для приема ИК-сигналов
IRrecv irrecv(irPin);
decode_results results;
// Определение пинов для управления
#define PWM1_PIN            5   // Пин для первого PWM канала
#define PWM2_PIN            6   // Пин для второго PWM канала

// Определение пинов для сдвигового регистра 74HCT595N
#define SHCP_PIN            2   // Пин для тактового сигнала (Shift Clock)
#define EN_PIN              7   // Пин для включения/выключения (Enable)
#define DATA_PIN            8   // Пин для последовательных данных (Data)
#define STCP_PIN            4   // Пин для хранения данных (Storage Clock)

// Определение констант для управления двигателями
const int Forward       = 92;   // Значение для движения вперед
const int Backward      = 163;  // Значение для движения назад
const int Turn_Left     = 149;  // Значение для поворота налево
const int Turn_Right    = 106;  // Значение для поворота направо
const int Top_Left      = 20;   // Значение для движения вверх-влево
const int Bottom_Left   = 129;  // Значение для движения вниз-влево
const int Top_Right     = 72;   // Значение для движения вверх-вправо
const int Bottom_Right  = 34;   // Значение для движения вниз-вправо
const int Stop          = 0;    // Значение для остановки
const int Contrarotate  = 20;   // Значение для вращения против часовой стрелки
const int Clockwise     = 72;   // Значение для вращения по часовой стрелке

// Функция setup() выполняется один раз при запуске Arduino
void setup()
{
    irrecv.enableIRIn(); // Включение приема
    // Установка режимов работы пинов
    pinMode(SHCP_PIN, OUTPUT);  // Установка пина для тактового сигнала как выход
    pinMode(EN_PIN, OUTPUT);     // Установка пина включения как выход
    pinMode(DATA_PIN, OUTPUT);   // Установка пина данных как выход
    pinMode(STCP_PIN, OUTPUT);    // Установка пина хранения как выход
    pinMode(PWM1_PIN, OUTPUT);   // Установка первого PWM пина как выход
    pinMode(PWM2_PIN, OUTPUT);   // Установка второго PWM пина как выход
}

unsigned long rezalt=0;

// Функция loop() выполняется бесконечно после setup()
void loop()
{
  
  if (irrecv.decode(&results)) {
    rezalt = results.value, HEX;
    if(rezalt == 0xFF18E7 ){ //полученные ИК датчиком значения сравниваем со значением определяюещее направление движения
      Motor(Forward, 250);   //передаём в функцию скорость и направление вращения
      delay(2000);           //задержка 2 секунды - необходима для корректной работы моторов
    }
    if(rezalt == 0xFF4AB5){ //полученные ИК датчиком значения сравниваем со значением определяюещее направление движения
      Motor(Backward, 250); //передаём в функцию скорость и направление вращения     
      delay(2000);          //задержка 2 секунды - необходима для корректной работы моторов
    }
    if(rezalt == 0xFF10EF){  //полученные ИК датчиком значения сравниваем со значением определяюещее направление движения 
      Motor(Clockwise, 250); //передаём в функцию скорость и направление вращения     
      delay(2000);           //задержка 2 секунды - необходима для корректной работы моторов
    }
    if(rezalt == 0xFF5AA5){     //полученные ИК датчиком значения сравниваем со значением определяюещее направление движения
      Motor(Contrarotate, 250); //передаём в функцию скорость и направление вращения  
      delay(2000);              //задержка 2 секунды - необходима для корректной работы драйверов моторов
    }
  /* Stop */
  Motor(Stop, 250); //если значений нет или уже были исполнены - Остановить моторы
  delay(20);        // задержка 20 миллисекунд - для корректной работы драйверов моторов
  irrecv.resume();  // Готовимся к следующему приему
  }
  rezalt=0;         // Онулируем значение, если не происходит обмена по ИК датчику

  //все команды управления
    // /* Forward */
    // Motor(Forward, 250);     
    // delay(2000);
    // /* Backward */
    // Motor(Backward, 250);
    // delay(2000);
    // /* Turn_Left */
    // Motor(Turn_Left, 250);
    // delay(2000);
    // /* Turn_Right */
    // Motor(Turn_Right, 250);
    // delay(2000);
    // /* Top_Left */
    // Motor(Top_Left, 250);
    // delay(2000);
    // /* Bottom_Right */
    // Motor(Bottom_Right, 250);
    // delay(2000);
    // /* Bottom_Left */
    // Motor(Bottom_Left, 250);
    // delay(2000);
    // /* Top_Right */
    // Motor(Top_Right, 250);
    // delay(2000);
    // /* Clockwise */
    // Motor(Clockwise, 250);
    // delay(2000);
    // /* Contrarotate */
    // Motor(Contrarotate, 250);
    // delay(2000);
    // /* Stop */
    // Motor(Stop, 250);
    // delay(2000);
}

//функция управления моторами
void Motor(int Dir, int Speed)
{
    digitalWrite(EN_PIN, LOW);
    analogWrite(PWM1_PIN, Speed);
    analogWrite(PWM2_PIN, Speed);

    digitalWrite(STCP_PIN, LOW);
    shiftOut(DATA_PIN, SHCP_PIN, MSBFIRST, Dir);
    digitalWrite(STCP_PIN, HIGH);
}