################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/CapaFS.c \
../src/CapaMemoria.c \
/home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/Helper.c \
../src/Kernel.c \
../src/ReceptorKernel.c \
../src/Service.c \
/home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/SocketsL.c \
../src/ThreadsKernel.c \
../src/UserInterface.c 

OBJS += \
./src/CapaFS.o \
./src/CapaMemoria.o \
./src/Helper.o \
./src/Kernel.o \
./src/ReceptorKernel.o \
./src/Service.o \
./src/SocketsL.o \
./src/ThreadsKernel.o \
./src/UserInterface.o 

C_DEPS += \
./src/CapaFS.d \
./src/CapaMemoria.d \
./src/Helper.d \
./src/Kernel.d \
./src/ReceptorKernel.d \
./src/Service.d \
./src/SocketsL.d \
./src/ThreadsKernel.d \
./src/UserInterface.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"../../Bibliotecas" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/Helper.o: /home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/Helper.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"../../Bibliotecas" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/Kernel.o: /home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Kernel/src/Kernel.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"../../Bibliotecas" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/SocketsL.o: /home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/SocketsL.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"../../Bibliotecas" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


