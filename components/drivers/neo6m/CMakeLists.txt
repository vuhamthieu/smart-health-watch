idf_component_register(
    SRCS "src/neo6m.c"
    INCLUDE_DIRS "include"  # Trỏ đến tasks/gps/include
    REQUIRES neo6m          # Phụ thuộc vào neo6m
)