TARGET = car
OBJS = main.o common.o map.o

INCDIR =
CFLAGS = -G0 -Wall -O2
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lpspgum -lpspgu -lpsprtc -lpsppower -lm -lstdc++

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = CAR TEST

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak