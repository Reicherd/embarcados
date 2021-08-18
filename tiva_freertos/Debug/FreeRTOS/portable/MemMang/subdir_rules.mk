################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/portable/MemMang/%.o: ../FreeRTOS/portable/MemMang/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: GNU Compiler'
	"/home/albino/ti/ccs1020/ccs/tools/compiler/gcc-arm-none-eabi-7-2017-q4-major/bin/arm-none-eabi-gcc-7.2.1" -c -mcpu=cortex-m4 -march=armv7e-m -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DPART_TM4C1294NCPDT -Dgcc -DTARGET_IS_TM4C129_RA1 -I"/home/albino/Documents/embarcados/embarcados/tiva_freertos" -I"/home/albino/ti/tivaware/" -I"/home/albino/Documents/embarcados/embarcados/tiva_freertos/FreeRTOS-Plus-CLI" -I"/home/albino/Documents/embarcados/embarcados/tiva_freertos/config" -I"/home/albino/Documents/embarcados/embarcados/tiva_freertos/FreeRTOS/include" -I"/home/albino/Documents/embarcados/embarcados/tiva_freertos/FreeRTOS/portable/GCC/ARM_CM4F" -I"/home/albino/ti/ccs1020/ccs/tools/compiler/gcc-arm-none-eabi-7-2017-q4-major/arm-none-eabi/include" -Og -ffunction-sections -fdata-sections -g -gdwarf-3 -gstrict-dwarf -Wall -specs="nosys.specs" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


