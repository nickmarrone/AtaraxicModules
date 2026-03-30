RACK_DIR ?= $(HOME)/src/Rack-SDK

# Flags will be included from $(RACK_DIR)/arch.mk and $(RACK_DIR)/compile.mk

# SOURCES specifies the source files
SOURCES += $(wildcard src/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
