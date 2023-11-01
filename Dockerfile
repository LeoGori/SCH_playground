FROM ubuntu:focal
ARG DEBIAN_FRONTEND=noninteractive


# Install locales package
RUN apt-get update && apt-get install -y locales

# Uncomment the desired locale and generate it
RUN sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
    dpkg-reconfigure --frontend=noninteractive locales && \
    update-locale LANG=en_US.UTF-8

# Set environment variables for UTF-8 configuration
ENV LANG en_US.UTF-8  
ENV LANGUAGE en_US:en  
ENV LC_ALL en_US.UTF-8

RUN echo "rebuild n.4"
RUN apt-get update
RUN apt-get install -y wget git qemu-system qemu-utils python3 python3-pip \
        gcc libelf-dev libssl-dev bc flex bison vim bzip2 libncurses-dev cpio


# Download gdb and pwndbg
RUN apt-get install git -y
RUN apt-get install gdb -y
RUN git clone https://github.com/pwndbg/pwndbg
WORKDIR /pwndbg
RUN ./setup.sh

# Download kernel
RUN mkdir -p /sources
WORKDIR /sources
RUN wget https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/snapshot/linux-5.16-rc1.tar.gz
RUN tar xvzf linux-5.16-rc1.tar.gz
RUN mv linux-5.16-rc1 linux
RUN wget https://busybox.net/downloads/busybox-1.32.1.tar.bz2
RUN tar xvjf busybox-1.32.1.tar.bz2

# initial build, so as to speed up development
COPY ./scripts/build-k.sh /sources
RUN /sources/build-k.sh

WORKDIR /scripts
EXPOSE 5900
EXPOSE 6000

# Setting up a few tools for uefi demos
RUN apt-get install -y parted
RUN apt-get install -y dosfstools

