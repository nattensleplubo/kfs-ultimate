FROM	debian:bullseye

RUN	apt-get update && apt-get install -y \
	build-essential \
	libncurses-dev \
	bison \
	flex \
	libssl-dev \
	libelf-dev \
	bc \
	wget \
	xz-utils \
	gcc-multilib \
	g++-multilib \
	qemu-system-x86 \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR	/kernel

CMD	["/bin/bash"]
