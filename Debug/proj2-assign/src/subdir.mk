################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../proj2-assign/src/hfpage.o \
../proj2-assign/src/main.o 

C_UPPER_SRCS += \
../proj2-assign/src/BMTester.C \
../proj2-assign/src/buf.C \
../proj2-assign/src/db.C \
../proj2-assign/src/main.C \
../proj2-assign/src/new_error.C \
../proj2-assign/src/page.C \
../proj2-assign/src/system_defs.C \
../proj2-assign/src/test_driver.C 

OBJS += \
./proj2-assign/src/BMTester.o \
./proj2-assign/src/buf.o \
./proj2-assign/src/db.o \
./proj2-assign/src/main.o \
./proj2-assign/src/new_error.o \
./proj2-assign/src/page.o \
./proj2-assign/src/system_defs.o \
./proj2-assign/src/test_driver.o 

C_UPPER_DEPS += \
./proj2-assign/src/BMTester.d \
./proj2-assign/src/buf.d \
./proj2-assign/src/db.d \
./proj2-assign/src/main.d \
./proj2-assign/src/new_error.d \
./proj2-assign/src/page.d \
./proj2-assign/src/system_defs.d \
./proj2-assign/src/test_driver.d 


# Each subdirectory must supply rules for building sources it contributes
proj2-assign/src/%.o: ../proj2-assign/src/%.C
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


