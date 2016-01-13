################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/C/SpinnIO/ExternalRateCoder.cpp 

OBJS += \
./src/C/SpinnIO/ExternalRateCoder.o 

CPP_DEPS += \
./src/C/SpinnIO/ExternalRateCoder.d 


# Each subdirectory must supply rules for building sources it contributes
src/C/SpinnIO/%.o: ../src/C/SpinnIO/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


