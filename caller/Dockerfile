FROM ubuntu:22.04
ENV TZ=Europe/Berlin
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN apt-get update && apt-get -y install sudo git wget cmake iputils-ping rpcbind iproute2 libc6-dev g++ libssl-dev build-essential autoconf libtool pkg-config clang cmake libboost-all-dev python3 #software-properties-common
#RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test
#RUN apt-get -y gcc-11

# Botan (AES)
RUN git clone https://github.com/randombit/botan.git /home/app/Botan
RUN cd /home/app/Botan
RUN python3 /home/app/Botan/configure.py --minimized-build --enable-modules=aes,cbc,auto_rng,hex --prefix="/usr/local/lib"
RUN sudo make && sudo make install

# SEAL (Homomorphic Encryption)
RUN git clone https://github.com/microsoft/SEAL.git /home/app/SEAL
#RUN cd /home/app/SEAL && git checkout 3.5.6 && git pull 
RUN cd /home/app/SEAL && git checkout 4.1.1 && git pull 
RUN cd /home/app/SEAL && cmake . -DBUILD_SHARED_LIBS=ON -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
RUN cd /home/app/SEAL && make && sudo make install

# LPCNet (Voice Encoder)
RUN git clone https://github.com/drowe67/LPCNet.git /home/app/LPCNet
RUN cd /home/app/LPCNet && mkdir build_linux && cd build_linux && cmake .. && make && make install

# RPClib (Communication)
RUN git clone https://github.com/rpclib/rpclib.git /home/app/rpclib
RUN cd /home/app/rpclib && cmake . && make && sudo make install
RUN mkdir -p /home/app/pirates
