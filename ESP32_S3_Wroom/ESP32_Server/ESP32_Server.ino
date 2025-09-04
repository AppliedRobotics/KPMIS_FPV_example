#include <WiFi.h>

const char* ssid = "ESP32_S3_AP"; // имя точки доступа
const char* password = "12345678"; // пароль точки доступа

WiFiServer server(80); // TCP сервер на порту 80

// Максимальное число клиентов
#define MAX_CLIENTS 20

// Структура для хранения клиента и его IP
struct ClientInfo {
  WiFiClient client;
  IPAddress ip;
  bool active; // активен ли клиент
};

ClientInfo clients[MAX_CLIENTS];

#define LED_PIN 2

void setup() {
  Serial.begin(115200);
  
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  
  server.begin();
  
  pinMode(LED_PIN, OUTPUT);
  
  // Инициализация массива
  for (int i=0; i<MAX_CLIENTS; i++) {
    clients[i].active = false;
    // client по умолчанию не активен
  }
}

void loop() {
  // Проверка новых подключений
  WiFiClient newClient = server.available();
  if (newClient) {
    IPAddress newIP = newClient.remoteIP();
    bool found = false;

    // Проверяем есть ли уже такой клиент по IP
    for (int i=0; i<MAX_CLIENTS; i++) {
      if (clients[i].active && clients[i].ip == newIP) {
        // Обновляем существующее соединение
        clients[i].client.stop(); // закрываем старое соединение
        clients[i].client = newClient;
        Serial.print("Обновление клиента с IP: ");
        Serial.println(newIP);
        found = true;
        break;
      }
    }

    if (!found) {
      // Добавляем нового клиента в свободную ячейку
      bool added = false;
      for (int i=0; i<MAX_CLIENTS; i++) {
        if (!clients[i].active) {
          clients[i].client = newClient;
          clients[i].ip = newIP;
          clients[i].active = true;
          Serial.print("Новый клиент с IP: ");
          Serial.println(newIP);
          added = true;
          break;
        }
      }
      if (!added) {
        // Переполнение — закрываем соединение
        newClient.stop();
        Serial.println("Максимальное число клиентов достигнуто");
      }
    }
  }

  int activeClientsCount = 0;

  // Обработка существующих клиентов и проверка их статуса
  for (int i=0; i<MAX_CLIENTS; i++) {
    if (clients[i].active) {
      WiFiClient &c = clients[i].client;

      if (!c.connected()) {
        // Клиент отключился — очищаем запись
        Serial.print("Клиент отключился с IP: ");
        Serial.println(clients[i].ip);
        c.stop();
        clients[i].active = false;
      } else {
        // Есть активное соединение — можно читать или писать данные
        if (c.available()) {
          String msg = c.readStringUntil('\n');
          Serial.print("Получено от ");
          Serial.print(clients[i].ip);
          Serial.print(": ");
          Serial.println(msg);
          
          // Можно отправить ответ или обработать сообщение
          c.println("Сообщение получено");
        }
        activeClientsCount++;
      }
    }
  }

  // Мигаем светодиод только если есть активные клиенты
  if (activeClientsCount > 0) {
    blinkLED(activeClientsCount);
    delay(2000); // пауза между миганиями
  } else {
    digitalWrite(LED_PIN, LOW); // светодиод не мигает, выключен
    delay(1000); // чуть дольше ждем без активности
  }
}

// Функция мигания LED N раз
void blinkLED(int times) {
  for (int i=0; i<times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }
}