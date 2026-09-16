#ifndef IOFFE_TIME_KINEMATIC_H
#define IOFFE_TIME_KINEMATIC_H

/**
 * @file IoffeTimeKinematic.h
 * @author PARTONS team
 * @date 2026
 * @version 1.0
 */

#include <ElementaryUtils/parameters/GenericType.h>
#include <ElementaryUtils/parameters/Parameters.h>
#include <string>
#include <vector>

#include "../../utils/type/PhysicalType.h"
#include "../../utils/type/PhysicalUnit.h"
#include "../Kinematic.h"

namespace ElemUtils {
class Packet;
} /* namespace ElemUtils */

namespace PARTONS {

/**
 * @class IoffeTimeKinematic
 *
 * @brief Class representing single kinematics for Ioffe-time distributions.
 *
 * This class represents a single Ioffe-time kinematics
 * \f$(\nu, z^{2}, \xi, t, \mu_{F}^{2}, \mu_{R}^{2})\f$, where:
 *
 * - \f$\nu = p \cdot z\f$ is the Ioffe time (dimensionless). For a hadron of momentum \f$p\f$ probed with a
 *   space-like separation \f$z\f$ between the quark fields, as in lattice QCD, \f$\nu = p_{3} z_{3}\f$,
 * - \f$z^{2} = -z_{\mu} z^{\mu} \geq 0\f$ is the (positive) square of the space-like separation. It is expressed
 *   in \f$\mathrm{GeV}^{-2}\f$ by default and may also be given in \f$\mathrm{fm}^{2}\f$. It is only relevant for
 *   pseudo-distributions (matrix elements at finite separation); modules evaluating light-cone Ioffe-time
 *   distributions ignore it and one may simply set it to zero,
 * - \f$\xi\f$ is the skewness and \f$t\f$ the momentum transfer squared, allowing for the evaluation of
 *   generalized (off-forward) Ioffe-time distributions. The forward case, relevant to PDFs, is \f$\xi = 0\f$ and
 *   \f$t = 0\f$,
 * - \f$\mu_{F}^{2}\f$ and \f$\mu_{R}^{2}\f$ are the factorization and renormalization scales. For pseudo-distributions
 *   \f$\mu_{F}^{2}\f$ is the \f$\overline{\mathrm{MS}}\f$ scale at which the light-cone distribution is matched.
 *
 * The XML kinematics type name is `IoffeTimeKinematic` with parameters `nu`, `z2`, `xi`, `t`, `MuF2`, `MuR2`
 * and the optional `*_unit` counterparts, e.g.:
 *
 * \code{.xml}
 * <kinematics type="IoffeTimeKinematic">
 *    <param name="nu" value="3." />
 *    <param name="z2" value="0.16" />
 *    <param name="z2_unit" value="fm2" />
 *    <param name="xi" value="0." />
 *    <param name="t" value="0." />
 *    <param name="MuF2" value="4." />
 *    <param name="MuR2" value="4." />
 * </kinematics>
 * \endcode
 */
class IoffeTimeKinematic: public Kinematic {

public:

    static const std::string IOFFE_TIME_KINEMATIC_CLASS_NAME; ///< Type of the kinematic in XML automation.

    static const std::string KINEMATIC_PARAMETER_NAME_NU; ///< Name of parameter to set Ioffe time via configure()
    static const std::string KINEMATIC_PARAMETER_NAME_Z2; ///< Name of parameter to set squared separation via configure()
    static const std::string KINEMATIC_PARAMETER_NAME_XI; ///< Name of parameter to set skewness via configure()
    static const std::string KINEMATIC_PARAMETER_NAME_T; ///< Name of parameter to set momentum transfer via configure()
    static const std::string KINEMATIC_PARAMETER_NAME_MUF2; ///< Name of parameter to set factorization scale via configure()
    static const std::string KINEMATIC_PARAMETER_NAME_MUR2; ///< Name of parameter to set renormalization scale via configure()

    static const std::string KINEMATIC_PARAMETER_NAME_NU_UNIT; ///< Name of parameter to set unit of Ioffe time via configure()
    static const std::string KINEMATIC_PARAMETER_NAME_Z2_UNIT; ///< Name of parameter to set unit of squared separation via configure()
    static const std::string KINEMATIC_PARAMETER_NAME_XI_UNIT; ///< Name of parameter to set unit of skewness via configure()
    static const std::string KINEMATIC_PARAMETER_NAME_T_UNIT; ///< Name of parameter to set unit of momentum transfer via configure()
    static const std::string KINEMATIC_PARAMETER_NAME_MUF2_UNIT; ///< Name of parameter to set unit of factorization scale via configure()
    static const std::string KINEMATIC_PARAMETER_NAME_MUR2_UNIT; ///< Name of parameter to set unit of renormalization scale via configure()

    /**
     * Default constructor.
     */
    IoffeTimeKinematic();

