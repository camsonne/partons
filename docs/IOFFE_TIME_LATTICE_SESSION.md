# Ioffe-time distributions in PARTONS for lattice QCD data — session notes

Written for hand-over (e.g. to Claude in Slack or a colleague). It summarizes the
conversation that produced the `claude/ioffe-time-partons-tnxs0v` branches, the
design decisions, what was verified and what is still open.

Session: https://claude.ai/code/session_01N21X7ND9kKyfujaFzRRQQg

## 1. Request

> We want to implement Ioffe time in PARTONS so it can use LQCD data easily for
> global fits.

Lattice QCD does not measure PDFs or GPDs in x. It measures equal-time matrix
elements of quark bilinears separated by a space-like distance z, at hadron
momentum p. These depend on the Ioffe time ν = p·z and on z². After
renormalization (ratio to the rest-frame matrix element) they give the
*reduced pseudo-Ioffe-time distribution* 𝔐(ν, z²), related to the light-cone
Ioffe-time distribution (ITD)

    ℳ(ν, μ²) = ∫₋₁¹ dx e^{ixν} F(x, μ²)

by a one-loop matching kernel. To fit lattice data together with exclusive
data, PARTONS therefore needs to evaluate the ITD and the matched pseudo-ITD of
any GPD model at arbitrary (ν, z²).

## 2. Which repositories contain the work

| Repository | Branch | Status |
|---|---|---|
| `camsonne/partons` | `claude/ioffe-time-partons-tnxs0v` | **Core implementation** (commit `b93edca`) + this document |
| `camsonne/partons-example` | `claude/ioffe-time-partons-tnxs0v` | **Examples**: XML scenarios, C++ examples, lattice-data χ² example, toy data (commit `2187193`) |
| `camsonne/Partons-Pytorch-module` | — | untouched (natural place for the differentiable lattice channel of a global fit, see §7) |
| `camsonne/elementary-utils`, `camsonne/numa`, `camsonne/gepard`, `camsonne/ldrdgff` | — | untouched |

Everything needed to *compute* Ioffe-time quantities is in `partons`. Everything
needed to *see how to use it*, including against a lattice data file, is in
`partons-example`. No pull requests were opened.

## 3. Repository survey that drove the design

- PARTONS (v5.0.0) has a strict header/source mirror (`include/partons/...`,
  `src/partons/...`), sources picked up by `file(GLOB_RECURSE)`, C++17.
- Modules self-register with `BaseObjectRegistry::registerBaseObject(new X("X"))`
  in a static `classId`. Abstract module types need a `newXModule` pair in
  `ModuleObjectFactory`; services need a getter in `ServiceObjectRegistry`.
- The cleanest recent vertical slice to copy was `collinear_distribution`
  (kinematic, result, module, service, `KinematicUtils` file reader, batch-size
  property).
- CFF modules show how a module owns a `GPDModule` sub-module and integrates
  over x with NumA functors (`MathIntegratorModule`).
- There was no Ioffe/lattice/quasi/pseudo/Mellin code anywhere, no Fourier
  integrator, no fitting infrastructure and no database layer in the
  open-source tree.

## 4. Design

New vertical slice `ioffe_time`, mirroring `collinear_distribution`:

### Beans (`include/partons/beans/ioffe_time/`)
- `IoffeTimeKinematic(nu, z2, xi, t, MuF2, MuR2)`: ν dimensionless, z² in
  `GeVm2` (default) or `fm2`, ξ and t for generalized (off-forward) ITDs, μF²
  the MS-bar matching scale, μR² the scale of α_s. In XML, `z2`, `xi`, `t` are
  optional and default to 0. Class name for XML: `IoffeTimeKinematic`.
- `IoffeTimeDistributionResult`: for each GPD type two `PartonDistribution`
  objects (real and imaginary parts) so all flavors, the gluon and the
  singlet/non-singlet combinations are available. Helpers:
  `getQuarkDistribution(type, flavor)`, `getGluonDistribution(type)`,
  `getIsovector(type)` (u − d, the combination lattice groups publish).

### Modules (`include/partons/modules/ioffe_time/`)
- `IoffeTimeDistributionModule` (abstract, `ModuleObject` + `MathIntegratorModule`):
  holds the `GPDModule` sub-module (XML key `GPDModule`), performs
  ∫dx K(x) F(x) over [−1, 1] split at x = 0, ±ξ, with a per-call cache of GPD
  evaluations so the model is called once per x node for all partons, both
  components and the singlet/non-singlet pieces. Daughters define the kernel via
  `kernelRealPart(x, isQuark)` / `kernelImaginaryPart(x, isQuark)`.
  Default integrator: adaptive Gauss–Kronrod 21 (`GK21_ADAPTIVE`), changeable with
  the usual `integrator_type` parameter.
- `IoffeTimeDistributionLightCone`: K = e^{ixν}. Works for any ξ, t.
- `IoffeTimeDistributionPseudoNLO`: reduced pseudo-ITD with one-loop matching,

      𝔐(ν, z²) = ∫₀¹ du ℐ(uν, μ²) { δ(1−u) − (α_s C_F/2π) [ B(u) ln(z²μ² e^{2γ_E+1}/4) + D(u) ]₊ }

  with, for unpolarized quarks, B(u) = (1+u²)/(1−u), D(u) = 4 ln(1−u)/(1−u) − 2(1−u)
  (Radyushkin 2018; Izubuchi, Ji, Jin, Stewart, Zhao 2018; Joó et al. 2019).
  Helicity uses the same one-loop kernel; transversity uses B_T = 2u/(1−u),
  D_T = 4 ln(1−u)/(1−u) (HadStruc). Because the relation is linear in ℐ, the
  u-integral is moved into the x-kernel,

      K(w) = e^{iw} − (α_s C_F/2π) ∫₀¹ du [L B(u) + D(u)] (e^{iuw} − e^{iw}),  w = xν,

  so the GPD is still evaluated once per x node; kernel values are cached per x.
  Parameters: `alphaS` (fixed) or a `RunningAlphaStrongModule` sub-module
  (evaluated at μR²); `matchingKernel` = `auto` (from GPD type) | `unpolarized` |
  `helicity` | `transversity`. Requires ξ = 0 and z² > 0 (throws otherwise).
  Gluons are transformed at leading order only.

