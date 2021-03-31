# Usage {#Usage}

*The full example presented here for compilation and execution is available in [the S4BXI git repository](https://framagit.org/s4bxi/s4bxi/-/tree/master/examples)*

## Preparing your application for simulation

S4BXI is able to [run unmodified scientific applications](https://hal.inria.fr/hal-02972297) that use Portals for network communications. It can also be used to run unmodified MPI applications, using the Portals BTL of OpenMPI, with only a few modifications to MPI's initialisation (to correctly setup ranks).

The only requirement is to compile your applications as shared libraries (instead of regular executable binaries), and to add our simulator to your include paths. Internally making a shared library allows the simulator to fetch symbols from your application (using `dlopen`) and run them in a controlled environment. There are two ways to do this: either manually in your build system, or using our own compilers (which are simply wrappers around your usual compiler):

- **Manual method:** usually this is just a few flag to add somewhere in your build toolchain, for example `-shared` and `-I <S4BXI path>/include` for GCC, or `SHARED` and `include_directories(<S4BXI path>/include)` for CMake. Note that you don't even need to link with Portals/S4BXI: since you are building a shared library, all symbols are not required to be resolved at compile time, and Portals' API will automatically be available when running the simulator.

- **Automatic method using our compilers:** this is still experimental, but it works very similarly to SMPI's compilers: you simply need to replace your usual compiler by `s4bxicc` or `s4bxicxx` (to compile C or C++), which should add the required flags automatically. These scripts might not be recognized as valid compilers by autoconf or CMake (because they only produce shared libs instead of executable binaries), in which case you need to set the `S4BXI_PRETEND_CC` environment variable **at configuration time only** (for more explanations see [SMPI's documentation](https://simgrid.org/doc/latest/app_smpi.html#troubleshooting-with-smpi)).

Similarly to SimGrid, S4BXI provides a CMake module, which can be found [here](https://framagit.org/s4bxi/s4bxi-cmake-modules). This module allows S4BXI to find itself automatically if you installed it in the standard location (`/opt/s4bxi`). If you installed it somewhere else, you can specify the path manually by setting the `S4BXI_PATH` cmake variable (`-DS4BXI_PATH=...`). If you use this module, compiling a simple `hello.cpp` program for simulation could look like this:

```cmake
cmake_minimum_required(VERSION 3.9)

project(hello)

# We assume the CMake modules are in your source_directory/cmake/Modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(S4BXI REQUIRED)

include_directories(${S4BXI_INCLUDE_DIR}) # portals4.h lives in this directory

add_library(hello SHARED hello.cpp)

# target_link_libraries(hello ${S4BXI_LIBRARY})
# └──> Linking is optionnal: S4BXI will be available at runtime automatically
```

## Running a simulation

A simulation has three inputs: 

- A platform, which is an XML file describing the cluster on which you want your application to run

- A deployment, which is also an XML file, and which describes which actors you want to run on each machine of your cluster

- Your application, compiled as a shared library

The entry point is the binary `s4bximain`, which has been installed in `<S4BXI path>/bin` when you built S4BXI, therefore the complete command to run a simulation looks like this (for the `hello` program compiled in the previous example):

```bash
s4bximain ./platform.xml ./deploy.xml ./libhello.so hello
```

(The last parameter is the name of your application, which will be passed to your main in `argv[0]` in case you need it)

**Note:** our simulator will need access to S4BXI's and SimGrid's libraries, so you should make sure that they are in your library path. For example if they are installed in standard locations: `LD_LIBRARY_PATH=/opt/s4bxi/lib:/opt/simgrid/lib`

### A word on XML configuration files

Although XML inputs are simply [regular SimGrid's](https://simgrid.org/doc/latest/platform.html) [configuration files](https://simgrid.org/doc/latest/Deploying_your_Application.html), S4BXI adds a few requirements (because our pre-defined actors expect a specific description of each machine). 

For this reason, it's usually easier to use our scripts to generate configuration files automatically. This is still a work in progress, and all this configuration might change radically in the near future because of evolutions in SimGrid itself, but our configuration generator is available [on Framagit](https://framagit.org/s4bxi/s4bxi-config-generator).

If these scripts don't work out for you (or you don't like TypeScript utilities), you can also write/generate configuration files manually. You can find examples in our test suite ([simple platform](https://framagit.org/s4bxi/s4bxi/-/blob/master/teshsuite/platforms/quito.xml) ; [client-server deployment](https://framagit.org/s4bxi/s4bxi/-/blob/master/teshsuite/deploys/quito_client_server_real_memory.xml)). The main requirement are:

---

_**Platform:**_ Each machine of your cluster should consist of: 

- one or more *CPU hosts*, for example
```xml
<host id="machine0" speed="10Gf"/>
```

- a PCI cable, which can be represented as two cable for more accuracy, for example:
```xml
<link id="machine0_PCI_FAT" bandwidth="10GBps" latency="0ns" sharing_policy="FATPIPE"/>
<link id="machine0_PCI" bandwidth="15.75GBps" latency="250ns"/>
```

- a *NIC host* with an optionnal *NID* property, for example 
```xml
<host id="machine0_NIC" speed="1Gf"/>
```

- a network cable, for example 
```xml
<link id="machine0_BXI" bandwidth="10GBps" latency="500ns"/>
```

- the associated routing, so that all these hosts communicate together (and communicate with the outside world), for example (if we have a `<router>` named *router0* in the platform)
```xml
<route src="machine0" dst="machine0_NIC">
    <link_ctn id="machine0_PCI_FAT" />
    <link_ctn id="machine0_PCI" />
</route>
<route src="machine0_NIC" dst="router0">
    <link_ctn id="machine0_BXI"/>
</route>
```

So with everything wrapped together, an example platform with only one machine connected to a router would look like:

```xml
<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
<platform version="4.1">
    <config>
        <prop id="network/model" value="CM02" />
        <prop id="network/loopback-lat" value="0.000000001" />
        <prop id="network/loopback-bw" value="99000000000" />
    </config>
    <zone id="AS0" routing="Floyd">
        <!-- Hosts -->
        <host id="machine0" speed="10Gf"/>
        <host id="machine0_NIC" speed="1Gf">
            <prop id="nid" value="42"/>
        </host>

        <!-- A single switch/router -->
        <router id="router0"/>

        <!-- PCI cable -->
        <link id="machine0_PCI_FAT" bandwidth="10GBps" latency="0ns" sharing_policy="FATPIPE"/>
        <link id="machine0_PCI" bandwidth="15.75GBps" latency="250ns"/>
        <!-- Network cable -->
        <link id="machine0_BXI" bandwidth="10GBps" latency="500ns"/>

        <!-- Wiring (linking cables with hosts and routers) -->
        <route src="machine0" dst="machine0_NIC">
            <link_ctn id="machine0_PCI_FAT" />
            <link_ctn id="machine0_PCI" />
        </route>
        <route src="machine0_NIC" dst="router0">
            <link_ctn id="machine0_BXI"/>
        </route>
    </zone>
</platform>
```

(Note that we set some global configuration at the top of the file. This isn't mandatory, but these parameters are strongly recommended, especially the CM02 network model. For more infos on this see [SimGrid's documentation](https://simgrid.org/doc/latest/Configuring_SimGrid.html))

---

_**Deployment:**_ Each machine of your cluster should run the following actors:

- One or more *user_app* actor: these represent your application and should run on the CPU host(s)

- One or more *nic_initiator* and *nic_target* actors: there should be at least one of each on each NIC host, but you can add more to model parallel-processing capacities of your hardware (if it has any). These actor should have the "VN" property set (to differentiate several *Virtual Networks*). Valid VN values are 0 and 1 for targets, and 2 and 3 for initiator. If this notion of VN makes no sense to you, always use **1** for targets and **3** for initiators

- **Optionnal:** a (preferably single) *nic_e2e* actor on the NIC host, dedicated to the end-to-end reliability processing (retransmission of message after a timeout). If this actor is not present, this processing will be disabled automatically (but please be consistent in all your cluster: include it everywhere or nowhere)

So for example, the deployment for *machine0* in our previous example could look like:

```xml
<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
<platform version="4.1">
    <actor host="machine0" function="user_app"/>
    <actor host="machine0_NIC" function="nic_initiator"/>
        <prop id="VN" value="3"/>
    </actor>
    <actor host="machine0_NIC" function="nic_target">
        <prop id="VN" value="1"/>
    </actor>
    <actor host="machine0_NIC" function="nic_e2e"/>
</platform>
```

## Options of the simulator

Several options can be passed in the form of environment variables to modify the behaviour of the simulator. These include:

### E2E 

Timeout and retries can be configured using `S4BXI_MAX_RETRIES` (*default=5*) and `S4BXI_RETRY_TIMEOUT` (*default=10.0*) environment variables (the timeout is in seconds and can be floating point).

E2E processing can be globally disabled using `S4BXI_E2E_OFF` (*default=false*). In earlier versions E2E used to cause a huge drop in the performance in the simulator, which is the reason why this option was added, but nowadays the overhead is usually not that big.

### Precision/Speed tradeoff

Some options can speed up the simulation at the cost of some accuracy:

- `S4BXI_MODEL_PCI_COMMANDS`: if `true` then a small PCI transfer is added in simulation for each command that is sent to a NIC (*default=true*)

- `S4BXI_QUICK_ACKS`: if `true` then NICs that receive a Put (or Atomic) request will trigger a Portals ACK event at initiator side without sending any actual ACK message on the network (thanks to simulated world's magic), so it saves 1 or 2 small message transfers per Put (or Atomic) operation (1 if E2E is disabled, 2 otherwise, because of the BXI ack)

- `S4BXI_MAX_MEMCPY`: if set to a positive value **N**, only **N** bytes of payload will be copied from an incomming message into the corresponding buffer (MD or LE/ME buffer) when doing Portals operations (Put, Get, etc.). Obviously this could break the application being simulated, but if messages' payload are not important for the execution flow of the program this can speed up the simulation a little bit (*default=-1*)

### CPU modeling

There is no detailed CPU model in the simulator, and computations are modeled in a way that is extremely similar to SMPI: the compute time between network operations is measured **on the real physical machine that is running the simulation**, and then injected in the simulated world. These benchmarked computation times can be multiplied by a factor which corresponds to the variable `S4BXI_CPU_FACTOR` (*default=1*). The smallest computation can be ignored (i.e. not injected in the simulation) using the variable `S4BXI_CPU_THRESHOLD` (*default=1e-7*), which is defines a threshold (in seconds) under which computations are ignored

### Other

S4BXI can generate logs of various events (Network operation, PCI transfers, computations, etc.) in CSV format. To turn on this feature, simply specify `S4BXI_LOG_FOLDER` (*default="/dev/null"*) and CSV files will be generated in this directory (the files are split each 10000 operations). The log files can then be vizualized using our [web viewer](https://s4bxi.julien-emmanuel.com/log-viewer/)

S4BXI generates temporary files at startup, which are deleted very quickly (before your code starts to run). To keep this files, set `S4BXI_KEEP_TEMPS` (*default=false*) to `true`. This should really only be used when debugging the internals of S4BXI

Because the simulation is single-threaded, all actors run in the same process, which causes problems because each actor running your application should have its global variables privatized (since in a real cluster each actor would correspond to a different process, possibly running on a different machine than the others). S4BXI uses the same technique as SMPI to privatize variables (which consists in copies of libraries on disk to trick the linker). If you also need some shared libraries to be privatized (and not just global variables), you can specify them in `S4BXI_PRIVATIZE_LIBS` (*default=""*). For example if you want to simulate an OpenMPI app, you probably want to set `S4BXI_PRIVATIZE_LIBS="libmpi.so.40;libopen-rte.so.40;libopen-pal.so.40"` so that each actor running the application gets its own private copy of the OpenMPI runtime