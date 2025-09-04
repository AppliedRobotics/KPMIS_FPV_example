// Определяем пины для управления ШИМ
#define PWM1_PIN            5 // Пин для первого ШИМ
#define PWM2_PIN            6 // Пин для второго ШИМ      

// Пины для чипа 74HCT595N
#define SHCP_PIN            2 // Пин для сдвига тактового сигнала
#define EN_PIN              7 // Пин для управления включением
#define DATA_PIN            8 // Пин для передачи последовательных данных
#define STCP_PIN            4 // Пин для тактового сигнала регистра памяти        

unsigned long previousMillis = 0; // Хранит время последнего обновления
const long interval = 20; // Интервал в миллисекундах для обновления

// Константы для управления движением
const int Forward       = 92; // Вперед
const int Backward      = 163; // Назад
const int Turn_Left     = 149; // Поворот влево
const int Turn_Right    = 106; // Поворот вправо 
const int Top_Left      = 20; // Верхний левый
const int Bottom_Left   = 129; // Нижний левый
const int Top_Right     = 72; // Верхний правый
const int Bottom_Right  = 34; // Нижний правый
const int Stop          = 0; // Стоп
const int Contrarotate  = 20; // Против часовой стрелки
const int Clockwise     = 72; // По часовой стрелке

// Определяем переменные для хранения значений
int j1_X = 50; // Координаты первого джойстика по X
int j1_Y = 50; // Координаты первого джойстика по Y
int j2_X = 50; // Координаты второго джойстика по X
int j2_Y = 50; // Координаты второго джойстика по Y
int buttons[8] = {0}; // Массив для хранения состояния кнопок b0 - b7
int slider1 = 90; // Значение первого слайдера
int slider2 = 90; // Значение второго слайдера
int pos1 = 90; // Позиция первого сервомотора
int pos2 = 90; // Позиция второго сервомотора

#include <Servo.h> // Подключаем библиотеку для управления сервомоторами

Servo servo1; // Создаем объект для первого сервомотора
Servo servo2; // Создаем объект для второго сервомотора

void setup() {
  // Serial.begin(9600); // Инициализация сериал-коммуникации для Bluetooth
  Serial.begin(115200);  // Инициализация сериал-коммуникации для WiFi 
  
  // Устанавливаем режим работы пинов
  pinMode(SHCP_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(STCP_PIN, OUTPUT);
  pinMode(PWM1_PIN, OUTPUT);
  pinMode(PWM2_PIN, OUTPUT);

  // Подключаем сервомоторы к соответствующим пинам
  servo1.attach(9); // Подключаем первый сервомотор к пину 9
  servo2.attach(10); // Подключаем второй сервомотор к пину 10
}

void loop() {
  unsigned long currentMillis = millis(); // Получаем текущее время
  if (Serial.available()) { // Проверяем, есть ли доступные данные в последовательном порту
    String input = Serial.readStringUntil('\n'); // Читаем строку до конца строки
    parseInput(input); // Парсим входные данные
    MotorControl(); // Управляем моторами на основе парсенных данных
    delay(20); // Задержка
  }
  
  // Проверка интервала для обновления
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Сохраняем текущее время
    Ser(); // Обновляем позиции сервомоторов
    delay(20); // Задержка
  }
}

void MotorControl() {
  // Управление движением на основе значений джойстика
  if (j1_X < 65 && j1_X > 35) {
    if (j1_Y > 55) {
      /* Вперед */
      Motor(Forward, map(j1_Y, 55, 100, 50, 250));     
    }
    if (j1_Y < 45) {
      /* Назад */
      Motor(Backward, map(j1_Y, 45, 0, 50, 250));
    }
    if (j1_Y > 45 && j1_Y < 55) {
      /* Стоп */
      Motor(Stop, 250);
    }
  } else {
    if (j1_X > 65) {
      /* Вперед */
      Motor(Turn_Left, map(j1_X, 65, 100, 100, 250));     
    }
    if (j1_X < 35) {
      /* Назад */
      Motor(Turn_Right, map(j1_X, 35, 0, 100, 250));
    }
  }
}

void Motor(int Dir, int Speed) {
    digitalWrite(EN_PIN, LOW); // Включаем управление
    analogWrite(PWM1_PIN, Speed); // Устанавливаем скорость для первого мотора
    analogWrite(PWM2_PIN, Speed); // Устанавливаем скорость для второго мотора

    digitalWrite(STCP_PIN, LOW); // Подготавливаем сдвиг
    shiftOut(DATA_PIN, SHCP_PIN, MSBFIRST, Dir); // Отправляем направление
    digitalWrite(STCP_PIN, HIGH); // Завершаем сдвиг
}

void Ser() {
    // Обновляем позиции сервомоторов на основе значений слайдеров
    if (pos1 < slider1) {
      pos1++; // Увеличиваем позицию первого сервомотора
      servo1.write(180 - pos1); // Устанавливаем угол
    }
    if (pos1 > slider1) {
      pos1--; // Уменьшаем позицию первого сервомотора
      servo1.write(180 - pos1); // Устанавливаем угол
    }

    if (pos2 < slider2) {
      pos2++; // Увеличиваем позицию второго сервомотора
      servo2.write(180 - pos2); // Устанавливаем угол
    }
    if (pos2 > slider2) {
      pos2--; // Уменьшаем позицию второго сервомотора
      servo2.write(180 - pos2); // Устанавливаем угол
    }
}

void parseInput(String input) {
  // Разделяем строку на части
  int separatorIndex1 = input.indexOf(':'); // Находим первый разделитель
  int separatorIndex2 = input.indexOf(':', separatorIndex1 + 1); // Находим второй разделитель
  
  if (separatorIndex1 > 0) {
    String key = input.substring(0, separatorIndex1); // Получаем ключ
    String value1 = input.substring(separatorIndex1 + 1, separatorIndex2); // Получаем первое значение
    String value2 = input.substring(separatorIndex2 + 1); // Получаем второе значение

    // Парсим значения в зависимости от ключа
    if (key == "j1") {
      j1_X = value1.toInt(); // Присваиваем значения для первого джойстика
      j1_Y = value2.toInt();
    } else if (key == "j2") {
      j2_X = value1.toInt(); // Присваиваем значения для второго джойстика
      j2_Y = value2.toInt();
    } else if (key.startsWith("b")) {
      int buttonIndex = key.charAt(1) - '0'; // Получаем индекс кнопки
      if (buttonIndex >= 0 && buttonIndex < 8) {
        buttons[buttonIndex] = value1.toInt(); // Присваиваем значение кнопки
      }
    } else if (key == "s1") {
      slider1 = value1.toInt(); // Присваиваем значение для первого слайдера
    } else if (key == "s2") {
      slider2 = value1.toInt(); // Присваиваем значение для второго слайдера
    }
  }

  // Выводим результаты для проверки (закомментировано)
  // Serial.print("Joystick 1 - X: ");
  // Serial.print(j1_X);
  // Serial.print(", Y: ");
  // Serial.println(j1_Y);
  
  // Serial.print("Joystick 2 - X: ");
  // Serial.print(j2_X);
  // Serial.print(", Y: ");
  // Serial.println(j2_Y);
  
  // Serial.print("Buttons: ");
  // for (int i = 0; i < 8; i++) {
  //   Serial.print("b");
  //   Serial.print(i);
  //   Serial.print(": ");
  //   Serial.print(buttons[i]);
  //   Serial.print(" ");
  // }
  // Serial.println();

  // Serial.print("Slider 1: ");
  // Serial.print(slider1);
  // Serial.print(", Slider 2: ");
  // Serial.println(slider2);
}
