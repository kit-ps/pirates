FROM ubuntu:20.04
ENV TZ=Europe/Berlin
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN apt-get update && apt-get -y install sudo git wget cmake iputils-ping rpcbind iproute2 libc6-dev g++ libssl-dev build-essential autoconf libtool pkg-config clang cmake libboost-all-dev
RUN git clone https://github.com/drowe67/LPCNet.git /home/app/LPCNet
RUN cd /home/app/LPCNet && mkdir build_linux && cd build_linux && cmake .. && make && make install
#RUN mkdir -p /home/app/pirates
RUN /home/app/LPCNet/build_linux/src/lpcnet_enc /home/app/LPCNet/wav/david.wav /home/app/david.out
