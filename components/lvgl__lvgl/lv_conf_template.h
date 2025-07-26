/**
 * @file lv_conf.h
 * Configuration file for LVGL v9
 */

#if 1 /*Set it to "1" to enable content*/
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0
#define LV_COLOR_SCREEN_TRANSP 0
#define LV_COLOR_MIX_ROUND_OFS 0
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)

/*=========================
   MEMORY SETTINGS
 *=========================*/
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (48U * 1024U)          // Use 48KB for better performance
#define LV_MEM_ADR 0
#define LV_MEM_BUF_MAX_NUM 16

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DISP_DEF_REFR_PERIOD 100      
#define LV_INDEV_DEF_READ_PERIOD 30
#define LV_TICK_CUSTOM 0                   // Let LVGL handle ticks
#define LV_DPI_DEF 130

/*====================
   FEATURE CONFIGURATION
 *====================*/
#define LV_DRAW_COMPLEX 1
#define LV_SHADOW_CACHE_SIZE 0
#define LV_CIRCLE_CACHE_SIZE 4
#define LV_LAYER_SIMPLE_BUF_SIZE (8 * 1024)          // Reduced for ESP32
#define LV_LAYER_SIMPLE_FALLBACK_BUF_SIZE (2 * 1024) // Reduced for ESP32
#define LV_IMG_CACHE_DEF_SIZE 0
#define LV_GRADIENT_MAX_STOPS 2
#define LV_GRAD_CACHE_DEF_SIZE 0
#define LV_DITHER_GRADIENT 0
#define LV_DISP_ROT_MAX_BUF (2*1024)      // Reduced for ESP32

/*-------------
 * Logging - ENABLE for debugging
 *-----------*/
#define LV_USE_LOG 1                       // ENABLE logging for debugging
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF 1
#endif

/*-------------
 * Asserts
 *-----------*/
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0

/*==================
   FONT USAGE
 *===================*/
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*==================
 * WIDGET USAGE  
 *================*/
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BTN 1  
#define LV_USE_BTNMATRIX 1
#define LV_USE_CANVAS 0              // Disable to save memory
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 1
#define LV_USE_LIST 1

/*==================
 * EXTRA COMPONENTS
 *==================*/
#define LV_USE_ANIMIMG 0             // Disable to save memory
#define LV_USE_CALENDAR 0            // Disable to save memory
#define LV_USE_CHART 0               // Disable to save memory
#define LV_USE_COLORWHEEL 0          // Disable to save memory
#define LV_USE_IMGBTN 1
#define LV_USE_KEYBOARD 0            // Disable to save memory
#define LV_USE_LED 1
#define LV_USE_MENU 0                // Disable to save memory
#define LV_USE_METER 0               // Disable to save memory
#define LV_USE_MSGBOX 1
#define LV_USE_SPAN 0                // Disable to save memory
#define LV_USE_SPINBOX 0             // Disable to save memory
#define LV_USE_SPINNER 1
#define LV_USE_TABVIEW 0             // Disable to save memory
#define LV_USE_TILEVIEW 0            // Disable to save memory
#define LV_USE_WIN 0                 // Disable to save memory

/*-----------
 * Themes
 *----------*/
#define LV_USE_THEME_DEFAULT 1
#define LV_USE_THEME_BASIC 0         // Disable to save memory
#define LV_USE_THEME_MONO 0          // Disable to save memory

/*-----------
 * Layouts
 *----------*/
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*====================
   COMPILER SETTINGS
 *====================*/
#define LV_BIG_ENDIAN_SYSTEM 0
#define LV_ATTRIBUTE_TICK_INC
#define LV_ATTRIBUTE_TIMER_HANDLER
#define LV_ATTRIBUTE_FLUSH_READY
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 4
#define LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_FAST_MEM
#define LV_ATTRIBUTE_DMA
#define LV_USE_LARGE_COORD 0

// Disable demos and examples to save memory
#define LV_BUILD_EXAMPLES 0
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_STRESS 0
#define LV_USE_DEMO_MUSIC 0

#endif /*LV_CONF_H*/
#endif /*End of "Content enable"*/
