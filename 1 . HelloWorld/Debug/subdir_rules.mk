################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/bin/cl2000" -v28 -ml -mt --float_support=fpu32 --include_path="C:/ti/Plantilla" --include_path="C:/ti/c2000/C2000Ware_6_00_00_00/device_support/f2833x/common/include" --include_path="C:/ti/c2000/C2000Ware_6_00_00_00/device_support/f2833x/headers/include" --include_path="C:/ti/c2000/C2000Ware_6_00_00_00/libraries/math/FPUfastRTS/c28/include" --include_path="C:/ti/c2000/C2000Ware_6_00_00_00/libraries/math/IQmath/c28/include" --include_path="C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/include" --include_path="C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/include" --define="_DEBUG" --define="LARGE_MODEL" -g --diag_warning=225 --diag_wrap=off --display_error_number --abi=coffabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/bin/cl2000" -v28 -ml -mt --float_support=fpu32 --include_path="C:/ti/Plantilla" --include_path="C:/ti/c2000/C2000Ware_6_00_00_00/device_support/f2833x/common/include" --include_path="C:/ti/c2000/C2000Ware_6_00_00_00/device_support/f2833x/headers/include" --include_path="C:/ti/c2000/C2000Ware_6_00_00_00/libraries/math/FPUfastRTS/c28/include" --include_path="C:/ti/c2000/C2000Ware_6_00_00_00/libraries/math/IQmath/c28/include" --include_path="C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/include" --include_path="C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS/include" --define="_DEBUG" --define="LARGE_MODEL" -g --diag_warning=225 --diag_wrap=off --display_error_number --abi=coffabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


