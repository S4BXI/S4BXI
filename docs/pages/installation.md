# Installation {#Installation}

S4BXI has few dependencies: it only requires SimGrid to be installed, and therefore the Boost library (because it's a dependency of SimGrid itself). To install SimGrid, see [its documentation](https://simgrid.org/doc/latest/Installing_SimGrid.html). S4BXI requires a very recent version of SimGrid, so you'll probably want to avoid the precompiled `apt` package, which can be a bit older than what we need.

The project can be configured / compiled / installed using cmake and make :

```bash
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/opt/s4bxi ..
make # You might want to add `-j N` if you have N cores on your CPU
make install # Probably do that as root
```

Or you can simply run `./rebuild.sh`, which should do all that automatically
