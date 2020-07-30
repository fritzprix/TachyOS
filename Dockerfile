FROM	ubuntu:18.04


RUN	apt update
RUN apt install -y build-essential gcc-arm-none-eabi clang llvm git python-pip
RUN mkdir build
WORKDIR /build
CMD ["./ci_build.sh"]