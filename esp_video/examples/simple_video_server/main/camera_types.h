// camera_types.h
#pragma once
#include "example_video_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdint.h>

// The shared state between the AI Task and the Web Streamer
typedef struct {
  SemaphoreHandle_t mutex;
  uint8_t *jpeg_buffer; // Holds the latest encoded JPEG
  uint32_t jpeg_size;
  char ai_result_json[256]; // Holds the latest AI prediction
  uint32_t frame_count;     // To track new frames
} shared_frame_state_t;

typedef struct web_cam_video {
  int fd;
  uint8_t index;
  example_encoder_handle_t encoder_handle;
  uint8_t *jpeg_out_buf;
  uint32_t jpeg_out_size;
  uint8_t *buffer[CONFIG_EXAMPLE_CAMERA_VIDEO_BUFFER_NUMBER];
  uint32_t buffer_size;
  uint32_t width;
  uint32_t height;
  uint32_t pixel_format;
  uint8_t jpeg_quality;
  uint32_t frame_rate;
  SemaphoreHandle_t sem;
  uint32_t support_control_jpeg_quality : 1;

  // NEW: Pointer to our shared thread-safe state
  shared_frame_state_t *shared_state;
} web_cam_video_t;

typedef struct web_cam {
  uint8_t video_count;
  web_cam_video_t video[0];
} web_cam_t;