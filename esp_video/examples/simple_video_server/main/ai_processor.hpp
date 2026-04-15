// ai_processor.hpp
#pragma once
#include "camera_types.h"
#include "esp_err.h"

// Initialize models and database
esp_err_t init_ai_models();

// Spawn the background task
void start_ai_processing_task(web_cam_video_t *video_config);