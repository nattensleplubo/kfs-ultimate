# KFS Project: Compiling an i386 Kernel in Docker

## Introduction

This guide will walk you through setting up a Docker environment to compile a 32-bit (i386) Linux kernel for the 42 KFS (Kernel From Scratch) project. Using Docker ensures a consistent build environment regardless of your host system.

## Prerequisites

- Docker installed on your system
- Basic understanding of Docker concepts
- Your KFS project files ready

## Step 1: Create Your Dockerfile

Create a `Dockerfile` in your project directory:

```dockerfile
FROM debian:bullseye

# Install necessary packages for kernel compilation
RUN apt-get update && apt-get install -y \
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

# Set working directory
WORKDIR /kernel

# Keep container running
CMD ["/bin/bash"]
```

### What each package does:
- **build-essential**: Core compilation tools (gcc, make, etc.)
- **libncurses-dev**: For menuconfig interface
- **bison & flex**: Parser generators needed by kernel build
- **libssl-dev**: SSL library for kernel crypto
- **libelf-dev**: ELF object file access library
- **bc**: Basic calculator for kernel scripts
- **gcc-multilib & g++-multilib**: 32-bit compilation support
- **qemu-system-x86**: To test your kernel

## Step 2: Build Your Docker Image

```bash
docker build -t kfs-builder .
```

This creates an image named `kfs-builder` with all necessary tools.

## Step 3: Run Your Docker Container

### Option A: Interactive mode (recommended for learning)

```bash
docker run -it --rm -v $(pwd):/kernel kfs-builder
```

### Option B: With a specific name (easier to reconnect)

```bash
docker run -it --name kfs-build -v $(pwd):/kernel kfs-builder
```

### Understanding the flags:
- `-it`: Interactive terminal
- `--rm`: Remove container when you exit (Option A only)
- `--name kfs-build`: Give your container a name
- `-v $(pwd):/kernel`: Mount current directory to /kernel in container
- `kfs-builder`: The image to use

## Step 4: Download the Linux Kernel Source

Inside the Docker container:

```bash
# Download kernel version 5.10 (LTS - good for learning)
wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.10.tar.xz

# Extract
tar -xf linux-5.10.tar.xz
cd linux-5.10
```

## Step 5: Configure the Kernel for i386

### Option A: Use a minimal config (recommended for KFS)

```bash
# Start with minimal 32-bit configuration
make ARCH=i386 defconfig
```

### Option B: Use menuconfig for customization

```bash
# Interactive configuration menu
make ARCH=i386 menuconfig
```

### Important i386-specific settings to verify:

Navigate through menuconfig and ensure:
- **Processor type and features** → Processor family = 486 or higher
- **Binary Emulations** → IA32 Emulation (should be enabled)
- **General setup** → Keep kernel configuration minimal for learning

## Step 6: Compile the Kernel

```bash
# Compile with i386 architecture
# -j$(nproc) uses all available CPU cores
make ARCH=i386 -j$(nproc)
```

### What happens during compilation:
1. **Stage 1**: Generates configuration files
2. **Stage 2**: Compiles kernel source files (.c → .o)
3. **Stage 3**: Links object files into kernel image
4. **Stage 4**: Creates compressed kernel image (bzImage)

The compilation can take 10-30 minutes depending on your system.

## Step 7: Verify Your Compiled Kernel

After successful compilation, you'll find:

```bash
# The compressed kernel image
ls -lh arch/i386/boot/bzImage

# Or on some systems:
ls -lh arch/x86/boot/bzImage
```

## Step 8: Test Your Kernel with QEMU

### Create a minimal test:

```bash
# Run your kernel with QEMU (it will kernel panic without a rootfs, but proves it boots)
qemu-system-i386 -kernel arch/x86/boot/bzImage
```

You should see kernel boot messages. Press `Ctrl+A` then `X` to exit QEMU.

## Step 9: Create a Build Script

Create `build.sh` in your project root:

