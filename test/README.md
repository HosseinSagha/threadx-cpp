# ThreadX Test App
# Minimum Requirements
* **[Git](https://gitforwindows.org/)**
* **[CMake](https://cmake.org/download/)**
* **[Ninja build system](https://github.com/ninja-build/ninja/releases)**
    * Add executable/app path to **PATH** environment variable.
* **[GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)**
    * Remove **blank spaces** from the installation folder e.g. **C:\Arm-GNU-Toolchain-arm-none-eabi\xx.y-relz**.
# Developement Hints
* Update **SYSTEM_CLOCK** value for corrosponding compiler in **tx_initialize_low_level.S**.
* For ARMv8-M architecture (m23, m33, m55 and m85), [based on the security mode](https://github.com/eclipse-threadx/rtos-docs/blob/main/rtos-docs/threadx/user-guide-armv8m/chapter2.md), define **TX_SINGLE_MODE_NON_SECURE** or **TX_SINGLE_MODE_SECURE** in **tx_user.in**.
# Build Command
```
$ build.sh [clean | init | update | master | pull]
```
# Submodules
```
$ build.sh init
$ build.sh master
$ build.sh pull
$ build.sh update
```
# Build
```
$ build.sh clean
$ build.sh
```
