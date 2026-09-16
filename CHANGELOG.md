**Unreleased**

*Updates:*
* optional LibTorch tensor backend (CMake option `PARTONS_WITH_TORCH`, off by default): DVCS coefficient functions factored into scalar-type templates (`DVCSCFFKernels.h`, used by `DVCSCFFStandard` with no change of results) and new `DVCSCFFTorch` module evaluating them as batched, differentiable tensor operations on a fixed tanh-sinh grid; check program `test/torch/dvcs_cff_torch_check`

------

**4.0**

*Updates:*
* update Docker files
* few minor updates

*Physics:*
* addition of GAM2 and DDVCS processes
* new and updateded modules for the evaluation of total cross-section

------

**3.0**

*Updates:*

* improve configuration with CMake 
   * Qt is version either Qt 4 or 5 can be used
   * printing of versions improved
   * automatic population of environment_configuration.dat file
* add new GPD types, namelly for transversity nad higher-twist GPDs
* add Docker configuration file with description
* remove MSTW files (now one can chose PDF with LHAPDF)

*Physics:*
* addition of new exclusive production channel - DVMP
* addition of objects for collinear distributions, including services, DB connection, evolution
* addition  of  APFEL interface
* addition  of  LHAPDF interface
* addition of  subtraction constant module related to Eur.Phys.J.C 81 (2021) 4, 300
* addition of GPD module related to Phys.Rev.D 103 (2021) 11, 114019 
* addtion of GPD model used in Phys. Lett. B 805 (2020) 135454
* addition of few observable modules, like for the evaluation of backward-forward asymmetry for TCS, or total cross-section for DVCS 
* addition to new runnung coupling and threshold modules, mostly related to APFEL

------

**2.0**

*Updates:*

* update of architecture of services
* update of DB scheme
* standardisation of names and methods
* scales and xi converter modules now process and full-kinematic dependent
* propagation of kinematics and GPD types to integration routines
* comparison methods updated

*Bug fixes:*

* smart pointers
* minor memory leaks corrected
* problem with calling GPD evolution fixed
* dispersion relation subtraction constant
* memory leak in threads corrected

*Features:*

* new printing
* possibility to run computations with only specific GPD types
* units
* code comments
* possibility to set database server port via properties file

*Physics:*

* addition of new exclusive production channel - TCS
* neural network library added
* DVCS CCF module added based on recent neural network analysis
* unused stuff removed
* new GPD module for a selection of single parton type

------

**1.0**

*Initial version*
