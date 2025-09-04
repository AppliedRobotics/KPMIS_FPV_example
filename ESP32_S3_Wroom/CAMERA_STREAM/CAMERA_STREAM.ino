#include "esp_http_server.h"      // Для работы с HTTP-сервером ESP32
#include "esp_timer.h"            // Для работы с таймерами высокого разрешения
#include "img_converters.h"       // Для конвертации форматов изображений
#include "fb_gfx.h"               // Графические функции для фреймбуфера
#include "esp32-hal-ledc.h"       // Для управления ШИМ (LED Control)
#include "sdkconfig.h"            // Конфигурация проекта из menuconfig

#include "esp_camera.h"           // Основная библиотека работы с камерой
#include <WiFi.h>                 // Библиотека для работы с Wi-Fi

// Определение границы для multipart-ответа (формат MJPEG)
#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

// Глобальная переменная для хранения handle HTTP-сервера
httpd_handle_t stream_httpd = NULL;

// ===========================
// Введите ваши учетные данные Wi-Fi
// ===========================
const char *ssid = "ESP32";        // SSID точки доступа
const char *password = "01234567"; // Пароль точки доступа

// Обработчик для streaming видео
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;         // Указатель на фреймбуфер камеры
  struct timeval _timestamp;      // Временная метка кадра
  esp_err_t res = ESP_OK;         // Результат выполнения операций
  size_t _jpg_buf_len = 0;        // Длина JPEG буфера
  uint8_t *_jpg_buf = NULL;       // Буфер для JPEG данных
  char *part_buf[128];            // Буфер для HTTP заголовков

  static int64_t last_frame = 0;  // Время последнего кадра для расчета FPS
  if (!last_frame) {
    last_frame = esp_timer_get_time(); // Инициализация при первом вызове
  }

  // Установка типа контента для streaming
  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res; // В случае ошибки возвращаем код
  }

  // Установка CORS заголовков для кросс-доменных запросов
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "X-Framerate", "60"); // Заголовок с информацией о FPS

  // Основной цикл streaming
  while (true) {
    fb = esp_camera_fb_get(); // Получение кадра от камеры
    if (!fb) {
      log_e("Camera capture failed"); // Логирование ошибки
      res = ESP_FAIL;
    } else {
      _timestamp.tv_sec = fb->timestamp.tv_sec;     // Сохранение временной метки
      _timestamp.tv_usec = fb->timestamp.tv_usec;
      
      // Конвертация в JPEG если необходимо
      if (fb->format != PIXFORMAT_JPEG) {
        bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        esp_camera_fb_return(fb); // Освобождение фреймбуфера
        fb = NULL;
        if (!jpeg_converted) {
          log_e("JPEG compression failed");
          res = ESP_FAIL;
        }
      } else {
        _jpg_buf_len = fb->len; // Если уже JPEG, используем как есть
        _jpg_buf = fb->buf;
      }
    }
    
    // Отправка границы multipart
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    
    // Отправка HTTP заголовков для кадра
    if (res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    
    // Отправка самих JPEG данных
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    
    // Освобождение ресурсов
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf); // Освобождение если буфер был создан при конвертации
      _jpg_buf = NULL;
    }
    
    // Проверка ошибок отправки
    if (res != ESP_OK) {
      log_e("Send frame failed");
      break;
    }
    
    // Расчет и логирование FPS
    int64_t fr_end = esp_timer_get_time();
    int64_t frame_time = fr_end - last_frame;
    last_frame = fr_end;
    frame_time /= 1000; // Конвертация в миллисекунды
    
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
    uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
#endif
    log_i(
      "MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps)", 
      (uint32_t)(_jpg_buf_len), 
      (uint32_t)frame_time, 
      1000.0 / (uint32_t)frame_time, 
      avg_frame_time,
      1000.0 / avg_frame_time
    );
  }

  return res;
}

// Обработчик для главной страницы
static esp_err_t index_handler(httpd_req_t *req) {
  // HTML страница с видео потоком
  String html = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<title>Camera Stream</title>"
                "<style>"
                "body { text-align: center; position: relative; overflow: hidden; height: 100vh; }"
                "</style>"
                "</head>"
                "<body>"
                "<img src=\"/stream\" style=\"width: 100%; height: auto;\" />"
                "</body>"
                "</html>";

  httpd_resp_send(req, html.c_str(), html.length());
  return ESP_OK;
}

// Запуск HTTP сервера
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 16; // Максимальное количество обработчиков

  // Регистрация обработчиков
  httpd_uri_t index_uri = {
    .uri       = "/",            // Корневой URL
    .method    = HTTP_GET,       // HTTP метод
    .handler   = index_handler,  // Функция-обработчик
    .user_ctx  = NULL            // Контекст пользователя
  };

  httpd_uri_t stream_uri = {
    .uri       = "/stream",      // URL для streaming
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  // Запуск сервера и регистрация обработчиков
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &index_uri);
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

// Прототип функции для настройки LED вспышки
void setupLedFlash();

// Функция setup - выполняется один раз при старте
void setup() {
  WiFi.softAP(ssid, password); // Запуск точки доступа
  Serial.println("Access Point started");
  Serial.begin(115200);        // Инициализация последовательного порта
  Serial.setDebugOutput(true); // Включение debug вывода
  Serial.println();

  // Конфигурация камеры
  camera_config_t config;

  // Настройка пинов камеры
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 11;
  config.pin_d1 = 9;
  config.pin_d2 = 8;
  config.pin_d3 = 10;
  config.pin_d4 = 12;
  config.pin_d5 = 18;
  config.pin_d6 = 17;
  config.pin_d7 = 16;
  config.pin_xclk = 15;
  config.pin_pclk = 13;
  config.pin_vsync = 6;
  config.pin_href = 7;
  config.pin_sccb_sda = 4;
  config.pin_sccb_scl = 5;
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;  // Частота тактирования камеры
  config.frame_size = FRAMESIZE_UXGA; // Размер кадра
  config.pixel_format = PIXFORMAT_JPEG;  // для streaming
  //config.pixel_format = PIXFORMAT_RGB565; // для face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;        // Качество JPEG
  config.fb_count = 1;             // Количество фреймбуферов

  // Оптимизация для PSRAM
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {            // Если есть PSRAM
      config.jpeg_quality = 10;    // Лучшее качество
      config.fb_count = 2;         // Двойной буфер
      config.grab_mode = CAMERA_GRAB_LATEST; // Последний кадр
    } else {
      // Ограничения при отсутствии PSRAM
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Оптимальные настройки для распознавания лиц
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

// Специфичные настройки для разных моделей камер
#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // Инициализация камеры
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Настройка сенсора камеры
  sensor_t *s = esp_camera_sensor_get();
  // Коррекция для OV3660 сенсора
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // Вертикальное отражение
    s->set_brightness(s, 1);   // Яркость
    s->set_saturation(s, -2);  // Насыщенность
  }
  
  // Уменьшение размера кадра для большего FPS
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

// Специфичные настройки отражения для разных плат
#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Настройка LED вспышки если определен пин
#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

  Serial.print("WiFi connecting");
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer(); // Запуск HTTP сервера

  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.print(WiFi.softAPIP()); // Вывод IP-адреса точки доступа
  Serial.println("' to connect");
}

// Основной цикл - почти ничего не делает, все обработки в отдельных задачах
void loop() {
  delay(10000); // Простая задержка
}
