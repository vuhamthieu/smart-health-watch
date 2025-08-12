idf_component_register(
  SRCS "src/mqtt.c" "src/mqtt_task.c"
  INCLUDE_DIRS "include"
  REQUIRES wifi mqtt http
  PRIV_REQUIRES json
)