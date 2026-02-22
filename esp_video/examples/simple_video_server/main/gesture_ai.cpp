#include "esp_ppa.h"
#include "esp_video_device.h"
#include "hand_gesture_recognition.hpp"

extern "C" char current_gesture_name[32] = "None";

void gesture_ai_task(void *pvParameters) {
  // 1. Initialize the Model (stored in Flash/PSRAM)
  HandGestureRecognition *recognizer = new HandGestureRecognition();

  // 2. Prepare a small RGB888 buffer for the AI (224x224 is standard)
  uint8_t *ai_input =
      (uint8_t *)heap_caps_malloc(224 * 224 * 3, MALLOC_CAP_SPIRAM);

  while (1) {
    // We'll receive a signal or pointer from the Video Server here
    // For simplicity, we "tap" the frame from the video server's queue
    esp_video_fbuf_t *fbuf = get_latest_frame();

    if (fbuf) {
      // 3. Use PPA Hardware to resize and convert JPEG/YUV to RGB888
      // This happens in hardware, zero CPU usage!
      ppa_convert_and_resize(fbuf, ai_input, 224, 224);

      // 4. Run Inference (PIE-accelerated)
      auto results = recognizer->infer(ai_input);
      if (!results.empty()) {
        strcpy(current_gesture_name, results.front().label);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50)); // Run at ~20 FPS
  }
}