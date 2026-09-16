#ifndef IOFFE_TIME_DISTRIBUTION_MODULE_H
#define IOFFE_TIME_DISTRIBUTION_MODULE_H

/**
 * @file IoffeTimeDistributionModule.h
 * @author PARTONS team
 * @date 2026
 * @version 1.0
 */

#include <ElementaryUtils/parameters/Parameters.h>
#include <map>
#include <string>
#include <vector>

#include "../../beans/automation/BaseObjectData.h"
#include "../../beans/gpd/GPDType.h"
#include "../../beans/ioffe_time/IoffeTimeDistributionResult.h"
#include "../../beans/ioffe_time/IoffeTimeKinematic.h"
#include "../../beans/List.h"
#include "../../beans/parton_distribution/PartonDistribution.h"
#include "../../ModuleObject.h"
#include "../MathIntegratorModule.h"

namespace NumA {
class FunctionType1D;
} /* namespace NumA */

namespace PARTONS {

class GPDModule;

/**
 * @class IoffeTimeDistributionModule
 *
 * @brief Abstract class for the evaluation of Ioffe-time distributions from GPD (or PDF) models.
 *
 * The Ioffe-time distribution (ITD) is the Fourier conjugate of a parton distribution with respect to the
 * momentum fraction \f$x\f$. For a GPD \f$F(x, \xi, t, \mu_{F}^{2})\f$ (in the forward limit a PDF) it reads
 * \f[
 * \mathcal{M}(\nu, \xi, t; \mu_{F}^{2}) = \int_{-1}^{1} \mathrm{d}x \, e^{i x \nu} F(x, \xi, t, \mu_{F}^{2}) \,,
 * \f]
 * where \f$\nu = p \cdot z\f$ is the Ioffe time. This is the quantity that lattice QCD calculations of equal-time
 * matrix elements at space-like separation \f$z\f$ (quasi- and pseudo-distributions) give access to, after
 * renormalization and perturbative matching. Providing it in PARTONS allows any GPD model to be confronted directly
 * with lattice QCD data, e.g. in a global fit combining exclusive processes and lattice matrix elements.
 *
 * This abstract class provides the machinery shared by all implementations:
 * - the underlying GPDModule (mandatory sub-module, set via setGPDModule() or via XML automation),
 * - the numerical integration over \f$x\f$ (MathIntegratorModule), with a cache of GPD evaluations so that
 *   the GPD model is evaluated once per \f$x\f$ node for all partons, both components and all combinations,
 * - the generic transform \f$\int \mathrm{d}x\, K(x) F(x)\f$ for a kernel \f$K\f$ defined by the daughter classes
 *   through kernelRealPart() and kernelImaginaryPart(). The light-cone ITD corresponds to
 *   \f$K(x) = e^{i x \nu}\f$, while pseudo-distributions include the perturbative matching in the kernel.
 *
 * For the result conventions see IoffeTimeDistributionResult.
 *
 * Available implementations:
 * - IoffeTimeDistributionLightCone: leading-order light-cone ITD, i.e. the pure Fourier transform,
 * - IoffeTimeDistributionPseudoNLO: reduced pseudo-ITD at finite \f$z^{2}\f$, including the one-loop matching
 *   kernel, for the direct comparison with lattice QCD data.
 */
class IoffeTimeDistributionModule: public ModuleObject,
        public MathIntegratorModule {

public:

    static const std::string IOFFE_TIME_DISTRIBUTION_MODULE_CLASS_NAME; ///< Type of the module in XML automation.

    /**
     * Destructor.
     */
    virtual ~IoffeTimeDistributionModule();

    virtual IoffeTimeDistributionModule* clone() const = 0;
    virtual std::string toString() const;
    virtual void resolveObjectDependencies();
    virtual void run();
    virtual void configure(const ElemUtils::Parameters &parameters);
    virtual void prepareSubModules(
            const std::map<std::string, BaseObjectData>& subModulesData);

    /**
     * Compute Ioffe-time distributions at given kinematics for the requested GPD types.
     * @param kinematic Kinematics.
     * @param gpdType List of GPD types to be computed. If empty, all available types are computed.
     * @return Result.
     */
    virtual IoffeTimeDistributionResult compute(
            const IoffeTimeKinematic &kinematic, const List<GPDType>& gpdType =
                    List<GPDType>());

    /**
     * Get list of GPD types that can be computed by this module with the underlying GPD module.
     */
    virtual List<GPDType> getListOfAvailableGPDTypeForComputation() const;

    //********************************************************
    //*** SETTERS AND GETTERS ********************************
    //********************************************************

    /**
     * Get underlying GPD module.
     */
    GPDModule* getGPDModule() const;

    /**
     * Set underlying GPD module.
     */
    void setGPDModule(GPDModule* pGPDModule);

protected:

    /**
     * Default constructor.
     * @param className Name of the class.
     */
    IoffeTimeDistributionModule(const std::string &className);

    /**
     * Copy constructor.
     */
    IoffeTimeDistributionModule(const IoffeTimeDistributionModule &other);

    /**
     * Set internal kinematics.
     */
    virtual void setKinematics(const IoffeTimeKinematic& kinematic);

    virtual void initModule();
    virtual void isModuleWellConfigured();

    /**
     * Compute real and imaginary parts of the distribution for a given GPD type.
     * The default implementation performs the generic transform with the kernels given by
     * kernelRealPart() and kernelImaginaryPart().
     * @param gpdType GPD type.
     * @param realPart Real part to be filled.
     * @param imaginaryPart Imaginary part to be filled.
     */
    virtual void computeDistribution(GPDType::Type gpdType,
            PartonDistribution& realPart, PartonDistribution& imaginaryPart);

    /**
     * Real part of the kernel multiplying the GPD in the \f$x\f$-integral.
     * The default is \f$\cos(x \nu)\f$.
     * @param x Momentum fraction.
     * @param isQuark True for quark, false for gluon.
     */
    virtual double kernelRealPart(double x, bool isQuark);

    /**
     * Imaginary part of the kernel multiplying the GPD in the \f$x\f$-integral.
     * The default is \f$\sin(x \nu)\f$.
     * @param x Momentum fraction.
     * @param isQuark True for quark, false for gluon.
     */
    virtual double kernelImaginaryPart(double x, bool isQuark);

    /**
     * Evaluate the GPD of the current type at given \f$x\f$ (cached within a call of compute()).
     */
    const PartonDistribution& evaluateGPD(double x);

    /**
     * Perform the transform \f$\int \mathrm{d}x\, K(x) F(x)\f$ for all partons and fill the result.
     * The full \f$x \in [-1, 1]\f$ range is used for quarks and gluons, while the singlet and non-singlet
     * combinations are transformed over \f$x \in [0, 1]\f$.
     */
    void transform(GPDType::Type gpdType, PartonDistribution& realPart,
            PartonDistribution& imaginaryPart);

    /**
     * Integrate the integrand over a range split at the points where GPDs are not smooth (\f$x = 0, \pm\xi\f$).
     */
    double integrateWithBreakPoints(double xMin, double xMax,
            std::vector<double>& params);

    double m_nu; ///< Ioffe time.
    double m_z2; ///< Square of the space-like separation (in \f$\mathrm{GeV}^{-2}\f$).
    double m_xi; ///< Skewness.
    double m_t; ///< Momentum transfer squared (in \f$\mathrm{GeV}^{2}\f$).
    double m_MuF2; ///< Factorization scale squared (in \f$\mathrm{GeV}^{2}\f$).
    double m_MuR2; ///< Renormalization scale squared (in \f$\mathrm{GeV}^{2}\f$).

    GPDType::Type m_currentGPDComputeType; ///< GPD type of the current computation.
    List<GPDType> m_listGPDTypeAvailable; ///< GPD types this module is able to transform.
    GPDModule* m_pGPDModule; ///< Pointer to the underlying GPD module.

private:

    /**
     * Parton codes used in the integrand parameters.
     */
    enum PartonCode {
        GLUON = -1
    };

    /**
     * Component codes used in the integrand parameters.
     */
    enum ComponentCode {
        REAL_PART = 0, IMAGINARY_PART = 1
    };

    /**
     * Field codes used in the integrand parameters.
     */
    enum FieldCode {
        FULL = 0, PLUS = 1, MINUS = 2
    };

    /**
     * Integrand of the transform.
     * @param x Momentum fraction.
     * @param params Parameters: [0] parton code (QuarkFlavor::Type or GLUON), [1] component code, [2] field code.
     */
    double integrand(double x, std::vector<double>& params);

    /**
     * Create integration functors.
     */
    void initFunctorsForIntegrations();

    NumA::FunctionType1D* m_pIntegrand; ///< Functor for the integrand of the transform.

    std::map<double, PartonDistribution> m_GPDCache; ///< Cache of GPD evaluations for the current kinematics and GPD type.
};

} /* namespace PARTONS */

#endif /* IOFFE_TIME_DISTRIBUTION_MODULE_H */
