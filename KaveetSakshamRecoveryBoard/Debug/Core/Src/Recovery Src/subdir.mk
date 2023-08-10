################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Recovery\ Src/Aprs.c \
../Core/Src/Recovery\ Src/AprsPacket.c \
../Core/Src/Recovery\ Src/AprsTransmit.c \
../Core/Src/Recovery\ Src/GPS.c \
../Core/Src/Recovery\ Src/VHF.c 

OBJS += \
./Core/Src/Recovery\ Src/Aprs.o \
./Core/Src/Recovery\ Src/AprsPacket.o \
./Core/Src/Recovery\ Src/AprsTransmit.o \
./Core/Src/Recovery\ Src/GPS.o \
./Core/Src/Recovery\ Src/VHF.o 

C_DEPS += \
./Core/Src/Recovery\ Src/Aprs.d \
./Core/Src/Recovery\ Src/AprsPacket.d \
./Core/Src/Recovery\ Src/AprsTransmit.d \
./Core/Src/Recovery\ Src/GPS.d \
./Core/Src/Recovery\ Src/VHF.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Recovery\ Src/Aprs.o: ../Core/Src/Recovery\ Src/Aprs.c Core/Src/Recovery\ Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U575xx -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_NON_SECURE=1 -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../AZURE_RTOS/App -I../Middlewares/ST/threadx/common/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"Core/Src/Recovery Src/Aprs.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/Recovery\ Src/AprsPacket.o: ../Core/Src/Recovery\ Src/AprsPacket.c Core/Src/Recovery\ Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U575xx -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_NON_SECURE=1 -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../AZURE_RTOS/App -I../Middlewares/ST/threadx/common/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"Core/Src/Recovery Src/AprsPacket.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/Recovery\ Src/AprsTransmit.o: ../Core/Src/Recovery\ Src/AprsTransmit.c Core/Src/Recovery\ Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U575xx -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_NON_SECURE=1 -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../AZURE_RTOS/App -I../Middlewares/ST/threadx/common/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"Core/Src/Recovery Src/AprsTransmit.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/Recovery\ Src/GPS.o: ../Core/Src/Recovery\ Src/GPS.c Core/Src/Recovery\ Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U575xx -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_NON_SECURE=1 -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../AZURE_RTOS/App -I../Middlewares/ST/threadx/common/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"Core/Src/Recovery Src/GPS.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/Recovery\ Src/VHF.o: ../Core/Src/Recovery\ Src/VHF.c Core/Src/Recovery\ Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U575xx -DTX_INCLUDE_USER_DEFINE_FILE -DTX_SINGLE_MODE_NON_SECURE=1 -c -I../Core/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc -I../Drivers/STM32U5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U5xx/Include -I../Drivers/CMSIS/Include -I../AZURE_RTOS/App -I../Middlewares/ST/threadx/common/inc -I../Middlewares/ST/threadx/ports/cortex_m33/gnu/inc -I../Middlewares/ST/threadx/utility/low_power -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"Core/Src/Recovery Src/VHF.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Recovery-20-Src

clean-Core-2f-Src-2f-Recovery-20-Src:
	-$(RM) ./Core/Src/Recovery\ Src/Aprs.cyclo ./Core/Src/Recovery\ Src/Aprs.d ./Core/Src/Recovery\ Src/Aprs.o ./Core/Src/Recovery\ Src/Aprs.su ./Core/Src/Recovery\ Src/AprsPacket.cyclo ./Core/Src/Recovery\ Src/AprsPacket.d ./Core/Src/Recovery\ Src/AprsPacket.o ./Core/Src/Recovery\ Src/AprsPacket.su ./Core/Src/Recovery\ Src/AprsTransmit.cyclo ./Core/Src/Recovery\ Src/AprsTransmit.d ./Core/Src/Recovery\ Src/AprsTransmit.o ./Core/Src/Recovery\ Src/AprsTransmit.su ./Core/Src/Recovery\ Src/GPS.cyclo ./Core/Src/Recovery\ Src/GPS.d ./Core/Src/Recovery\ Src/GPS.o ./Core/Src/Recovery\ Src/GPS.su ./Core/Src/Recovery\ Src/VHF.cyclo ./Core/Src/Recovery\ Src/VHF.d ./Core/Src/Recovery\ Src/VHF.o ./Core/Src/Recovery\ Src/VHF.su

.PHONY: clean-Core-2f-Src-2f-Recovery-20-Src

