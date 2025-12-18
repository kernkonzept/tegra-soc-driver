# tegra-soc drivers

Driver for NVIDIA Tegra
- BPMP (Boot and Power Management Processor)
- HSP (Hardware Synchronization Primitives)

This package contains of the following projects:
- *bpmp*': L4Re service for driving the BPMP firmware interface
- *libbpmp*: library used by the bpmp service and by the IO server
- *libpwm* tiny library for controlling the PWM fan controllers

The BPMP firmware interface is used to control many aspects of NVIDIA Tegra SoCs
including
- clocks,
- reset lines,
- power gates,
- temperature sensors,
- debugging,
- i2c interfaces,
- strap values (variables sampled by hardware units during startup),
- UPHY,
- and more.

The *bpmp* service multiplexes client requests to the BPMP firmware interface.
There is a per-client definition with white lists and black lists to manage
access to certain
- clocks,
- reset lines, and
- power gates.

This mechanism can be used to grant Linux clients (running as Uvmm instances)
access to many devices controlled by BPMP and preventing those clients to
power-off unused devices.


# Documentation

This package is part of the L4Re Operating System Framework. For
documentation and build instructions please refer to
[l4re.org](https://l4re.org).


# Contributions

We welcome contributions. Please see the
[contributors guide](https://l4re.org/contributing/).


# License

Detailed licensing and copyright information can be found in the
[LICENSE](LICENSE.spdx) file.
