#ifndef PARALLEL_LCD_CONFIGURATION_H
#  define PARALLEL_LCD_CONFIGURATION_H

#  define CONFIG_ENTRY_MODE 0x04
#  define CONFIG_INCREMENTAL 0x02
#  define CONFIG_DECREMENTAL 0x00
#  define CONFIG_SHIFT_ON 0x01
#  define CONFIG_SHIFT_OFF 0x00

#  define CONFIG_DISPLAY 0x08
#  define CONFIG_DISPLAY_ON 0x04
#  define CONFIG_CURSOR_ON 0x02
#  define CONFIG_BLINK_ON 0x01

#  define CONFIG_DISPLAY_OR_CURSOR 0x10
#  define CONFIG_DISPLAY_SHIFT 0x08
#  define CONFIG_CURSOR_MOVEMENT 0x00
#  define CONFIG_CURSOR_MOVEMENT_LEFT 0x04
#  define CONFIG_CURSOR_MOVEMENT_RIGHT 0x00

#  define CONFIG_FUNCTION_SET 0x20
#  define CONFIG_8BIT_MODE 0x10
#  define CONFIG_4BIT_MODE 0x00
#  define CONFIG_2LINE_MODE 0x08
#  define CONFIG_1LINE_MODE 0x00
#  define CONFIG_5x11DOT_MODE 0x04
#  define CONFIG_5x8DOT_MODE 0x00

#endif	/* PARALLEL_LCD_CONFIGURATION_H */
