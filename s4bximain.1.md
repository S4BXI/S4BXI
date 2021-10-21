% S4BXIMAIN(1) s4bximain 0.1.0
% Julien EMMANUEL
% October 2021

# NAME

s4bximain - run an S4BXI simulation

# SYNOPSIS

**s4bximain** *PLATFORM* *DEPLOY* *APPLICATION* *APPLICATION_NAME* [*OPTIONS*]

# DESCRIPTION

**s4bximain** starts an S4BXI simulation of the provided *APPLICATION*, deployed using the configuration in the *DEPLOY* XML file, running on the cluster *PLATFORM*

# ARGUMENTS

*PLATFORM*
:   Platform file, describing the simulated cluster. It can use the XML or C++ format provided by SimGrid. In the case of a C++ platform, it should be compiled as a shared library beforehand (so the .so file should be specified)

*DEPLOY*
:   Deployment file, describing how actors should be instanciated on the simulated platform. This file is always in XML format, but it can be generated using the S4BXI config generator (to be executed with Deno)

*APPLICATION*
:   Path to the application to model, compiled in the form of a shared library. It doesn't necessarily needs to be linked with either S4BXI or SimGrid, as symbols don't need to be resolved at compile time for shared libraries, and they will be available at runtime because s4bximain is correctly linked with all the dependencies

*APPLICATION_NAME*
:   Name of the simulated application. The only use for this parameter is to forward it to `argv[0]` on each simulated process, so in most case its value doesn't matter

*OPTIONS*
:   The options available are the one supported by SimGrid (mostly in the form of configuration parameters: `--cfg=option:value`)