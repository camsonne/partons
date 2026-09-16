#ifndef IOFFE_TIME_DISTRIBUTION_LIGHT_CONE_H
#define IOFFE_TIME_DISTRIBUTION_LIGHT_CONE_H

/**
 * @file IoffeTimeDistributionLightCone.h
 * @author PARTONS team
 * @date 2026
 * @version 1.0
 */

#include <string>

#include "IoffeTimeDistributionModule.h"

namespace PARTONS {

/**
 * @class IoffeTimeDistributionLightCone
 *
 * @brief Light-cone Ioffe-time distribution of a GPD model.
 *
 * This module evaluates the light-cone Ioffe-time distribution (ITD), i.e. the plain Fourier transform of
 * the GPD with respect to the momentum fraction,
 * \f[
 * \mathcal{M}(\nu, \xi, t; \mu_{F}^{2}) = \int_{-1}^{1} \mathrm{d}x \, e^{i x \nu} F(x, \xi, t, \mu_{F}^{2}) \,.
 * \f]
 * The kinematic variable \f$z^{2}\f$ is ignored. In the forward limit (\f$\xi = 0, t = 0\f$) this is the ITD of
 * the collinear PDF, whose real and imaginary parts are the cosine and sine transforms of the valence
 * (\f$q - \bar{q}\f$) and total (\f$q + \bar{q}\f$) distributions, respectively. Off-forward kinematics give the
 * generalized ITD relevant for lattice QCD calculations of GPDs.
 *
 * This is the leading-order (in the matching) relation to lattice QCD matrix elements: lattice reduced
 * pseudo-ITDs \f$\mathfrak{M}(\nu, z^{2})\f$ coincide with the light-cone ITD up to \f$O(\alpha_{s})\f$
 * corrections and higher-twist \f$O(z^{2})\f$ terms. For a quantitative comparison with lattice data use
 * IoffeTimeDistributionPseudoNLO instead.
 *
 * The module has no parameter besides the integration routine (see MathIntegratorModule). Example of XML
 * configuration:
 *
 * \code{.xml}
 * <module type="IoffeTimeDistributionModule" name="IoffeTimeDistributionLightCone">
 *    <module type="GPDModule" name="GPDGK16">
 *    </module>
 * </module>
 * \endcode
 */
class IoffeTimeDistributionLightCone: public IoffeTimeDistributionModule {

public:

    static const unsigned int classId; ///< Unique ID to automatically register the class in the registry.

    /**
     * Constructor.
     * @param className Name of the class.
     */
    IoffeTimeDistributionLightCone(const std::string &className);

    /**
     * Destructor.
     */
    virtual ~IoffeTimeDistributionLightCone();

    virtual IoffeTimeDistributionLightCone* clone() const;

protected:

    /**
     * Copy constructor.
     */
    IoffeTimeDistributionLightCone(const IoffeTimeDistributionLightCone &other);
};

} /* namespace PARTONS */

#endif /* IOFFE_TIME_DISTRIBUTION_LIGHT_CONE_H */
