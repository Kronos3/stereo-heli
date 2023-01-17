## Fc

This Fc component will implement an MSPv2/iNAV compatible
driver to the flight controller. Telemetry will be downlinked
and waypoints and settings will be uplinked to the flight
controller.

The Heli FSW will manage the Fc but the Fc will perform the
actual flight operation. This component will also manage
the connection to a joystick via the bluetooth chip on the
Raspberry Pi.
