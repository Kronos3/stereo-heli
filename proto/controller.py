import struct

BUTTON = 0x1
AXIS = 0x2
INIT = 0x80

# Filter out (type_, number_)
event_filter = {
}

mapping = {
    (BUTTON, 0): "X",
    (BUTTON, 1): "CIRCLE",
    (BUTTON, 2): "TRIANGLE",
    (BUTTON, 3): "SQUARE",
    (BUTTON, 4): "L1",
    (BUTTON, 5): "R1",
    (BUTTON, 6): "L2",
    (BUTTON, 7): "R2",
    (BUTTON, 8): "SELECT",
    (BUTTON, 9): "START",
    (BUTTON, 10): "PS",
    (BUTTON, 11): "L3",
    (BUTTON, 12): "R3",
    (AXIS, 0): "LX",
    (AXIS, 1): "LY",
    (AXIS, 2): "L2",
    (AXIS, 3): "RX",
    (AXIS, 4): "RY",
    (AXIS, 5): "R2",
    (AXIS, 6): "HORIZONTAL",
    (AXIS, 7): "VERTICAL",
}

with open("/dev/input/js0", "rb") as controller:
    while True:
        raw_event = controller.read(8)
        _, value, type_, number = struct.unpack("IhBB", raw_event)
        key = type_, number
        if key in event_filter:
            continue

        if type_ & BUTTON:
            print("B ", end='')
        if type_ & AXIS:
            print("A ", end='')
        if type_ & INIT:
            print("I ", end='')
            type_ &= ~INIT
            key = type_, number

        if key in mapping:
            print(f"{mapping[key]}, {value}")
        else:
            print(f"{number}, {value}")
