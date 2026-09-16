#ifndef DVCS_CFF_TORCH_H
#define DVCS_CFF_TORCH_H

/**
 * @file DVCSCFFTorch.h
 * @author PARTONS LibTorch backend
 *
 * DVCS CFFs computed with the same coefficient functions as DVCSCFFStandard
 * (DVCSCFFKernels.h) on a fixed double-exponential (tanh-sinh) quadrature
 * grid, as batched torch tensor algebra. Only built with the CMake option
 * PARTONS_WITH_TORCH; the default PARTONS build does not depend on LibTorch and
 * does not contain this class.
 */

#ifdef PARTONS_WITH_TORCH

#include <torch/torch.h>

#include <complex>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../../../beans/automation/BaseObjectData.h"
#include "../../../beans/gpd/GPDType.h"
#include "../../../beans/PerturbativeQCDOrderType.h"
#include "DVCSConvolCoeffFunctionModule.h"

namespace PARTONS {
class GPDModule;
class PartonDistribution;
class RunningAlphaStrongModule;
} /* namespace PARTONS */

namespace PARTONS {

/**
 * @class DVCSCFFTorch
 *
 * @brief DVCS CFFs at LO and NLO on a fixed tanh-sinh grid, as tensors.
 *
 * Same physics as DVCSCFFStandard (identical kernels and subtraction
 * constants, see DVCSCFFKernels.h) and the same double-exponential quadrature
 * scheme as NumA's DExpIntegrator1D, except that the grid is frozen at one
 * refinement level instead of adapted to the integrand. That trade turns the
 * x-convolution into a weighted sum -- for many kinematic points at once a
 * batched matrix product -- with two consequences the adaptive scheme cannot
 * offer:
 *
 *  - it runs as one tensor operation over all points and nodes (CPU or GPU),
 *  - it is differentiable end to end w.r.t. the GPD samples (autograd), which
 *    is what a neural-network GPD needs to be fitted through the real PARTONS
 *    coefficient functions.
 *
 * Two ways to use it:
 *
 *  1. As an ordinary PARTONS module (through DVCSConvolCoeffFunctionService,
 *     XML automation, or compute()): identical interface to DVCSCFFStandard,
 *     configured with the same perturbative order parameter plus, optionally,
 *     PARAMETER_NAME_QUADRATURE_LEVEL. The GPD module is sampled at the grid
 *     nodes through the normal double-precision GPDModule::compute(). Results
 *     agree with DVCSCFFStandard to the quadrature tolerance (see
 *     test/torch/dvcs_cff_torch_check.cpp).
 *
 *  2. Through the static batched API (makeGrid / sampleGPD / evaluate): the
 *     GPD samples are tensors -- from a GPD module, or straight from a network
 *     with requires_grad -- and the result is a pair of {P} tensors {Re, Im}
 *     for P kinematic points, connected to the samples' autograd graph.
 *
 * Quark and gluon GPD inputs follow DVCSCFFStandard: the quark sample at x is
 * the charge-averaged singlet combination sum_q e_q^2 GPD_q^(+)(x)
 * (QuarkDistribution::getQuarkDistributionPlus, whose sign convention differs
 * between the vector and the axial GPDs and is set by the GPD module), the
 * gluon sample is 2 GPD_g(x).
 */
class DVCSCFFTorch: public DVCSConvolCoeffFunctionModule {

public:

    static const unsigned int classId; ///< Unique ID to automatically register the class in the registry.

    /**
     * Name of the parameter to set the tanh-sinh refinement level via
     * configure() or XML (step h = 2^-level, nodes t = k h for |t| <= 3, so
     * 2 * 3 * 2^level + 1 nodes per sub-interval). Default: 5, i.e. 193 nodes
     * on each of [0, xi] and [xi, 1].
     */
    static const std::string PARAMETER_NAME_QUADRATURE_LEVEL;

    /**
     * Constructor.
     * See BaseObject::BaseObject and ModuleObject::ModuleObject for more details.
     * @param className Name of last child class.
     */
    DVCSCFFTorch(const std::string &className);

    virtual DVCSCFFTorch* clone() const;

    /**
     * Destructor.
     */
    virtual ~DVCSCFFTorch();

    virtual void resolveObjectDependencies();
    virtual void configure(const ElemUtils::Parameters &parameters);
    virtual void prepareSubModules(
            const std::map<std::string, BaseObjectData>& subModulesData);

