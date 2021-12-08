# S4BXI {#Home}

S4BXI is a simulator of the [Portals4 network API](https://cs.sandia.gov/Portals/). It is written using SimGrid's S4U interface, which provides a fast flow-model. More specifically, this simulator is tuned to model as best as possible Bull's hardware implementation of portals ([BXI interconnect](https://atos.net/produits/calcul-haute-performance-hpc/bxi-bull-exascale-interconnect))

This documentation covers:
- [Installation](@ref Installation): how to setup S4BXI and its dependencies
- [Usage](@ref Usage): how to run your Portals applications in simulation
- [High level APIS](@ref HighLevelAPIs): methodology to model higher-level APIs on top of Portals
- [Network model](@ref NetworkModel): details about the internal flow model for communications
