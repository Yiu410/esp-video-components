// ai_processor.cpp
#include "ai_processor.hpp"
#include "cJSON.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "sys/errno.h"
#include <string.h>
#include <sys/ioctl.h>

#include "dl_model_base.hpp"
#include "hand_detect.hpp"
#include "hand_gesture_recognition.hpp"
#include "human_face_detect.hpp"
#include "human_face_recognition.hpp"

static const char *TAG = "AI_TASK";

// External ASM binaries
extern const uint8_t user1_rgb_start[] asm("_binary_yiu_rgb_start");
extern const uint8_t user2_rgb_start[] asm("_binary_jerry_rgb_start");
extern const uint8_t user3_rgb_start[] asm("_binary_thomas_rgb_start");

// AI Models
static HandDetect *hand_detector = nullptr;
static HandGestureRecognizer *gesture_recognizer = nullptr;
static HumanFaceDetect *face_detector = nullptr;
static HumanFaceRecognizer *face_recognizer = nullptr;

// UDP define
#define JETSON_IP "10.42.0.1" // Replace with Orin's IP if different
#define UDP_PORT 5000

int udp_sock = -1;
struct sockaddr_in dest_addr;

void init_udp() {
  dest_addr.sin_addr.s_addr = inet_addr(JETSON_IP);
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(UDP_PORT);

  udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (udp_sock < 0) {
    ESP_LOGE(TAG, "Unable to create UDP socket");
  } else {
    ESP_LOGI(TAG, "UDP Ready! Aimed at Jetson Brain: %s:%d", JETSON_IP,
             UDP_PORT);
  }
}

