# CAN_LOGGER

## Description

This package provides a logging mechanism for CAN (Controller Area Network) channels. It is designed to read frames from a specific CAN channel, detect events, and log frames before and after the event for analysis. The package uses a state machine to handle different states of the logging process.

![diagram for can_logger](diagram.png)

## Installation

To install the package, run the following command:

```bash
catkin build can_logger
```

## Usage

To use the package, run the following command:

```bash
sudo ./devel/lib/can_logger/can_logger_node
```
## parameters

All parameters are set in the `config.yaml` file in config folder. The parameters are as follows:
```yaml
DURATION : 30                           # The duration of the logging process in seconds.
CAN_INTERFACE : "can0"                  # The CAN interface to be used.
CAN_HZ : 1000                           # The frequency of the CAN channel.

EVENT_FRAME:                            # The event frame to be detected.                                          
    CAN_ID : 1                          # The CAN ID of the event frame.
    SIGNAL:                             # The signal of the event frame.
        START_BIT: 0                    # The start bit of the signal.
        LENGTH: 8                       # The length of the signal.
        SCALE: 1.0                      # The scale of the signal.
        OFFSET: 0.0                     # The offset of the signal.
        VALUE: 1.0                      # The value of the signal.
``` 


