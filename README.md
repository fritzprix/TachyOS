# TachyOS 
[![Build Status](https://drone.io/github.com/fritzprix/tachyos/status.png)](https://drone.io/github.com/fritzprix/tachyos/latest)
## About
> TachyOS is the RTOS based on microkernel architecture which includes only minimal components like thread / synchronization, memory management, inter-thread communication while supporting execution context / address space isolation(protection) and extensible modular interface.  

### Motivation   
 + Build modular architecture which supports both static linked(kernel mode) and dynamic-linked module(user mode). 
 + Provides dynamic loading and dependency management of user application.
 + Provides secure runtime environment for user application.

### Features
 + Support Multi-threading with separate address space (by MPU)
 + Preemptive scheduler with 6 execution priorities  
 + Various lightweight synchronization  
  + Monitor (mutex / condition variable)  
  + Event (blocking pub / sub)  
  + Semaphore  
  + Barrier (from [Modern Operating System](www.camden.rutgers.edu/~master/os/documents/slides/MOS-3e-02.pdf) by Andrew S. Tanenbaum)
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
 + arm gcc cross compiler  (ver: 4.9-2014q4 rel)    
   link : [GCC ARM Embedded in Launchpad] (https://launchpad.net/gcc-arm-embedded)   
 + gnu arm eclipse plug-in   
   link : [GNU ARM Eclipse Plug-ins ] (http://gnuarmeclipse.livius.net/blog/)   
 + For Windows user, MinGW or Cygwin should be installed (might be included in GCC ARM toolchain installation)
 
## License 
 LGPL V3.0 









