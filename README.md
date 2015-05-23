#TachyOS 
##About
 >This project is started by personal interest in real-time operating system. I have built this from the scratch, enjoying various topics in operating system. although this project is still its infant stage and required to be tested extensively. it functions normally for a few ported target.
###Motivation   
 + Light weight & simple design for ease of use (minimize learning curve)   
 + Support dynamic loading of an application program   
 + Support remote debugging and profiling of system performance (distributed node device in mind)   
###Features
 + Microkernel Architecture   
 + Preemtive scheduler with 6 execution priority    
 + Various Lightweight Synchronization (POSIX monitor / event(similar to signal) / Semaphore   
 + Various Inter-thread communication (MsgQ / MailQ)   
 + HAL (UART / I2C / SPI / TIMER / RTC / GPIO)   


##Target Supported  
 + STM32F40x (ARM Cortex-M4)   
 + STM32F41x (ARM Cortex-M4)   
 + STM32F20x (ARM Cortex-M3)   

##Build Environment   
 project is developed with eclipse IDE with ARM Cross compile tool (toolchain and plugin)
 + arm gcc cross compiler  (ver: 4.9-2014q4 rel)    
   link : [GCC ARM Embedded in Launchpad] (https://launchpad.net/gcc-arm-embedded)   
 + gnu arm eclipse plug-in   
   link : [GNU ARM Eclipse Plug-ins ] (http://gnuarmeclipse.livius.net/blog/)   
 + For Windows user, MinGW or Cygwin should be installed (maybe included in GCC ARM toolchain installation)   
###How to build




##To-Do
> 
 + Implementing Memory Management in layered fashion with various MPU / MMU hardware support in mind  
 + Implementing lightweight alternative to newlibc, which provided with ARM gcc toolchain   
 + Documentation Work for project





