################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/entity.c \
../src/gui.c \
../src/ingame.c \
../src/main.c \
../src/models.c \
../src/network.c \
../src/render.c \
../src/world.c \
../src/xstring.c 

OBJS += \
./src/entity.o \
./src/gui.o \
./src/ingame.o \
./src/main.o \
./src/models.o \
./src/network.o \
./src/render.o \
./src/world.o \
./src/xstring.o 

C_DEPS += \
./src/entity.d \
./src/gui.d \
./src/ingame.d \
./src/main.d \
./src/models.d \
./src/network.d \
./src/render.d \
./src/world.d \
./src/xstring.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	i686-w64-mingw32-gcc -std=gnu11 -I/mingw/include/ -I/usr/share/mingw-w64/include -O0 -g3 -Wall -c -fmessage-length=0 -m32 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

