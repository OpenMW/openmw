# Build OpenMW using Docker

## Build Docker image

Replace `LINUX_VERSION` with the Linux distribution you wish to use.
```
docker build -f Dockerfile.LINUX_VERSION -t openmw.LINUX_VERSION .
```

## Build OpenMW using Docker

Labeling systems like SELinux require that proper labels are placed on volume content mounted into a container.
Without a label, the security system might prevent the processes running inside the container from using the content.
The Z option tells Docker to label the content with a private unshared label.
```
docker run -v /path/to/openmw:/openmw:Z -e NPROC=2 -it openmw.LINUX_VERSION
```
