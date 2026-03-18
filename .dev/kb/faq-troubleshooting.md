# FAQ & Troubleshooting

## Programming Issues

### Serial connection fails or programming unsuccessful
Hold the **BOOT button** while powering on to enter download mode, then retry.

### Program uploads but won't execute
Verify that the example program version matches your product version (board variant).

## Development Environment

### VS Code environment configuration problems
Try switching to a different network connection — connectivity issues often cause setup failures during package downloads.

### Arduino IDE compilation errors
Ensure the **Tools** menu is properly configured for your device (board, port, partition scheme, etc.).

### First compilation takes a very long time
This is expected. The first build compiles all dependencies. Subsequent builds are incremental and much faster.

## System

### AppData folder not visible (Windows)
Enable hidden items: File Explorer → View → check "Hidden items".

## Hardware

### Battery standby duration
In clock mode, a **1500mAh battery** can operate for **more than 15 days**.

## Serial Port Identification

### Windows
- Device Manager: `Win+R` → `devmgmt.msc`
- Command Prompt: `mode`

### Linux
- `dmesg | grep tty`
- `ls /dev/ttyUSB*`
- `ls /dev/ttyS*`

### macOS
- `ls /dev/cu.*`
- `ls /dev/tty.*`
