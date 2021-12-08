# High level APIs {#HighLevelAPIs}

## General methodology

While S4BXI is a simulator of Portals, [it is also suited to model higher-level APIs](https://hal.inria.fr/hal-03366573)
like MPI or OpenSHMEM. These APIs should be able to run unmodified in theory, but in practice it might require you to
slightly modify their initialization: indeed, they often exchange metadata about the job at startup, using tools like
PMIx for example. These tools are not reimplemented in S4BXI, and therefore any call to them should be replaced by
values obtained from simulation so that the API initializes properly. To help this process, the simulator exposes the
following primitives:

```C
uint32_t s4bxi_get_my_rank();
uint32_t s4bxi_get_rank_number();
char* s4bxi_get_hostname_from_rank(int rank);
int s4bxi_get_ptl_process_from_rank(int rank, ptl_process_t* out);
void s4bxi_set_ptl_process_for_rank(ptl_handle_ni_t ni);
void s4bxi_keyval_store_pointer(char* key, void* value);
void* s4bxi_keyval_fetch_pointer(int rank, char* key);
```

Other general-purpose helpers are also available from the simulated application, which can help improve the simulation's
performance or simply help debugging and logging

```C
// print functions which will add the identification of the current actor as a prefix
int s4bxi_fprintf(FILE* stream, const char* fmt, ...);
int s4bxi_printf(const char* fmt, ...);
// inject work into our computation model manually
void s4bxi_compute(double flops);
void s4bxi_compute_s(double seconds);
// number of seconds in the simulated world since the start of the simulation
double s4bxi_simtime();
// specify if the application is currently polling (for example on a PtlEQGet)
void s4bxi_set_polling(unsigned int p);
unsigned int s4bxi_is_polling();
// barrier on all actors that execute user code
void s4bxi_barrier();
// change log_level (requires a log folder to be specified, otherwise it's a no-op)
void s4bxi_set_loglevel(int l);
```

## A word on MPI simulation

While S4BXI can be used to model any API that runs on top of Portals, some features were developed to facilitate MPI
simulation in particular. More specifically, S4BXI provides an MPI middleware that intercepts calls to MPI primitives,
which allows having several MPI models and switching between them at runtime. The most obvious use-case of this is that
this middleware allows switching between a real-world MPI implementation (such as OpenMPI) and
[SMPI](https://hal.inria.fr/hal-01415484v2) to model network operations. The motivation behind running SMPI in
S4BXI is to allow some portions of applications to be simulated faster, even though this means that our Portals models
is completely bypassed (which lowers the accuracy of the simulation). 

This MPI middleware is not built by default: to enable it, two configuration parameters must be passed to CMake:
`BUILD_MPI_MIDDLEWARE` (set to `1`) and `SimGrid_SOURCE` (set to the location of SimGrid's sources, as we need to
include some of their private headers). To use it, simply set the environment `S4BXI_SMPI_IMPLEM` to `0` or `1` where
running `s4bximain` to specify if the SMPI model should be used by default, or the real-world MPI implementation. To
have more control about the model used (and potentially switch during the execution), the following function can be used
from the simulated application's code:

```C
void s4bxi_use_smpi_implem(int v);
```

The middleware also allows logging every MPI call, which can be useful to debug deadlocks in the application's code for
example. To use this feature, S4BXI needs to be built with the parameter `LOG_MPI_CALLS` set to 1 (this is a
compile-time parameter because allowing to switch dynamically at runtime is too costly for the simulation's perfomance)