// 2. Fire and forget the JSON string
void send_udp_json(const char *json_str) {
  if (udp_sock != -1 && strlen(json_str) > 0) {
    int err = sendto(udp_sock, json_str, strlen(json_str), 0,
                     (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
      ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
    }
  }
}

struct PreEnrollData {
  const uint8_t *image_data;
  int id;
  const char *name;
};

esp_err_t init_ai_models() {
  ESP_LOGI(TAG, "Loading AI Models...");
  hand_detector = new HandDetect();
  hand_detector->set_score_thr(0.5);
  gesture_recognizer = new HandGestureRecognizer();

  face_detector = new HumanFaceDetect();
  face_recognizer = new HumanFaceRecognizer("/spiffs/face.db",
                                            HumanFaceFeat::MBF_S8_V1, false);

  // Pre-enrollment logic (moved exactly from your app_main)
  if (face_recognizer->get_num_feats() == 0) {
    ESP_LOGI(TAG, "Running batch offline pre-enrollment...");
    PreEnrollData users_to_enroll[] = {{user1_rgb_start, 1, "Yiu"},
                                       {user2_rgb_start, 2, "Jerry"},
                                       {user3_rgb_start, 3, "Thomas"}};
    int num_users = sizeof(users_to_enroll) / sizeof(users_to_enroll[0]);
    for (int i = 0; i < num_users; i++) {
      dl::image::img_t pre_img;
      pre_img.data = (void *)users_to_enroll[i].image_data;
      pre_img.width = 240;
      pre_img.height = 240;
      pre_img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;

      auto &pre_face_results = face_detector->run(pre_img);
      if (!pre_face_results.empty()) {
        face_recognizer->enroll(pre_img, pre_face_results);
      }
    }
  }
  return ESP_OK;
}

static void ai_processing_task(void *arg) {
  web_cam_video_t *video = (web_cam_video_t *)arg;
  struct v4l2_buffer buf;
  char local_ai_json[256];

  while (1) {
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    // 1. Capture Frame (Blocks until sensor has a frame, not until network
    // sends)
    if (ioctl(video->fd, VIDIOC_DQBUF, &buf) != 0) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    uint32_t jpeg_encoded_size = 0;
    memset(local_ai_json, 0, sizeof(local_ai_json));

    cJSON *root = cJSON_CreateObject();

    // 2. Run AI Inference
    if (video->pixel_format != V4L2_PIX_FMT_JPEG) {
      dl::image::img_t img;
      img.data = video->buffer[buf.index];
      img.width = video->width;
      img.height = video->height;
      img.pix_type = (video->pixel_format == V4L2_PIX_FMT_RGB565)
                         ? dl::image::DL_IMAGE_PIX_TYPE_RGB565
                         : dl::image::DL_IMAGE_PIX_TYPE_RGB888;

      // Hand Detection

      auto &detect_results = hand_detector->run(img);
      if (!detect_results.empty()) {
        auto gesture_result =
            gesture_recognizer->recognize(img, detect_results);
        const dl::cls::result_t best_gesture = gesture_result.front();
        snprintf(local_ai_json, sizeof(local_ai_json),
                 "{\"detected\":true,\"gesture\":\"%s\"}",
                 best_gesture.cat_name);

        // cJSON_AddStringToObject(root, "type", "gesture");
        cJSON_AddStringToObject(root, "gesture_name", best_gesture.cat_name);
        cJSON_AddNumberToObject(root, "gesture_confidence", best_gesture.score);

        ESP_LOGI(TAG, "Gesture recognized: %s (score=%.4f)",
                 best_gesture.cat_name ? best_gesture.cat_name : "unknown",
                 best_gesture.score);
      } else {
        strcpy(local_ai_json, "{\"detected\":false}");
      }

      // Face Detection (truncated for brevity, insert your face logic here)
      auto &face_results = face_detector->run(img);
      if (!face_results.empty()) {
        auto recognize_results = face_recognizer->recognize(img, face_results);
        // Extract logic...

        if (!recognize_results.empty()) {
          if (recognize_results.front().id !=
              -1) { // If recognize() returns a single result struct (not a
                    // vector)
            ESP_LOGI(TAG, "Matched Face ID: %d (Similarity:%f)",
                     recognize_results.front().id,
                     recognize_results.front().similarity);
            // cJSON_AddStringToObject(root, "type", "face");
            cJSON_AddNumberToObject(root, "face_id",
                                    recognize_results.front().id);
            cJSON_AddNumberToObject(root, "face_similarity",
                                    recognize_results.front().similarity);
          }
        } else {
          ESP_LOGI(TAG, "Unknown Face Detected");
        }
      }

      char *local_ai_json = cJSON_PrintUnformatted(root);
      // BLAST IT OVER WI-FI TO THE ORIN
      send_udp_json(local_ai_json);
      // Clean up memory to prevent a leak
      cJSON_Delete(root);

      // 3. Encode to JPEG
      example_encoder_process(video->encoder_handle, video->buffer[buf.index],
                              video->buffer_size, video->jpeg_out_buf,
                              video->jpeg_out_size, &jpeg_encoded_size);
    } else {
      video->jpeg_out_buf = video->buffer[buf.index];
      jpeg_encoded_size = buf.bytesused;
    }

    // 4. Update Shared State thread-safely
    if (video->shared_state && xSemaphoreTake(video->shared_state->mutex,
                                              pdMS_TO_TICKS(50)) == pdTRUE) {
      // Copy the encoded JPEG and JSON to the shared buffer
      memcpy(video->shared_state->jpeg_buffer, video->jpeg_out_buf,
             jpeg_encoded_size);
      video->shared_state->jpeg_size = jpeg_encoded_size;
      strncpy(video->shared_state->ai_result_json, local_ai_json,
              sizeof(video->shared_state->ai_result_json));
      video->shared_state->frame_count++;

      xSemaphoreGive(video->shared_state->mutex);
    }

    // 5. Release Frame back to camera
    ioctl(video->fd, VIDIOC_QBUF, &buf);

    // Yield slightly to prevent complete CPU starvation
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void start_ai_processing_task(web_cam_video_t *video_config) {
  // Allocate the shared state buffers in PSRAM (SPIRAM) to prevent
  // out-of-memory errors
  video_config->shared_state = new shared_frame_state_t();
  video_config->shared_state->mutex = xSemaphoreCreateMutex();
  video_config->shared_state->jpeg_buffer = (uint8_t *)heap_caps_malloc(
      video_config->jpeg_out_size, MALLOC_CAP_SPIRAM);
  video_config->shared_state->jpeg_size = 0;
  video_config->shared_state->frame_count = 0;

  // Pin to core 1 (leaving core 0 for WiFi/Network)
  xTaskCreatePinnedToCore(ai_processing_task, "AI_Core_Task", 1024 * 32,
                          (void *)video_config, 5, NULL, 1);
}