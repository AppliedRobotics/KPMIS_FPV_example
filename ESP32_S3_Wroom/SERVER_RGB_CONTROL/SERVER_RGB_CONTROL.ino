#include "esp_http_server.h"   // Библиотека HTTP-сервера ESP-IDF
#include <WiFi.h>              // Библиотека WiFi для ESP32
#include <Adafruit_NeoPixel.h> // Библиотека для управления светодиодами NeoPixel

// Данные для точки доступа WiFi
const char* ssid = "KPMIS_CAR";
const char* password = "01234567";

#define PIN 48        // GPIO пин для подключения линии данных NeoPixel
#define NUMPIXELS 1   // Количество светодиодов NeoPixel в ленте

// Создание объекта ленты NeoPixel с указанным пином и количеством светодиодов
Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Хэндл экземпляра HTTP-сервера
httpd_handle_t stream_httpd = NULL;

// Переменные для хранения текущих значений RGB (0-255)
uint8_t red_val = 0;
uint8_t green_val = 0;
uint8_t blue_val = 0;

// Функция обновления цвета NeoPixel на основе текущих значений RGB
void updateColor() {
  // Установка цвета первого (и единственного) пикселя
  strip.setPixelColor(0, strip.Color(red_val, green_val, blue_val));
  // Отправка обновленных данных цвета на ленту светодиодов
  strip.show();
}

// Обработчик HTTP GET для корневой страницы "/"
// Отдает HTML-страницу с ползунками и кнопкой для управления RGB значениями
static esp_err_t index_handler(httpd_req_t *req) {
  // Формирование HTML-страницы в виде строки
  String html = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<title>Applied Robotics RGB Control</title>"
                "<style>"
                "body { text-align: center; font-family: Arial, sans-serif; margin: 50px; }"
                ".btn { padding: 20px 40px; font-size: 24px; margin: 10px; cursor: pointer; "
                "background-color: #4CAF50; color: white; border: none; border-radius: 10px; }"
                ".btn:hover { background-color: #45a049; }"
                ".slider { width: 300px; }"
                "</style>"
                "<script>"
                "function sendColor() {"
                "  const r = document.getElementById('red').value;"
                "  const g = document.getElementById('green').value;"
                "  const b = document.getElementById('blue').value;"
                "  fetch(`/set_color?r=${r}&g=${g}&b=${b}`)"
                "    .then(response => response.text())"
                "    .then(data => console.log(data));"
                "}"
                "</script>"
                "</head>"
                "<body>"
                "<h1>Applied Robotics RGB Control</h1>"
                "<button class='btn' onclick='sendColor()'>Set Color</button><br><br>"
                "<label for='red'>Red: <span id='r_val'>0</span></label><br>"
                "<input type='range' min='0' max='255' value='0' class='slider' id='red' oninput='document.getElementById(\"r_val\").innerText = this.value'><br>"
                "<label for='green'>Green: <span id='g_val'>0</span></label><br>"
                "<input type='range' min='0' max='255' value='0' class='slider' id='green' oninput='document.getElementById(\"g_val\").innerText = this.value'><br>"
                "<label for='blue'>Blue: <span id='b_val'>0</span></label><br>"
                "<input type='range' min='0' max='255' value='0' class='slider' id='blue' oninput='document.getElementById(\"b_val\").innerText = this.value'><br>"
                "</body>"
                "</html>";

  // Отправка HTML-страницы клиенту
  httpd_resp_send(req, html.c_str(), html.length());
  return ESP_OK;
}

// Обработчик HTTP GET для URI "/set_color"
// Парсит RGB значения из параметров URL запроса и обновляет цвет светодиода
static esp_err_t set_color_handler(httpd_req_t *req) {
  char buf[100]; // Буфер для хранения строки запроса URL
  size_t buf_len = httpd_req_get_url_query_len(req) + 1; // Длина запроса + нулевой терминатор
  if (buf_len > sizeof(buf)) buf_len = sizeof(buf); // Ограничение размера буфера

  // Получение строки запроса URL (например "r=255&g=100&b=50")
  if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
    char val_str[10]; // Временный буфер для хранения каждого значения параметра в виде строки

    // Извлечение параметра 'r' и преобразование в целое число
    if (httpd_query_key_value(buf, "r", val_str, sizeof(val_str)) == ESP_OK) {
      red_val = atoi(val_str);
    }
    // Извлечение параметра 'g' и преобразование в целое число
    if (httpd_query_key_value(buf, "g", val_str, sizeof(val_str)) == ESP_OK) {
      green_val = atoi(val_str);
    }
    // Извлечение параметра 'b' и преобразование в целое число
    if (httpd_query_key_value(buf, "b", val_str, sizeof(val_str)) == ESP_OK) {
      blue_val = atoi(val_str);
    }
    // Обновление цвета светодиода с новыми значениями
    updateColor();
  }
  // Ответ простым сообщением "OK"
  httpd_resp_send(req, "OK", 2);
  return ESP_OK;
}

// Функция запуска HTTP-сервера и регистрации обработчиков URI
void startServer() {
  // Использование конфигурации HTTP-сервера по умолчанию
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 16; // Разрешить до 16 обработчиков URI

  // Определение обработчика URI для корневой страницы "/"
  httpd_uri_t index_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_handler,
    .user_ctx = NULL
  };

  // Определение обработчика URI для установки цвета "/set_color"
  httpd_uri_t set_color_uri = {
    .uri = "/set_color",
    .method = HTTP_GET,
    .handler = set_color_handler,
    .user_ctx = NULL
  };

  // Запуск HTTP-сервера
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    // Регистрация обработчиков URI
    httpd_register_uri_handler(stream_httpd, &index_uri);
    httpd_register_uri_handler(stream_httpd, &set_color_uri);
  }
}

void setup() {
  Serial.begin(115200); // Инициализация последовательной связи для отладки

  strip.begin(); // Инициализация ленты NeoPixel
  strip.show();  // Изначальное выключение всех светодиодов

  // Запуск точки доступа WiFi с указанным SSID и паролем
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP()); // Вывод IP-адреса точки доступа

  startServer(); // Запуск HTTP-сервера
}

void loop() {
  // Основной цикл ничего не делает, сервер работает асинхронно в фоновом режиме
}
