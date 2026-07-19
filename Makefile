TARGET = car
OBJS = main.o common.o map.o psptexture.o

INCDIR =
CFLAGS = -G0 -Wall -O2
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lpspgum_vfpu -lpspvfpu -lpspgu -lpsprtc -lpsppower -lm -lstdc++

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Car Sandbox Prototype

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak