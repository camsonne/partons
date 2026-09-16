#ifndef IOFFE_TIME_DISTRIBUTION_PSEUDO_NLO_H
#define IOFFE_TIME_DISTRIBUTION_PSEUDO_NLO_H

/**
 * @file IoffeTimeDistributionPseudoNLO.h
 * @author PARTONS team
 * @date 2026
 * @version 1.0
 */

#include <ElementaryUtils/parameters/Parameters.h>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../../beans/automation/BaseObjectData.h"
#include "IoffeTimeDistributionModule.h"

namespace NumA {
class FunctionType1D;
class Integrator1D;
} /* namespace NumA */

namespace PARTONS {

class RunningAlphaStrongModule;

/**
 * @class IoffeTimeDistributionPseudoNLO
 *
 * @brief Reduced pseudo-Ioffe-time distribution at finite separation \f$z^{2}\f$, with one-loop matching.
 *
 * Lattice QCD gives access to equal-time matrix elements of quark bilinears separated by a space-like
 * distance \f$z\f$, \f$\mathcal{M}(\nu, z^{2})\f$ with \f$\nu = p \cdot z\f$. In the pseudo-distribution approach
 * @cite Radyushkin:2017cyf, the ratio
 * \f[
 * \mathfrak{M}(\nu, z^{2}) = \frac{\mathcal{M}(\nu, z^{2}) / \mathcal{M}(\nu, 0)}{\mathcal{M}(0, z^{2}) / \mathcal{M}(0, 0)}
 * \f]
 * (the reduced pseudo-ITD) is renormalization-group invariant and is related to the light-cone ITD
 * \f$\mathcal{I}(\nu, \mu^{2})\f$ of the \f$\overline{\mathrm{MS}}\f$ PDF through the one-loop matching relation
 * @cite Radyushkin:2018cvn @cite Izubuchi:2018srq @cite Joo:2019jct
 * \f[
 * \mathfrak{M}(\nu, z^{2}) = \int_{0}^{1} \mathrm{d}u \, \mathcal{I}(u \nu, \mu^{2})
 * \left\{ \delta(1-u) - \frac{\alpha_{s} C_{F}}{2\pi}
 * \left[ B(u) \ln\left( z^{2} \mu^{2} \frac{e^{2\gamma_{E}+1}}{4} \right) + D(u) \right]_{+} \right\}
 * + O(z^{2} \Lambda_{\mathrm{QCD}}^{2}) \,,
 * \f]
 * where for unpolarized quarks \f$B(u) = (1+u^{2})/(1-u)\f$ is the Altarelli-Parisi kernel and
 * \f$D(u) = 4 \ln(1-u)/(1-u) - 2(1-u)\f$. The same one-loop kernel applies to the helicity case
 * @cite Edwards:2022hxv, while for transversity @cite HadStruc:2021qdf \f$B_{T}(u) = 2u/(1-u)\f$ and
 * \f$D_{T}(u) = 4 \ln(1-u)/(1-u)\f$.
 *
 * Since the relation is linear in the light-cone distribution, the \f$u\f$-convolution is moved into the kernel of
 * the \f$x\f$-integral,
 * \f[
 * \mathfrak{M}(\nu, z^{2}) = \int_{-1}^{1} \mathrm{d}x \, F(x, \mu^{2}) \, K(x \nu, z^{2} \mu^{2}) \,, \quad
 * K(w, z^{2}\mu^{2}) = e^{i w} - \frac{\alpha_{s} C_{F}}{2\pi} \int_{0}^{1} \mathrm{d}u \,
 * \left[ B(u) \ln\left( z^{2} \mu^{2} \frac{e^{2\gamma_{E}+1}}{4} \right) + D(u) \right] \left( e^{i u w} - e^{i w} \right) \,,
 * \f]
 * so that the GPD model is evaluated only once per \f$x\f$ node, and the matching is exact in the sense of the
 * plus prescription.
 *
 * Remarks:
 * - the factorization scale \f$\mu_{F}^{2}\f$ of the kinematics is used as \f$\mu^{2}\f$, and \f$\alpha_{s}\f$ is
 *   evaluated at \f$\mu_{R}^{2}\f$ (or given as a fixed number),
 * - the result is not normalized: for a flavor combination whose lowest moment is not unity (e.g. a single
 *   flavor), divide by the same combination at \f$\nu = 0\f$ to obtain the reduced distribution normalized to
 *   \f$\mathfrak{M}(0, z^{2}) = 1\f$. For the isovector combination \f$u - d\f$ of unpolarized quarks the
 *   normalization is automatic when the model satisfies the quark number sum rule,
 * - the one-loop matching implemented here is the forward one, hence the module requires \f$\xi = 0\f$. The
 *   \f$t\f$-dependence is allowed (it enters through the GPD model only), giving the pseudo-ITD of zero-skewness
 *   GPDs, as studied on the lattice,
 * - gluons are transformed with the leading-order kernel only (the gluon one-loop matching is not implemented),
 * - for \f$z^{2} = 0\f$ the matching logarithm is ill defined; use IoffeTimeDistributionLightCone instead.
 *
 * Available parameters:
 * - `alphaS`: fixed value of \f$\alpha_{s}\f$ used in the matching. Alternatively, provide a
 *   RunningAlphaStrongModule as sub-module (evaluated at \f$\mu_{R}^{2}\f$),
 * - `matchingKernel`: `auto` (default, selected from the GPD type: unpolarized for \f$H, E\f$, helicity for
 *   \f$\tilde{H}, \tilde{E}\f$ and transversity for the chiral-odd GPDs), `unpolarized`, `helicity` or
 *   `transversity`,
 * - `integrator_type`: integration routine used for the \f$x\f$-integral (see MathIntegratorModule).
 *
 * Example of XML configuration:
 *
 * \code{.xml}
 * <module type="IoffeTimeDistributionModule" name="IoffeTimeDistributionPseudoNLO">
 *    <param name="alphaS" value="0.303" />
 *    <module type="GPDModule" name="GPDGK16">
 *    </module>
 * </module>
 * \endcode
 */
class IoffeTimeDistributionPseudoNLO: public IoffeTimeDistributionModule {

public:

    static const unsigned int classId; ///< Unique ID to automatically register the class in the registry.

    static const std::string PARAM_NAME_ALPHA_S; ///< Name of parameter to set the fixed value of alpha_s via configure().
    static const std::string PARAM_NAME_MATCHING_KERNEL; ///< Name of parameter to select the matching kernel via configure().

    /**
     * Type of the one-loop matching kernel.
     */
    enum MatchingKernelType {
        AUTO = 0, ///< Selected from the GPD type.
        UNPOLARIZED = 1, ///< Unpolarized quarks (GPDs H and E).
        HELICITY = 2, ///< Quark helicity (GPDs Ht and Et).
        TRANSVERSITY = 3 ///< Quark transversity (chiral-odd GPDs).
    };

    /**
     * Constructor.
     * @param className Name of the class.
     */
    IoffeTimeDistributionPseudoNLO(const std::string &className);

    /**
     * Destructor.
     */
    virtual ~IoffeTimeDistributionPseudoNLO();

    virtual IoffeTimeDistributionPseudoNLO* clone() const;
    virtual std::string toString() const;
    virtual void configure(const ElemUtils::Parameters &parameters);
    virtual void prepareSubModules(
            const std::map<std::string, BaseObjectData>& subModulesData);

    /**
     * Convert a matching kernel type from string.
     */
    static MatchingKernelType matchingKernelTypeFromString(
            const std::string &name);

    /**
     * Convert a matching kernel type to string.
     */
    static std::string matchingKernelTypeToString(MatchingKernelType type);

    //********************************************************
    //*** SETTERS AND GETTERS ********************************
    //********************************************************

    /**
     * Get fixed value of alpha_s (only meaningful if isAlphaSFixed()).
     */
    double getAlphaS() const;

    /**
     * Set fixed value of alpha_s. This disables the RunningAlphaStrongModule if any.
     */
    void setAlphaS(double alphaS);

    /**
     * Check if a fixed value of alpha_s is used.
     */
    bool isAlphaSFixed() const;

    /**
     * Get running alpha_s module.
     */
    RunningAlphaStrongModule* getRunningAlphaStrongModule() const;

    /**
     * Set running alpha_s module (evaluated at MuR2). This disables the fixed value of alpha_s.
     */
    void setRunningAlphaStrongModule(
            RunningAlphaStrongModule* pRunningAlphaStrongModule);

    /**
     * Get type of the matching kernel.
     */
    MatchingKernelType getMatchingKernelType() const;

    /**
     * Set type of the matching kernel.
     */
    void setMatchingKernelType(MatchingKernelType matchingKernelType);

    /**
     * Get the value of alpha_s used in the last computation.
     */
    double getAlphaSUsed() const;

protected:

    /**
     * Copy constructor.
     */
    IoffeTimeDistributionPseudoNLO(const IoffeTimeDistributionPseudoNLO &other);

    virtual void initModule();
    virtual void isModuleWellConfigured();

    virtual void computeDistribution(GPDType::Type gpdType,
            PartonDistribution& realPart, PartonDistribution& imaginaryPart);

    virtual double kernelRealPart(double x, bool isQuark);
    virtual double kernelImaginaryPart(double x, bool isQuark);

private:

    /**
     * Matching kernel for a given GPD type in AUTO mode.
     */
    MatchingKernelType resolveMatchingKernelType(GPDType::Type gpdType) const;

    /**
     * Logarithmic part of the one-loop kernel, \f$B(u)\f$ (without plus prescription).
     */
    double kernelB(double u) const;

    /**
     * Finite part of the one-loop kernel, \f$D(u)\f$ (without plus prescription).
     */
    double kernelD(double u) const;

    /**
     * Integrand of the u-convolution: \f$[L B(u) + D(u)] (\cos(u w) - \cos(w))\f$ or the sine counterpart.
     * @param u Integration variable.
     * @param params Parameters: [0] w = x nu, [1] component (0 for real part, 1 for imaginary part).
     */
    double matchingIntegrand(double u, std::vector<double>& params);

    /**
     * Evaluate (and cache) real and imaginary parts of the matched kernel at given x.
     */
    const std::pair<double, double>& matchedKernel(double x);

    /**
     * Create integration functors.
     */
    void initFunctorsForIntegrations();

    double m_alphaS; ///< Fixed value of alpha_s.
    bool m_isAlphaSFixed; ///< True if the fixed value of alpha_s is used.
    RunningAlphaStrongModule* m_pRunningAlphaStrongModule; ///< Pointer to the running alpha_s module (optional).
    MatchingKernelType m_matchingKernelType; ///< Configured type of the matching kernel.

    MatchingKernelType m_activeKernelType; ///< Type of the matching kernel for the current GPD type.
    double m_alphaSUsed; ///< Value of alpha_s used in the current computation.
    double m_prefactor; ///< \f$\alpha_{s} C_{F} / (2\pi)\f$.
    double m_logZ2Mu2; ///< \f$\ln(z^{2} \mu^{2} e^{2\gamma_{E}+1}/4)\f$.

    NumA::Integrator1D* m_pMatchingIntegrator; ///< Integration routine for the u-convolution.
    NumA::FunctionType1D* m_pMatchingIntegrand; ///< Functor for the u-convolution integrand.

    std::map<double, std::pair<double, double> > m_kernelCache; ///< Cache of matched kernel values for the current kinematics.
};

} /* namespace PARTONS */

#endif /* IOFFE_TIME_DISTRIBUTION_PSEUDO_NLO_H */
