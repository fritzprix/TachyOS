# TachyOS

[![Build Status](https://travis-ci.org/fritzprix/TachyOS.svg?branch=master)](https://travis-ci.org/fritzprix/TachyOS)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/bc5b451d38624e44a58c8fd4913bc4a5)](https://www.codacy.com/app/innocentevil0914/TachyOS?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=fritzprix/TachyOS&amp;utm_campaign=Badge_Grade)

## Note

> This project is subjected to serious HOMEBREWING and still in INFANCY. So any experimental change can be made without any notice.  

## About

> TachyOS is the RTOS based on microkernel architecture which includes only minimal components like thread / synchronization, memory management, inter-thread communication while supporting execution context / address space isolation(protection) and extensible modular interface. Name of this project comes from the hypothetical particle called 'tachyon' [t챈ki.�뭤], whose speed increases as its energy decreases theoretically, is thought to be the ideal of the real time application which runs in very constrained environment. 

## Motivation

+ Build modular architecture which supports both static linked(kernel mode) and dynamic-linked module(user mode).
+ Provides dynamic loading and dependency management of an application through network.
+ Provides secure runtime environment and fault isolation for user application using memory protection facility which is generally available in low-cost hardware.

## Features

 + provide runtime scalablility with various dynamic modules. (Not Supported Yet)
 + Provides application loading and debugging over the network. (Not Supported Yet)
 + Provides autonomous dependency (dynamic module) management over the network. (Not Supported Yet)
 + Support multi-threading 
    + The number of threads is limited only by available memory
    + Context-switch overhead is less than 5us in ARM CM4 (STM)
 + Preemptive scheduler with 6 execution priorities  
 + Various lightweight synchronization 
    + Monitor (mutex / condition variable)
    + Event (blocking pub / sub)  
    + Semaphore  
    + Barrier
 + Various Inter-thread communication 
    + Message Queue  
    + Mail Queue  
+ HAL (as static module) [UART / I2C / SPI / TIMER / RTC / GPIO]  

## Target Supported

 + STM32F40x (ARM Cortex-M4)
 + STM32F41x (ARM Cortex-M4)
 + STM32F20x (ARM Cortex-M3)

## Build Environment

 project is developed with eclipse IDE with ARM Cross compile tool (toolchain and plugin)

+ arm gcc cross compiler
   link : [GCC ARM Embedded in Launchpad] (https://launchpad.net/gcc-arm-embedded)

   ```shell
   $ sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa
   $ sudo apt-get update
   $ sudo apt-get install gcc-arm-embedded
   ```

+ Install clang to build dependent library
   > make sure clang / llvm-ar command is available
   ```shell
   $ sudo apt-install clang
   ```

+ python2.7 & pip for configuration utility (jconfigpy)

   ```shell
   $ sudo apt-get install python-dev python-pip  
   ```

+ gnu arm eclipse plug-in   (optional)
   link : [GNU ARM Eclipse Plug-ins ] (http://gnuarmeclipse.livius.net/blog/)
+ For Windows user, MinGW or Cygwin should be installed (might be included in GCC ARM toolchain installation)

## Build in Ubuntu

   1. get source code from repository
      ```shell
      $ git clone https://github.com/fritzprix/TachyOS.git
      $ cd TachyOS
      ```

   2. initiate project configuration and make your own configuration

       ```shell
       $ make config
       ```

   3. or you can use predefined configuration

      ```shell
       $ make config ARCH=ARM DEFCONF=stm32f4_def.conf
      ```

   4. build project

       ```shell
       $ make all
       ```

   5. (Optional) if you want reconfigure project
       ```shell
       $ make config_clean
       ```

## To-Do

+ Add SDIO HAL Implementation (~Apr.2016)
+ Add virtual file system   (~May.2016)

## License

 LGPL V3.0
