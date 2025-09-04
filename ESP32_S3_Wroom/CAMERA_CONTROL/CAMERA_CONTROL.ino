#include "esp_http_server.h" // Библиотека для работы с HTTP сервером
#include "esp_timer.h" // Библиотека для работы с таймерами
#include "img_converters.h" // Библиотека для конвертации изображений
#include "fb_gfx.h" // Библиотека для работы с графикой
#include "esp32-hal-ledc.h" // Библиотека для управления LED
#include "sdkconfig.h" // Конфигурация SDK
#include "esp_camera.h" // Библиотека для работы с камерой ESP32

#include <WiFi.h> // Библиотека для работы с WiFi
#include <WebServer.h> // Библиотека для работы с веб-сервером
#include <WebSocketsServer.h> // Библиотека для работы с WebSocket сервером

#define PART_BOUNDARY "123456789000000000000987654321" // Определение границы для потоковой передачи
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY; // Тип контента для потоковой передачи
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n"; // Граница потока
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n"; // Заголовок для части потока
httpd_handle_t stream_httpd = NULL; // Переменная для хранения обработчика HTTP сервера

const char* ssid = "KPMIS_CAR"; // SSID точки доступа WiFi
const char* password = "01234567"; // Пароль точки доступа WiFi

// Обработчик для потока видео
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL; // Указатель на буфер кадра, получаемого с камеры
  struct timeval _timestamp; // Структура для хранения временной метки кадра
  esp_err_t res = ESP_OK; // Переменная для хранения результата выполнения операций
  size_t _jpg_buf_len = 0; // Длина буфера JPEG
  uint8_t *_jpg_buf = NULL; // Указатель на буфер JPEG
  char *part_buf[128]; // Буфер для формирования части ответа

  static int64_t last_frame = 0; // Переменная для хранения времени последнего кадра
  if (!last_frame) {
    last_frame = esp_timer_get_time(); // Инициализация времени последнего кадра
  }

  // Установка типа ответа для HTTP запроса
  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res; // Если установка типа ответа не удалась, возвращаем ошибку
  }

  // Установка заголовков для CORS и частоты кадров
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "X-Framerate", "60");

  while (true) { // Бесконечный цикл для отправки кадров
    fb = esp_camera_fb_get(); // Получение кадра с камеры
    if (!fb) {
      log_e("Camera capture failed"); // Логируем ошибку, если захват кадра не удался
      res = ESP_FAIL; // Устанавливаем результат как ошибку
    } else {
      _timestamp.tv_sec = fb->timestamp.tv_sec; // Получаем секунды временной метки
      _timestamp.tv_usec = fb->timestamp.tv_usec; // Получаем микросекунды временной метки
      if (fb->format != PIXFORMAT_JPEG) { // Если формат кадра не JPEG
        // Конвертация кадра в JPEG формат
        bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        esp_camera_fb_return(fb); // Возврат буфера кадра
        fb = NULL; // Обнуляем указатель на кадр
        if (!jpeg_converted) {
          log_e("JPEG compression failed"); // Логируем ошибку сжатия JPEG
          res = ESP_FAIL; // Устанавливаем результат как ошибку
        }
      } else {
        _jpg_buf_len = fb->len; // Длина JPEG буфера
        _jpg_buf = fb->buf; // Указатель на буфер JPEG
      }
    }
    if (res == ESP_OK) {
      // Отправка границы потока
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (res == ESP_OK) {
      // Форматирование заголовка для части потока
      size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen); // Отправка заголовка
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len); // Отправка JPEG изображения
    }
    if (fb) {
      esp_camera_fb_return(fb); // Возврат буфера, если он существует
      fb = NULL; // Обнуляем указатель на кадр
      _jpg_buf = NULL; // Обнуляем указатель на JPEG буфер
    } else if (_jpg_buf) {
      free(_jpg_buf); // Освобождение памяти, если буфер JPEG существует
      _jpg_buf = NULL; // Обнуляем указатель на JPEG буфер
    }
    if (res != ESP_OK) {
      log_e("Send frame failed"); // Логируем ошибку при отправке кадра
      break; // Выход из цикла в случае ошибки
    }
    int64_t fr_end = esp_timer_get_time(); // Получаем время окончания кадра

    int64_t frame_time = fr_end - last_frame; // Вычисляем время, затраченное на обработку кадра
    last_frame = fr_end; // Обновляем время последнего кадра

    frame_time /= 1000; // Переводим время в миллисекунды
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
    uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time); // Вычисление среднего времени кадра
