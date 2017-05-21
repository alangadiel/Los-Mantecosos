################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Consola.c \
/home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/Helper.c \
/home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/SocketsL.c 

OBJS += \
./src/Consola.o \
./src/Helper.o \
./src/SocketsL.o 

C_DEPS += \
./src/Consola.d \
./src/Helper.d \
./src/SocketsL.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas" -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/Helper.o: /home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/Helper.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas" -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/SocketsL.o: /home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas/SocketsL.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2017-1c-Los-Mantecosos/Bibliotecas" -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