    // ##### BATCHED TENSOR API #####

    /**
     * The fixed quadrature: nodes and weights on [0, xi] (x1, w1) and on
     * [xi, 1] (x2, w2), each of shape {P, N} for P skewnesses.
     */
    struct Grid {
        torch::Tensor x1;
        torch::Tensor w1;
        torch::Tensor x2;
        torch::Tensor w2;
    };

    /**
     * GPD samples on a Grid: charge-averaged quark singlet combination and
     * 2 * gluon at the nodes ({P, N}) and at the diagonal x = xi ({P}).
     * Gluon tensors may be left undefined for an LO evaluation.
     */
    struct Samples {
        torch::Tensor quark1;
        torch::Tensor quark2;
        torch::Tensor quarkDiagonal;
        torch::Tensor gluon1;
        torch::Tensor gluon2;
        torch::Tensor gluonDiagonal;
    };

    /**
     * Per-point kinematic scalars, each {P}: skewness, alpha_s( muR^2 ) /
     * ( 2 pi ) and log( Q^2 / muF^2 ) (both unused at LO), and the number of
     * active flavours.
     */
    struct Scalars {
        torch::Tensor xi;
        torch::Tensor alphaSOver2Pi;
        torch::Tensor logQ2OverMu2;
        unsigned int nf;
    };

    /**
     * Build the tanh-sinh grid for skewnesses xi ({P}, double) at the given
     * refinement level. Nodes are strictly inside the sub-intervals.
     */
    static Grid makeGrid(const torch::Tensor& xi, unsigned int level);

    /**
     * Sample a GPD module on a grid: the P kinematic points are given as
     * parallel vectors, and the sampling goes through GPDModule::compute() at
     * each node (double precision, one call per node -- this is the boundary
     * with the rest of PARTONS). The returned tensors do not carry gradients.
     */
    static Samples sampleGPD(GPDModule* pGPDModule, const Grid& grid,
            const std::vector<double>& xi, const std::vector<double>& t,
            const std::vector<double>& MuF2, const std::vector<double>& MuR2,
            GPDType::Type gpdType);

    /**
     * The convolution: eqs. (8), (9) of the reference on the fixed grid.
     * Returns {Re, Im}, each {P}, differentiable w.r.t. the sample tensors.
     * @param polarized Axial (Ht, Et) instead of vector (H, E) kernels.
     */
    static std::pair<torch::Tensor, torch::Tensor> evaluate(const Grid& grid,
            const Samples& samples, const Scalars& scalars,
            PerturbativeQCDOrderType::Type qcdOrder, bool polarized);

    /**
     * Charge-averaged quark singlet combination of a PartonDistribution, as in
     * DVCSCFFStandard: sum over u, d, s of e_q^2 * GPD_q^(+).
     */
    static double squareChargeAveragedGPD(
            const PartonDistribution& partonDistribution);

    // ##### GETTERS & SETTERS #####

    RunningAlphaStrongModule* getRunningAlphaStrongModule() const;
    void setRunningAlphaStrongModule(
            RunningAlphaStrongModule* pRunningAlphaStrongModule);

    unsigned int getQuadratureLevel() const;
    void setQuadratureLevel(unsigned int level);

protected:

    /**
     * Copy constructor.
     * @param other Object to be copied.
     */
    DVCSCFFTorch(const DVCSCFFTorch &other);

    virtual void initModule();
    virtual void isModuleWellConfigured();

    virtual std::complex<double> computeUnpolarized();
    virtual std::complex<double> computePolarized();

private:

    /// The module path: one kinematic point, GPD sampled from m_pGPDModule.
    std::complex<double> computeCurrent(bool polarized);

    unsigned int m_level; ///< tanh-sinh refinement level.
    unsigned int m_nf; ///< Number of active flavours.
    double m_logQ2OverMu2; ///< log( Q^2 / muF^2 ).
    double m_alphaSOver2Pi; ///< alpha_s( muR^2 ) / ( 2 pi ).
    RunningAlphaStrongModule* m_pRunningAlphaStrongModule; ///< Related alphaS module.
};

} /* namespace PARTONS */

#endif /* PARTONS_WITH_TORCH */

#endif /* DVCS_CFF_TORCH_H */
