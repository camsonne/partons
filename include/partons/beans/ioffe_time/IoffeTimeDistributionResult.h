#ifndef IOFFE_TIME_DISTRIBUTION_RESULT_H
#define IOFFE_TIME_DISTRIBUTION_RESULT_H

/**
 * @file IoffeTimeDistributionResult.h
 * @author PARTONS team
 * @date 2026
 * @version 1.0
 */

#include <complex>
#include <map>
#include <string>
#include <vector>

#include "../gpd/GPDType.h"
#include "../parton_distribution/PartonDistribution.h"
#include "../QuarkFlavor.h"
#include "../Result.h"
#include "IoffeTimeKinematic.h"

namespace PARTONS {

/**
 * @class IoffeTimeDistributionResult
 *
 * @brief Class representing the result of the evaluation of Ioffe-time distributions at a given kinematics.
 *
 * For each GPD type the Ioffe-time distribution
 * \f[
 * \mathcal{M}(\nu, \xi, t; \mu_{F}^{2}, z^{2}) = \int_{-1}^{1} \mathrm{d}x \, e^{i x \nu} F(x, \xi, t, \mu_{F}^{2}) + \ldots
 * \f]
 * is a complex number for every parton species (quark flavors and gluons). It is stored as two PartonDistribution
 * objects: one for the real part and one for the imaginary part. The `quarkDistribution` field of a
 * QuarkDistribution holds the transform of the GPD over the full \f$x \in [-1, 1]\f$ range (quarks and
 * antiquarks), while `quarkDistributionPlus` and `quarkDistributionMinus` hold the transforms of the singlet
 * and non-singlet combinations \f$F^{q(\pm)}\f$ returned by the GPD module, restricted to \f$x \in [0, 1]\f$.
 *
 * In the forward limit and for unpolarized quarks, \f$F(x) = q(x)\f$ for \f$x > 0\f$ and \f$F(x) = -\bar{q}(-x)\f$
 * for \f$x < 0\f$, so that the real part of the distribution is the cosine transform of the valence combination
 * \f$q - \bar{q}\f$ and the imaginary part the sine transform of \f$q + \bar{q}\f$, in accordance with the
 * definitions used in lattice QCD analyses. Lattice data are typically given for the isovector combination
 * \f$u - d\f$, see getIsovector().
 */
class IoffeTimeDistributionResult: public Result<IoffeTimeKinematic> {

public:

    /**
     * Default constructor.
     */
    IoffeTimeDistributionResult();

    /**
     * Assignment constructor.
     * @param kinematic Kinematics associated to this result.
     */
    IoffeTimeDistributionResult(const IoffeTimeKinematic& kinematic);

    /**
     * Copy constructor.
     */
    IoffeTimeDistributionResult(const IoffeTimeDistributionResult &other);

    /**
     * Destructor.
     */
    virtual ~IoffeTimeDistributionResult();

    virtual std::string toString() const;

    /**
     * Add the distribution for a given GPD type.
     * @param gpdType GPD type.
     * @param realPart Real part of the distribution for all partons.
     * @param imaginaryPart Imaginary part of the distribution for all partons.
     */
    void addDistribution(GPDType::Type gpdType,
            const PartonDistribution& realPart,
            const PartonDistribution& imaginaryPart);

    /**
     * Get real part of the distribution for a given GPD type.
     */
    const PartonDistribution& getRealPart(GPDType::Type gpdType) const;

    /**
     * Get imaginary part of the distribution for a given GPD type.
     */
    const PartonDistribution& getImaginaryPart(GPDType::Type gpdType) const;

    /**
     * Get complex distribution of a given quark flavor for a given GPD type.
     */
    std::complex<double> getQuarkDistribution(GPDType::Type gpdType,
            QuarkFlavor::Type quarkFlavor) const;

    /**
     * Get complex gluon distribution for a given GPD type.
     */
    std::complex<double> getGluonDistribution(GPDType::Type gpdType) const;

    /**
     * Get complex isovector combination \f$u - d\f$ for a given GPD type.
     *
     * This is the flavor combination measured in most lattice QCD calculations of Ioffe-time
     * (pseudo-)distributions. For the unpolarized case its real part is normalized to unity at \f$\nu = 0\f$
     * by the quark number sum rule.
     */
    std::complex<double> getIsovector(GPDType::Type gpdType) const;

    /**
     * Check if the distribution for a given GPD type is available.
     */
    bool isAvailable(GPDType::Type gpdType) const;

    /**
     * Get list of GPD types for which the distribution is available.
     */
    std::vector<GPDType> listGPDTypeComputed() const;

    //********************************************************
    //*** SETTERS AND GETTERS ********************************
    //********************************************************

    const std::map<GPDType::Type, PartonDistribution>& getRealParts() const;
    const std::map<GPDType::Type, PartonDistribution>& getImaginaryParts() const;

private:

    std::map<GPDType::Type, PartonDistribution> m_realParts; ///< Real parts of the distribution, by GPD type.
    std::map<GPDType::Type, PartonDistribution> m_imaginaryParts; ///< Imaginary parts of the distribution, by GPD type.

    /**
     * Find a map entry, throwing a readable exception if missing.
     */
    const PartonDistribution& find(
            const std::map<GPDType::Type, PartonDistribution>& map,
            GPDType::Type gpdType, const std::string &funcName) const;
};

} /* namespace PARTONS */

#endif /* IOFFE_TIME_DISTRIBUTION_RESULT_H */