#endif
    log_i(
      "MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps)", (uint32_t)(_jpg_buf_len), (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time, avg_frame_time,
      1000.0 / avg_frame_time
    ); // Логируем информацию о кадре
  }

  return res; // Возврат результата
}

// Обработчик для главной страницы
static esp_err_t index_handler(httpd_req_t *req) {
  // HTML код для отображения страницы с видеопотоком
  String html = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<title>Camera Stream</title>"
                "<style>"
                "body { text-align: center; position: relative; overflow: hidden; height: 100vh; }"
                "</style>"
                "</head>"
                "<body>"
                "<img src=\"/stream\" style=\"width: 100%; height: auto;\" />" // Изображение с потоком
                "</body>"
                "</html>";

  httpd_resp_send(req, html.c_str(), html.length()); // Отправка HTML страницы клиенту
  return ESP_OK; // Возврат результата
}

// Функция для запуска сервера камеры
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG(); // Создание конфигурации сервера с настройками по умолчанию
  config.max_uri_handlers = 16; // Установка максимального количества обработчиков URI

  // Регистрация обработчиков для главной страницы и потока
  httpd_uri_t index_uri = {
    .uri       = "/", // URI для главной страницы
    .method    = HTTP_GET, // Метод HTTP
    .handler   = index_handler, // Обработчик для главной страницы
    .user_ctx  = NULL // Дополнительный контекст (не используется)
  };

  httpd_uri_t stream_uri = {
    .uri       = "/stream", // URI для потока
    .method    = HTTP_GET, // Метод HTTP
    .handler   = stream_handler, // Обработчик для потока
    .user_ctx  = NULL // Дополнительный контекст (не используется)
  };

  // Запуск HTTP сервера
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &index_uri); // Регистрация обработчика главной страницы
    httpd_register_uri_handler(stream_httpd, &stream_uri); // Регистрация обработчика потока
  }
}

void setupLedFlash(); // Прототип функции для настройки мигания LED

WebSocketsServer webSocket = WebSocketsServer(81); // Создание WebSocket сервера на порту 81

// Обработчик событий WebSocket
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) { // Если получено текстовое сообщение
    Serial.printf("%s\n", payload); // Логируем полученное сообщение
    
    // Отправить данные на Arduino UNO через TX/RX
    Serial2.printf("%s\n", payload); // Отправка данных на Arduino

    // Отправка ответа обратно в WebSocket
    webSocket.sendTXT(num, "Message received"); // Ответ клиенту
  }
}

