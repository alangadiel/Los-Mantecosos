################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/Helper.c \
../src/Kernel.c \
/home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/SocketsL.c 

OBJS += \
./src/Helper.o \
./src/Kernel.o \
./src/SocketsL.o 

C_DEPS += \
./src/Helper.d \
./src/Kernel.d \
./src/SocketsL.d 


# Each subdirectory must supply rules for building sources it contributes
src/Helper.o: /home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/Helper.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/Kernel.o: /home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Kernel/src/Kernel.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/SocketsL.o: /home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/SocketsL.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


