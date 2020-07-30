#!/bin/sh
make reset
make config ARCH=ARM DEFCONF=stm32f4_def.conf
make release && make all