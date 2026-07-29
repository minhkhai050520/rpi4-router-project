# rpi4-router-project
This project builds a router image for Raspberry Pi 4 using Buildroot inside Docker.

## Usage

1. Make the helper script executable:

```bash
chmod +x ./build-container.sh
```

2. Build the Docker image and run the build inside the container:

```bash
./build-container.sh
```

The container will mount the repository into `/workspace` and execute `./build.sh` inside the container.

## Output

After a successful build, the SD card image will be available at:

```bash
buildroot/output/images/sdcard.img
```
