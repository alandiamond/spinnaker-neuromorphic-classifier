################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/C/SpinnIO/src/DatabaseIF.cpp \
../src/C/SpinnIO/src/EIEIOMessage.cpp \
../src/C/SpinnIO/src/EIEIOReceiver.cpp \
../src/C/SpinnIO/src/EIEIOSender.cpp \
../src/C/SpinnIO/src/SocketIF.cpp \
../src/C/SpinnIO/src/SpikeInjectionMultiPopulation.cpp \
../src/C/SpinnIO/src/SpikeInjectionPopulation.cpp \
../src/C/SpinnIO/src/SpikeReceiverPopulation.cpp \
../src/C/SpinnIO/src/StopWatch.cpp \
../src/C/SpinnIO/src/Threadable.cpp 

OBJS += \
./src/C/SpinnIO/src/DatabaseIF.o \
./src/C/SpinnIO/src/EIEIOMessage.o \
./src/C/SpinnIO/src/EIEIOReceiver.o \
./src/C/SpinnIO/src/EIEIOSender.o \
./src/C/SpinnIO/src/SocketIF.o \
./src/C/SpinnIO/src/SpikeInjectionMultiPopulation.o \
./src/C/SpinnIO/src/SpikeInjectionPopulation.o \
./src/C/SpinnIO/src/SpikeReceiverPopulation.o \
./src/C/SpinnIO/src/StopWatch.o \
./src/C/SpinnIO/src/Threadable.o 

CPP_DEPS += \
./src/C/SpinnIO/src/DatabaseIF.d \
./src/C/SpinnIO/src/EIEIOMessage.d \
./src/C/SpinnIO/src/EIEIOReceiver.d \
./src/C/SpinnIO/src/EIEIOSender.d \
./src/C/SpinnIO/src/SocketIF.d \
./src/C/SpinnIO/src/SpikeInjectionMultiPopulation.d \
./src/C/SpinnIO/src/SpikeInjectionPopulation.d \
./src/C/SpinnIO/src/SpikeReceiverPopulation.d \
./src/C/SpinnIO/src/StopWatch.d \
./src/C/SpinnIO/src/Threadable.d 


# Each subdirectory must supply rules for building sources it contributes
src/C/SpinnIO/src/%.o: ../src/C/SpinnIO/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


