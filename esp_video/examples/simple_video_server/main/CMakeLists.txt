set(srcs "simple_video_server_example.cpp")
set(html_files "../frontend/gzipped/index.html.gz"
               "../frontend/gzipped/loading.jpg.gz"
               "../frontend/gzipped/favicon.ico.gz"
               "../frontend/gzipped/assets/index.js.gz"
               "../frontend/gzipped/assets/index.css.gz")

idf_component_register(SRCS "${srcs}"
                       PRIV_INCLUDE_DIRS .
                       EMBED_TXTFILES ${html_files})

target_add_binary_data(${COMPONENT_LIB} "models/espdet_pico_224_224_hand.espdl" TEXT)
target_add_binary_data(${COMPONENT_LIB} "models/mobilenetv2_0_5_128_128_gesture.espdl" TEXT)

target_add_binary_data(${COMPONENT_LIB} "models/human_face_detect_mnp_s8_v1.espdl" TEXT)
target_add_binary_data(${COMPONENT_LIB} "models/human_face_feat_mbf_s8_v1.espdl" TEXT)

target_add_binary_data(${COMPONENT_TARGET} "face_db/jerry.rgb" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "face_db/thomas.rgb" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "face_db/yiu.rgb" TEXT)