    /**
     * Assignment constructor.
     * @param nu Ioffe time.
     * @param z2 Square of the space-like separation (in \f$\mathrm{GeV}^{-2}\f$).
     * @param xi Skewness.
     * @param t Momentum transfer squared (in \f$\mathrm{GeV}^{2}\f$).
     * @param MuF2 Factorization scale squared (in \f$\mathrm{GeV}^{2}\f$).
     * @param MuR2 Renormalization scale squared (in \f$\mathrm{GeV}^{2}\f$).
     */
    IoffeTimeKinematic(double nu, double z2, double xi, double t, double MuF2,
            double MuR2);

    /**
     * Assignment constructor with physical types (values with units).
     */
    IoffeTimeKinematic(const PhysicalType<double> &nu,
            const PhysicalType<double> &z2, const PhysicalType<double> &xi,
            const PhysicalType<double> &t, const PhysicalType<double> &MuF2,
            const PhysicalType<double> &MuR2);

    /**
     * Assignment constructor with generic types.
     */
    IoffeTimeKinematic(const ElemUtils::GenericType &nu,
            const ElemUtils::GenericType &z2, const ElemUtils::GenericType &xi,
            const ElemUtils::GenericType &t, const ElemUtils::GenericType &MuF2,
            const ElemUtils::GenericType &MuR2);

    /**
     * Copy constructor.
     */
    IoffeTimeKinematic(const IoffeTimeKinematic &other);

    /**
     * Destructor.
     */
    virtual ~IoffeTimeKinematic();

    virtual void configure(const ElemUtils::Parameters &parameters);
    virtual std::string toString() const;

    void serialize(ElemUtils::Packet &packet) const;
    void unserialize(ElemUtils::Packet &packet);

    void serializeIntoStdVector(std::vector<double>& vec) const;
    void unserializeFromStdVector(std::vector<double>::const_iterator& it,
            const std::vector<double>::const_iterator& end);

    bool operator ==(const IoffeTimeKinematic& other) const;
    bool operator !=(const IoffeTimeKinematic& other) const;

    //********************************************************
    //*** SETTERS AND GETTERS ********************************
    //********************************************************

    const PhysicalType<double>& getNu() const;
    void setNu(const PhysicalType<double>& nu);
    void setNu(double nu, PhysicalUnit::Type unit = PhysicalUnit::NONE);

    const PhysicalType<double>& getZ2() const;
    void setZ2(const PhysicalType<double>& z2);
    void setZ2(double z2, PhysicalUnit::Type unit = PhysicalUnit::GEVm2);

    const PhysicalType<double>& getXi() const;
    void setXi(const PhysicalType<double>& xi);
    void setXi(double xi, PhysicalUnit::Type unit = PhysicalUnit::NONE);

    const PhysicalType<double>& getT() const;
    void setT(const PhysicalType<double>& t);
    void setT(double t, PhysicalUnit::Type unit = PhysicalUnit::GEV2);

    const PhysicalType<double>& getMuF2() const;
    void setMuF2(const PhysicalType<double>& muF2);
    void setMuF2(double muF2, PhysicalUnit::Type unit = PhysicalUnit::GEV2);

    const PhysicalType<double>& getMuR2() const;
    void setMuR2(const PhysicalType<double>& muR2);
    void setMuR2(double muR2, PhysicalUnit::Type unit = PhysicalUnit::GEV2);

protected:

    virtual void updateHashSum() const;

private:

    PhysicalType<double> m_nu; ///< Ioffe time \f$\nu = p \cdot z\f$.
    PhysicalType<double> m_z2; ///< Square of the space-like separation \f$z^{2}\f$ (in \f$\mathrm{GeV}^{-2}\f$ by default).
    PhysicalType<double> m_xi; ///< Skewness.
    PhysicalType<double> m_t; ///< Momentum transfer squared (in \f$\mathrm{GeV}^{2}\f$).
    PhysicalType<double> m_MuF2; ///< Factorization scale squared (in \f$\mathrm{GeV}^{2}\f$).
    PhysicalType<double> m_MuR2; ///< Renormalization scale squared (in \f$\mathrm{GeV}^{2}\f$).

    /**
     * Helper used in configure() to read a single variable and its optional unit.
     */
    void configureVariable(const ElemUtils::Parameters &parameters,
            const std::string &name, const std::string &unitName,
            PhysicalUnit::Type defaultUnit,
            void (IoffeTimeKinematic::*setter)(double, PhysicalUnit::Type));
};

/**
 * Stream operator to serialize class into Packet.
 */
ElemUtils::Packet& operator <<(ElemUtils::Packet& packet,
        IoffeTimeKinematic& kinematic);

/**
 * Stream operator to retrieve class from Packet.
 */
ElemUtils::Packet& operator >>(ElemUtils::Packet& packet,
        IoffeTimeKinematic& kinematic);

} /* namespace PARTONS */

#endif /* IOFFE_TIME_KINEMATIC_H */
