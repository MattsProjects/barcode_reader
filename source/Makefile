# Makefile for Basler pylon sample program
.PHONY: all clean

# The program to build
NAME       := barcodereader

# Installation directories for pylon
PYLON_ROOT ?= /opt/pylon5

# Build tools and flags
LD         := $(CXX)
CPPFLAGS   := $(shell $(PYLON_ROOT)/bin/pylon-config --cflags) -std=c++11
CXXFLAGS   := -I../include #e.g., CXXFLAGS=-g -O0 for debugging
LDFLAGS    := $(shell $(PYLON_ROOT)/bin/pylon-config --libs-rpath)

# uncomment to use prebuilt zxing armhf library
#LDLIBS     := $(shell $(PYLON_ROOT)/bin/pylon-config --libs) -L../lib/zxing/linux/armhf -L/usr/lib -lzxing -lopencv_core -lopencv_highgui -lopencv_imgproc
# uncomment to use prebuilt zxing x86 library
LDLIBS     := $(shell $(PYLON_ROOT)/bin/pylon-config --libs) -L../lib/zxing/linux/x86 -L/usr/lib -lzxing -lopencv_core -lopencv_highgui -lopencv_imgproc

# Rules for building
all: $(NAME)

$(NAME): $(NAME).o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(NAME).o: $(NAME).cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	$(RM) $(NAME).o $(NAME)
