BUILD_ROOT = $(realpath $(PHANTOM_HOME))/phantom

TARGET	= isomem.a

SRCDIRS += $(BUILD_ROOT)/isomem

INCDIRS += $(BUILD_ROOT)/isomem

EXCLUDED_OBJFILES=pvm_main.o root.o

# Uncomment to enable tracing
# PHANTOM_CFLAGS += -finstrument-functions

include $(BUILD_ROOT)/makerules-lib

include $(BUILD_ROOT)/isomem/makerules
