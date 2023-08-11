################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Lib\ Src/minmea.c 

OBJS += \
./Core/Src/Lib\ Src/minmea.o 

C_DEPS += \
./Core/Src/Lib\ Src/minmea.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Lib\ Src/minmea.o: ../Core/Src/Lib\ Src/minmea.c Core/Src/Lib\ Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U575xx -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_NON_SECURE=1 -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../AZURE_RTOS/App -I../Middlewares/ST/threadx/common/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"Core/Src/Lib Src/minmea.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Lib-20-Src

clean-Core-2f-Src-2f-Lib-20-Src:
	-$(RM) ./Core/Src/Lib\ Src/minmea.cyclo ./Core/Src/Lib\ Src/minmea.d ./Core/Src/Lib\ Src/minmea.o ./Core/Src/Lib\ Src/minmea.su

.PHONY: clean-Core-2f-Src-2f-Lib-20-Src

