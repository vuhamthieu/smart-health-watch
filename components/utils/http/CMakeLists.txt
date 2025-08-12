idf_component_register(SRCS "src/http_client.c" 
                      INCLUDE_DIRS "include"
                      REQUIRES esp_http_client temperature gps health wifi mqttc
                      PRIV_REQUIRES json )