### Service, wiring, file input
- `IoffeTimeDistributionService`: `computeSingleKinematic`, `computeManyKinematic`
  (threaded, batched), `printResults`. Property `ioffe_time.service.batch.size`
  is optional (default 1000) so old `partons.properties` files keep working.
- `KinematicUtils::getIoffeTimeKinematicFromFile`: `nu|z2|xi|t|MuF2|MuR2`, one
  point per line, optional first line `#none|fm2|none|GeV2|GeV2|GeV2`.
- `ModuleObjectFactory::newIoffeTimeDistributionModule(...)`,
  `ServiceObjectRegistry::getIoffeTimeDistributionService()`.

### XML usage

```xml
<task service="IoffeTimeDistributionService" method="computeManyKinematic">
   <task_param type="GPDType"><param name="value" value="H" /></task_param>
   <kinematics type="IoffeTimeKinematic">
      <param name="file" value="path/to/kinematics_ioffe_time_with_units.csv" />
   </kinematics>
   <computation_configuration>
      <module type="IoffeTimeDistributionModule" name="IoffeTimeDistributionPseudoNLO">
         <param name="matchingKernel" value="auto" />
         <module type="GPDModule" name="GPDGK16"></module>
         <module type="RunningAlphaStrongModule" name="RunningAlphaStrongStandard"></module>
      </module>
   </computation_configuration>
</task>
<task service="IoffeTimeDistributionService" method="printResults"></task>
```

### Conventions to remember
- Real part of the forward unpolarized ITD = cosine transform of q − q̄
  (valence), imaginary part = sine transform of q + q̄, because F(x<0) = −q̄(−x).
- Results are not normalized. The reduced pseudo-ITD is normalized to 1 at ν = 0;
  for u − d this is automatic when the model satisfies the quark-number sum rule,
  for other combinations divide by the same combination at ν = 0.

## 5. Examples (`partons-example`)

- `data/examples/ioffe_time/computeSingleKinematicsForIoffeTimeDistribution.xml`
- `data/examples/ioffe_time/computeManyKinematicsForIoffeTimeDistribution.xml`
- `data/examples/ioffe_time/kinematics_ioffe_time.csv`, `..._with_units.csv`
- `data/examples/ioffe_time/lattice_reduced_pseudo_itd_toy.csv` — format
  `nu|z2[fm2]|Re|dRe|Im|dIm`; **toy numbers, not lattice results**.
- `src/examples.cpp`: `computeSingleKinematicsForIoffeTimeDistribution()`,
  `computeManyKinematicsForIoffeTimeDistribution()`,
  `compareIoffeTimeDistributionWithLatticeData()` (reads the data file, evaluates
  the matched model at the data kinematics, prints a table and the χ² of u − d).
  The last one is the building block for including lattice data in a global fit.
- `bin/partons.properties`: added `ioffe_time.service.batch.size = 1000`.

## 6. Verification done (and not done)

- The container had no Docker daemon and none of the PARTONS third-party
  libraries (Qt/SFML/CLN/GSL/APFEL++/LHAPDF), so **the library was not built and
  nothing was executed inside PARTONS**.
- All new and modified sources (core and examples) pass `g++ -std=c++17
  -fsyntax-only` with the project flags against the real PARTONS, ElementaryUtils
  and NumA++ headers (SFML mutex and LHAPDF headers stubbed).
- A standalone C++ check of the matching mathematics (toy valence PDF): the
  swapped-order kernel agrees with the direct plus-prescription u-convolution to
  1e-11 for ν = 1, 3, 6; ℳ(0) = 1 is preserved; the ⟨x⟩ moment is reproduced from
  the slope of the imaginary part; the pseudo-ITD decreases with z² at fixed ν,
  as seen on the lattice.

First thing to do on a machine with the PARTONS stack: build `partons` and
`partons-example` on the branch, run the two XML scenarios, then
`compareIoffeTimeDistributionWithLatticeData()`.

## 7. Open points and next steps

1. **Check the helicity and transversity kernels** in
   `src/partons/modules/ioffe_time/IoffeTimeDistributionPseudoNLO.cpp`
   (`kernelB`, `kernelD`) against the reference used by the data set you fit;
   they were written from memory of the HadStruc papers.
2. Gluon one-loop matching and the skewed (ξ ≠ 0) one-loop matching are not
   implemented (LO only for gluons; PseudoNLO refuses ξ ≠ 0).
3. Higher-twist O(z²Λ²) corrections are not modelled; add a nuisance term in the
   fit if needed.
4. PARTONS has no minimizer. For the global fit, add a lattice channel to
   `Partons-Pytorch-module`: an `IoffeTimeConvolution` analogous to
   `DVCSConvolution` (linear map with fixed Jacobian cos(x_i ν) w_i / sin(x_i ν) w_i
   on the Gauss–Legendre grid) and an `ObservableKind` for Re/Im pseudo-ITD in
   `ReplicaDataset`, with the matching kernel above applied to the grid weights.
5. Optionally open pull requests for both branches.
