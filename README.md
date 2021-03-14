# S4BXI

[![ci](https://framagit.org/s4bxi/s4bxi/badges/master/pipeline.svg)](https://framagit.org/s4bxi/s4bxi/pipelines?scope=branches)

> *S4BXI: because your real platform is a real mess, and your simulator a faithful model*

*(Thanks Martin)*

Simulator written using SimGrid's S4U interface, which provides a model of the [Portals4 network API](https://cs.sandia.gov/Portals/). More specifically, it is tuned to model as best as possible Bull's hardware implementation of portals (BXI interconnect)

## Installation

You should have SimGrid installed before attempting to install S4BXI. For more infos on that, see [SimGrid's documentation](https://simgrid.org/doc/latest/Installing_SimGrid.html). S4BXI requires a very recent version of SimGrid, so you'll probably want to avoid the precompiled apt package, which can be a bit older than what we need.

The project can be configured / compiled / installed using cmake and make :

```shell
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/opt/s4bxi ..
make # You might want to add `-j N` if you have N cores on your CPU
make install # Probably do that as root
```

Or you can simply run `./rebuild.sh`, it should do all that automatically

## Usage

A simple CMakeLists.txt for an S4BXI project should look like 

```cmake
cmake_minimum_required(VERSION 3.9)

project(your_project)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(SimGrid REQUIRED)
find_package(S4BXI REQUIRED)

include_directories(${SimGrid_INCLUDE_DIR})
include_directories(${S4BXI_INCLUDE_DIR})

add_library(your_project SHARED your_main.cpp )
target_link_libraries(your_project ${S4BXI_LIBRARY})
```

SimGrid and S4BXI should find themselves automatically thanks to the dedicated cmake plugins if you installed them in standard locations (`/opt/simgrid` and `/opt/s4bxi`). If you installed them somewhere else, you can specify the path manually by setting the `SIMGRID_PATH` and `S4BXI_PATH` cmake variables (`-DSIMGRID_PATH=... -DS4BXI_PATH=...`)

In your code, there are two options :

- Include `<s4bxi/s4bxi.hpp>` and write you actors, which should extend `BxiMainActor` to be able to use the Portals4 API (S4U-like approach in SimGrid terms). In this case you can simply build your simulator as an executable instead of a shared library

- Build your Portals code (which should `#include <portals4.h>`) as a shared library (cf CMake conf above), and run it in **s4bximain** (which is in the **bin/** folder of your S4BXI installation) as follows : `s4bximain platform.xml deploy.xml ./libyour_project.so your_project [Simgrid options, such as --cfg=surf/precision:1e-9 for example]` (SMPI-like approach, which is the recommended for running unmodified Portals code)

## Modifications to S4U's workflow

The initiator doesn't use the target's mailbox directly to communicate, it has to issue its command to its *NIC*, which will wrap the message in a custom data structure, and send it to the target's *NIC*. The target's *NIC* will then process the message using the standard Portals4 workflow (match entry, write to MD, post event, send ACK, etc.).

The platform description file must be modified :
* Each actor is divided in four : a *main* actor, a *NIC TX* actor, a *NIC RX* actor and a *NIC_E2E* actor
* The links that used to connect main actors to the network now connect *NIC* actors to the rest of the network (usually to a router)
* Each *main actor* is now exclusively connected to its corresponding *NIC actors* through a fast link that models a PCIe 3.0 x16 port

The behaviour of the *main* actor is similar to the one of the former single actor (describes the application)

The behaviour of the *NIC* actors is defined by the wrapper and models Portals' behaviour using several daemon actors :
* RX actor is an infinite loop that listens to the network for incoming requests and processes it through the MD / EQ / etc. system
* TX actor is an infinite loop that listens to commands from the main actor and issues the corresponding requests to the network
* E2E actor is an infinite loop that listens to instructions from the TX actor : when it gets a message, it waits until the timeout is reached and then determines if the messages needs to be re-transmitted or not

## Mailboxes

To do all that, the actors share several mailboxes : 
* A *Request* mailbox for *RX*, which is targeted by other nodes when sending requests or responses (including ACKs)
* A *Command* mailbox for the main actor to issue instructions to be executed by *TX* (sometimes these commands can come from *RX*, to send a Portals ACK for example)
* An *E2E instructions* mailbox that allows the NIC Initiator (TX) to pass messages to the E2E actor
* Several *event* mailboxes that the main actor can query to get PUT, GET, SEND, ACK, REPLY, LINK etc. events. There is one of those mailboxes for each Portals EQ
* Several *CT* mailboxes that are used to wake up actors stuck on a `PtlCTPoll` (using a mechanism similar to EQs). There is not necessarily a mailbox for each CT, because they are needed only in case of a `PtlCTPoll` : `PtlCTGet` and `PtlCTWait` can be implemented in a more performant way (because counting events are simpler than full events, see Portals spec)

## New structure

![img](./s4bxi_arch.jpg)

## Performance / precision tradeoff

Several options allow the user to make faster simulations, by simplifying the model (and thus losing a bit of precision). These include (for each node) :

* __model_pci__ : If `false`, most (but not all, we still need to yield from times to times, which is good for both performance and precision) PCI communication are skipped. That determines wether we model inline messages and PIO / DMA difference. It can be set as a `<prop>` in the main actor of each node (default value is `true`)

* __e2e_off__ : If true, no re-transmit logic is executed. There is no specific prop to set, if you want it `true` simply put no E2E actor in your deploy for the desired node, otherwise it will be `false`
