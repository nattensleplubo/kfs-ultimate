FROM	ubuntu:22.04

ENV	DEBIAN_FRONTEND=noninteractive

RUN	apt-get update && apt-get install -y \
	build-essential \
	nasm \
	gcc \
	g++ \
	make \
	grub-pc-bin \
	grub-common \
	xorriso \
	mtools \
	qemu-system-x86 \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR	/build

CMD	["/bin/bash"]
