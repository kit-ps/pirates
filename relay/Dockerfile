FROM ubuntu:22.04
ENV TZ=Europe/Berlin
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN apt-get update && apt-get -y install sudo git wget cmake iputils-ping rpcbind iproute2 libc6-dev g++ libssl-dev build-essential autoconf libtool pkg-config clang cmake
RUN git clone https://github.com/rpclib/rpclib.git /home/app/rpclib
RUN cd /home/app/rpclib && cmake . && make && sudo make install
RUN mkdir -p /home/app/pirates