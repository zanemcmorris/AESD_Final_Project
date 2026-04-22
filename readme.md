# Wiki Page
The wiki page for this project can be found here [Project Overview](https://github.com/cu-ecen-aeld/final-project-zanemcmorris/wiki/Project-Overview)

## Repository Overview
This repo contains the server application for the NVME testing project. The main primary files that I've authored are in server/. I have included submodules for libparted and libnvme, but I only really make use of libparted at the moment. When we compile the Buildroot image the submodules are not included and instead use the pre-compiled modules packaged with Buildroot.

This program works on /dev/nvme0n1 if it is present on the system. Depending on the hardware, it may be the user's primary drive that contains all their information and operating system. For this reason, I have implemented a simple SAFE_MODE that checks whether the partition name starts with AESD or not. If it doesn't, then the function that can overwrite data is aborted and the user is alerted.

There are some outstanding bugs and implementation details that need refinement. For the most part, they are marked in the source code with TODO: comments. A key feature that is not behaving properly is creating a partition of 4096 sectors or more. Currently, the appliciation only allows for partitions of 1024, 2048, or 4096 sectors, rounding user input to the nearest one (only growing larger, not smaller).