```bash
#!/bin/bash

set -e  # Exit on error

KERNEL_VERSION="5.10"
ARCH="i386"

echo "=== KFS Kernel Build Script ==="
echo "Kernel: Linux ${KERNEL_VERSION}"
echo "Architecture: ${ARCH}"
echo ""

# Check if kernel source exists
if [ ! -d "linux-${KERNEL_VERSION}" ]; then
    echo "Downloading kernel source..."
    wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-${KERNEL_VERSION}.tar.xz
    tar -xf linux-${KERNEL_VERSION}.tar.xz
fi

cd linux-${KERNEL_VERSION}

echo "Configuring kernel..."
make ARCH=${ARCH} defconfig

echo "Building kernel..."
make ARCH=${ARCH} -j$(nproc)

echo ""
echo "=== Build Complete! ==="
echo "Kernel image: arch/x86/boot/bzImage"
ls -lh arch/x86/boot/bzImage
```

Make it executable:
```bash
chmod +x build.sh
```

## Step 10: Using Your Build Script in Docker

```bash
# From your host machine:
docker run -it --rm -v $(pwd):/kernel kfs-builder ./build.sh
```

## Common Issues and Solutions

### Issue 1: "No rule to make target 'debian/canonical-certs.pem'"
**Solution**: Disable certificate signing in config
```bash
scripts/config --disable SYSTEM_TRUSTED_KEYS
scripts/config --disable SYSTEM_REVOCATION_KEYS
make ARCH=i386 olddefconfig
```

### Issue 2: 32-bit compilation fails
**Solution**: Ensure multilib packages are installed
```bash
apt-get install gcc-multilib g++-multilib
```

### Issue 3: BTF generation errors
**Solution**: Disable BTF
```bash
scripts/config --disable DEBUG_INFO_BTF
make ARCH=i386 olddefconfig
```

## Advanced: Creating a Custom Config

For KFS, you might want a minimal kernel. Create `.config` manually:

```bash
# Start with minimal config
make ARCH=i386 allnoconfig

# Enable only what you need
make ARCH=i386 menuconfig
```

Essential options to enable:
- 64-bit kernel → **NO** (for i386)
- Processor family → Select appropriate
- ELF binary support → **YES**
- /proc file system → **YES**
- sysfs file system → **YES**

## Docker Tips for KFS Development

### Reconnecting to a named container:
```bash
docker start -ai kfs-build
```

### Copying files from container:
```bash
docker cp kfs-build:/kernel/linux-5.10/arch/x86/boot/bzImage ./my-kernel
```

### Cleaning up:
```bash
# Remove stopped containers
docker container prune

# Remove unused images
docker image prune
```

## Understanding the i386 Architecture

**i386** refers to:
- 32-bit x86 architecture
- Compatible with Intel 80386 and later processors
- Uses 32-bit addressing (4GB memory limit)
- Different calling conventions than x86_64

Your compiled kernel will be **32-bit only** and requires:
- 32-bit bootloader
- 32-bit userspace programs
- QEMU with `-m32` or `qemu-system-i386`

## Next Steps for KFS Project

1. **Study the kernel build process**: Understand Makefiles and Kbuild
2. **Modify kernel source**: Add your own syscalls or modules
3. **Create an initramfs**: Minimal root filesystem for testing
4. **Boot with your modifications**: Test in QEMU
5. **Document everything**: For your 42 evaluation

## Useful Commands Reference

```bash
# Clean kernel build
make ARCH=i386 clean

# Complete clean (including config)
make ARCH=i386 mrproper

# Show kernel version
make ARCH=i386 kernelversion

# Show all configuration options
make ARCH=i386 help

# Quick rebuild after changes
make ARCH=i386 -j$(nproc)
```

## Resources

- Linux Kernel Documentation: https://www.kernel.org/doc/html/latest/
- Kernel Newbies: https://kernelnewbies.org/
- KernelBuild: https://www.kernel.org/doc/Documentation/kbuild/

---

**Good luck with your KFS project!** Remember, understanding the build process is just as important as the final kernel. Take time to explore the Makefiles and understand each step.
