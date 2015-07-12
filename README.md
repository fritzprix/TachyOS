#TachyOS 
##About
>This project is started by personal interest in real-time operating system. I have built this from scratch while enjoying various topics (scheduler, memory management, synchronization, etc.) in operating system. although this project is still its infant stage and required to be tested extensively. it functions normally for a few ported target.

###Motivation   
 + Build light weight kernel for low cost micro-controller based system which is deployed and also could maintained in remote place. 
 + Build system which supports dynamic loading and debugging of an application program from remote place (via network) 

###Features
 + Microkernel Architecture   
 + Support Multi-threading  
 + Support Memory Protection Unit  
 + Preemptive scheduler with 6 execution  priorities  
 + Various lightweight synchronization  
  + Monitor (mutex / condition variable)  
  + Event (blocking pub / sub)  
  + Semaphore  
  + Barrier (from [Modern Operating System](www.camden.rutgers.edu/~master/os/documents/slides/MOS-3e-02.pdf) by Andrew S. Tanenbaum)
 + Various Inter-thread communication  
  + Message Queue  
  + Mail Queue  
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
 + For Windows user, MinGW or Cygwin should be installed (might be included in GCC ARM toolchain installation) 









