FROM ubuntu:xenial

RUN apt update

RUN apt install -y software-properties-common
RUN add-apt-repository ppa:mapmap/mapmap

RUN apt-get update
RUN apt-get install -y mapmap

CMD mapmap
