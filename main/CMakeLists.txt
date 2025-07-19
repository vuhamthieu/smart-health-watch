idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES              # liệt kê các component mà main trực tiếp dùng
        i2c       # component utils/i2c
        mlx90614          # component drivers/mlx90614
        ssd1306         # component libs/u8g2   
        u8g2              # thư viện u8g2
        temperature  # task đọc nhiệt độ
        display      # task hiển thị OLED
)