void setup() {
  // Инициализация последовательного порта для связи с Arduino UNO
  Serial.begin(115200); // Для отладки
  Serial2.begin(115200, SERIAL_8N1, 39, 40); // RX2=39, TX2=40
  pinMode(LED_BUILTIN, OUTPUT); // Установка пина LED как выход
  digitalWrite(LED_BUILTIN, HIGH);  // Включение LED
  delay(3000);                      // Задержка для визуализации
  digitalWrite(LED_BUILTIN, LOW);   // Выключение LED
  WiFi.softAP(ssid, password); // Запуск точки доступа WiFi

  webSocket.begin(); // Запуск WebSocket сервера
  webSocket.onEvent(onWebSocketEvent); // Регистрация обработчика событий WebSocket

  camera_config_t config; // Конфигурация камеры

  // Настройка параметров камеры
  config.ledc_channel = LEDC_CHANNEL_0; // Канал LEDC
  config.ledc_timer = LEDC_TIMER_0; // Таймер LEDC
  config.pin_d0 = 11; // Пин D0 для камеры
  config.pin_d1 = 9; // Пин D1 для камеры
  config.pin_d2 = 8; // Пин D2 для камеры
  config.pin_d3 = 10; // Пин D3 для камеры
  config.pin_d4 = 12; // Пин D4 для камеры
  config.pin_d5 = 18; // Пин D5 для камеры
  config.pin_d6 = 17; // Пин D6 для камеры
  config.pin_d7 = 16; // Пин D7 для камеры
  config.pin_xclk = 15; // Пин XCLK для камеры
  config.pin_pclk = 13; // Пин PCLK для камеры
  config.pin_vsync = 6; // Пин VSYNC для камеры
  config.pin_href = 7; // Пин HREF для камеры
  config.pin_sccb_sda = 4; // Пин SDA для SCCB
  config.pin_sccb_scl = 5; // Пин SCL для SCCB
  config.pin_pwdn = -1; // Пин для отключения питания (не используется)
  config.pin_reset = -1; // Пин для сброса (не используется)
  config.xclk_freq_hz = 20000000; // Частота XCLK
  config.frame_size = FRAMESIZE_VGA; // Размер кадра
  config.pixel_format = PIXFORMAT_JPEG;  // Формат пикселей для потока
  //config.pixel_format = PIXFORMAT_RGB565; // Формат для распознавания лиц
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY; // Режим захвата
  config.fb_location = CAMERA_FB_IN_PSRAM; // Местоположение буфера кадра
  config.jpeg_quality = 12; // Качество JPEG
  config.fb_count = 1; // Количество буферов кадра

  // Проверка наличия PSRAM и настройка параметров
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) { // Если PSRAM найден
      config.jpeg_quality = 10; // Установка качества JPEG
      config.fb_count = 2; // Увеличение количества буферов
      config.grab_mode = CAMERA_GRAB_LATEST; // Режим захвата последнего кадра
    } else {
      // Ограничение размера кадра при отсутствии PSRAM
      config.frame_size = FRAMESIZE_SVGA; // Установка размера кадра
      config.fb_location = CAMERA_FB_IN_DRAM; // Использование DRAM для буфера
    }
  } else {
    // Лучший вариант для распознавания лиц
    config.frame_size = FRAMESIZE_240X240; // Установка размера кадра
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2; // Увеличение количества буферов для ESP32S3
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP); // Настройка пина 13 как вход с подтяжкой
  pinMode(14, INPUT_PULLUP); // Настройка пина 14 как вход с подтяжкой
#endif

  // Инициализация камеры
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    // Если инициализация камеры не удалась, выходим из функции
    return;
  }

  sensor_t *s = esp_camera_sensor_get(); // Получение сенсора камеры
  if (s->id.PID == OV3660_PID) { // Если сенсор OV3660
    s->set_vflip(s, 1);        // Переворот изображения по вертикали
    s->set_brightness(s, 1);   // Увеличение яркости
    s->set_saturation(s, -2);  // Уменьшение насыщенности
  }
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA); // Установка размера кадра
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1); // Переворот изображения по вертикали для M5Stack
  s->set_hmirror(s, 1); // Отзеркаливание изображения для M5Stack
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1); // Переворот изображения по вертикали для ESP32S3
#endif
#if defined(LED_GPIO_NUM)
  setupLedFlash(); // Настройка мигания LED
#endif

  startCameraServer(); // Запуск сервера камеры

  // Вывод IP-адреса точки доступа
  // Serial.print("Camera Stream Ready! Go to: http://");
  // Serial.print(WiFi.softAPIP());
  // Serial.println("' to connect");
}

void loop() {
  webSocket.loop(); // Обработка событий WebSocket
}
