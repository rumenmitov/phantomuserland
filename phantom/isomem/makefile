BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= isomem.a

SRCDIRS += $(BUILD_ROOT)/isomem

INCDIRS += $(BUILD_ROOT)/isomem

# EXCLUDED_OBJFILES=pvm_main.o win_screen.o win_hal.o win_bulk.o nonstandalone.o x11_screen.o x11_display.o win_hal_win.o win_screen_win.o headless_screen.o unix_hal.o unix_hal_unix.o 

# Uncomment to enable tracing
# PHANTOM_CFLAGS += -finstrument-functions

include $(BUILD_ROOT)/makerules-lib

include $(BUILD_ROOT)/isomem/makerules
