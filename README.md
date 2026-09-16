# PARTONS

## Documentation

See [partons.cea.fr](http://partons.cea.fr).

## Release

To checkout the released version, use the tag `v1.0` or the branch `release-v1` (replace `v1` for subsequent versions).

## Optional LibTorch backend

PARTONS builds and runs exactly as before by default and has no dependency on
LibTorch. The CMake option `PARTONS_WITH_TORCH` adds a tensor backend for the
DVCS Compton form factors:

```
cmake -DPARTONS_WITH_TORCH=ON -DCMAKE_PREFIX_PATH=/path/to/libtorch ..
```

(LibTorch 2.6 or later requires a C++20 compiler; the option switches the build
to C++20. Download LibTorch from [pytorch.org](https://pytorch.org/get-started/locally/)
or point `CMAKE_PREFIX_PATH` / `Torch_DIR` at the `torch` directory of a pip
installation.)

What it adds:

* `DVCSCFFKernels.h` holds the DVCS coefficient functions (LO and NLO quark and
  gluon kernels, subtraction constants) as templates on the scalar type.
  `DVCSCFFStandard` now evaluates them with `double`; this is the same code path
  as before and produces identical results.
* `DVCSCFFTorch` (only with the option) is a `DVCSConvolCoeffFunctionModule` that
  evaluates the same kernels on `torch::Tensor`: the x-convolution runs on a fixed
  double-exponential (tanh-sinh) quadrature grid as batched tensor algebra
  instead of NumA's adaptive integrator. It is registered like any module
  (`ModuleObjectFactory`, XML automation) and takes the same perturbative order
  parameter as `DVCSCFFStandard`, plus an optional `torch_quadrature_level`
  (default 5, i.e. 193 nodes on each of [0, xi] and [xi, 1]). Its static API
  (`makeGrid`, `sampleGPD`, `evaluate`) computes many kinematic points at once
  from GPD sample tensors, and the result is differentiable with respect to
  those samples (autograd), which is what is needed to fit a neural-network GPD
  through the PARTONS coefficient functions.
* `test/torch/dvcs_cff_torch_check` compares `DVCSCFFTorch` with
  `DVCSCFFStandard` on the GK16 model for H, E, Ht, Et at LO and NLO, checks the
  batched API against the module path, and checks autograd against finite
  differences (`ctest` runs it). Agreement is at the level of the adaptive
  integrator's own tolerance: about 1e-8 relative at LO and 1e-4 at NLO.

The rest of PARTONS (GPD models, evolution, observables) is unchanged and stays
in double precision; the tensor backend is confined to the DVCS CFF module.

## Licensing

Copyright (C) 2018  PARTONS team.

Under GPLv3. See [LICENSE](LICENSE) file.

## Contact

See [contact page](http://partons.cea.fr/partons/doc/html/contact.html).