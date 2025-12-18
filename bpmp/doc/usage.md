# NVIDIA Tegra BPMP service {#l4re_servers_tegra_bpmp}

[comment]: # (This is a generated file. Do not change it.)
[comment]: # (Instead, change capdb.yml.)

The BPMP service is a driver for the NVIDIA Tegra BPMP firmware interface.


## Capabilities

* `vbus`

  Required for finding the devices which should be driven by this driver.

  The device with hid `tegra234-sysram` provides the shared memory used by the
  BPMP mailbox.

  The device with hid `tegra234-hsp` provides the Hardware Synchronization
  Primitives, namely the doorbell which is connected to the BPMP mailbox.

  Mandatory capability.

* `svr`

  Server Capability of application. Endpoint for IPC calls

  Mandatory capability.


## Command Line Options

* `-v`, `--verbose`

  Enable verbose mode. You can repeat this option to increase verbosity up to
  'trace5' level.

  Default log level is 'warn'. Passing this option once increases the level to
  'info'. Passing this option more than once increases the level further to
  'trace1', 'trace2', 'trace3', 'trace4' and 'trace5'.

  Can be used up to 6 times.

  Flag. True if provided.

* `-q`, `--quiet`

  This option enables the quiet mode. All output is silenced.

  Flag. True if provided.

## Building and Configuration

The BPMP service can be built using the L4Re build system. Just place this
project into your `pkg` directory. The resulting binary is called `tegra-bpmp`.

## Starting the device
```lua
local bpmp_channel = l:new_channel()
l:startv(
{
  caps = {
    vbus = linux_uses_bpmp_service and vbus_bpmp,
    svr = bpmp_channel:svr(),
  },
  log = { "bpmp-srv", "green", "keep", "show" },
},
"rom/tegra-bpmp",
"rom/bpmp.cfg",
"-v")
```
First, an IPC gate (`bpmp_channel`) is created which is used between the service
and the clients. The server-side is assigned to the mandatory `svr` capability
of the service.

A client connects with the server using the IPC gate by together with passing
the profile name.

The passed Lua config file (`bpmp.cfg` in this example) contains profiles for
all clients which need to connect to the server. A client without a profile is
not accepted. A client profile contains of black lists and white lists for
clocks, reset lines and power gates. For each of these resources, the following
rules apply:
  - If the black list and the white list are both empty, the client is **not**
  allowed to manipulate **any ID** of this resource.
  - If the white list contains the entry **"all"** then **all IDs** of this
  resource can be manipulated by the client, **except** IDs part of the black
  list.

So far there is a single client available -- the Uvmm proxy driver for the BPMP
device (`tegra_bpmp.cc`). The client driver expects the name of a capability as
part of the `bpmp` device in the Uvmm device tree:

  `l4vmm,bpmp = "bpmp"`;

The client driver tries to open a capability with the specified capability name
(in this exampe, "bpmp"). The capability object has to implement the vBus
interface (the interface of the IO server) and has to provide the following
devices:

  - `tegra234-hsp` -- the Tegra HSP device
  - `tegra234-sysram` -- the Tegra SYSRAM device
  - `L40009` -- the virtual interrupt controller

The vBus is implemented by the BPMP server implemented in this package.

## Virtual BPMP client

A svr capability has to be given to the BPMP service so that clients may be
created during runtime using the factory interface.

Call:   `create(0, "profile=<name>")`

* `"profile=<name>"`

  Set the profile name for this client. This name is used to lookup the black
  lists / white lists for clocks, reset lines and power gates this this client.

  String value.



