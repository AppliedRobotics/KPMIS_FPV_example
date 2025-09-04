/*
 * Методическое пособие: Управление двигателями с помощью Arduino и 74HCT595N
 * Данный код демонстрирует управление двигателями с использованием ШИМ и сдвигового регистра
 */

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
    // Установка режимов работы пинов
    pinMode(SHCP_PIN, OUTPUT);  // Установка пина для тактового сигнала как выход
    pinMode(EN_PIN, OUTPUT);     // Установка пина включения как выход
    pinMode(DATA_PIN, OUTPUT);   // Установка пина данных как выход
    pinMode(STCP_PIN, OUTPUT);    // Установка пина хранения как выход
    pinMode(PWM1_PIN, OUTPUT);   // Установка первого PWM пина как выход
    pinMode(PWM2_PIN, OUTPUT);   // Установка второго PWM пина как выход
}

// Функция loop() выполняется бесконечно после setup()
void loop()
{
    // Последовательное выполнение команд для управления двигателями
    Motor(Forward, 250);     // Движение вперед
    delay(2000);             // Задержка 2 секунды
    Motor(Backward, 250);    // Движение назад
    delay(2000);
    Motor(Turn_Left, 250);   // Поворот налево
    delay(2000);
    Motor(Turn_Right, 250);  // Поворот направо
    delay(2000);
    Motor(Top_Left, 250);    // Движение вверх-влево
    delay(2000);
    Motor(Bottom_Right, 250); // Движение вниз-вправо
    delay(2000);
    Motor(Bottom_Left, 250);  // Движение вниз-влево
    delay(2000);
    Motor(Top_Right, 250);    // Движение вверх-вправо
    delay(2000);
    Motor(Clockwise, 250);     // Вращение по часовой стрелке
    delay(2000);
    Motor(Contrarotate, 250);  // Вращение против часовой стрелки
    delay(2000);
    Motor(Stop, 250);          // Остановка
    delay(2000);
}

// Функция Motor управляет двигателями
void Motor(int Dir, int Speed)
{
    digitalWrite(EN_PIN, LOW); // Включение управления двигателями
    analogWrite(PWM1_PIN, Speed); // Установка скорости для первого двигателя
    analogWrite(PWM2_PIN, Speed); // Установка скорости для второго двигателя

    digitalWrite(STCP_PIN, LOW); // Подготовка к записи данных
    shiftOut(DATA_PIN, SHCP_PIN, MSBFIRST, Dir); // Передача направления в сдвиговый регистр
    digitalWrite(STCP_PIN, HIGH); // Сохранение данных
}
