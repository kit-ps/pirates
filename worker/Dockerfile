FROM ubuntu:22.04
ENV TZ=Europe/Berlin
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN apt-get update && apt-get -y install sudo git wget cmake iputils-ping rpcbind iproute2 libc6-dev g++ libssl-dev build-essential autoconf libtool pkg-config clang cmake libboost-all-dev
RUN git clone https://github.com/microsoft/SEAL.git /home/app/SEAL
RUN cd /home/app/SEAL && git checkout 4.1.1 && git pull 
RUN cd /home/app/SEAL && cmake . -DBUILD_SHARED_LIBS=ON -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
RUN cd /home/app/SEAL && make && sudo make install
RUN git clone https://github.com/rpclib/rpclib.git /home/app/rpclib
RUN cd /home/app/rpclib && cmake . && make && sudo make install
RUN mkdir -p /home/app/pirates