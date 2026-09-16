#include "../../../../include/partons/beans/ioffe_time/IoffeTimeKinematic.h"

#include <ElementaryUtils/string_utils/Formatter.h>

#include "../../../../include/partons/beans/channel/ChannelType.h"
#include "../../../../include/partons/Partons.h"
#include "../../../../include/partons/services/hash_sum/CryptographicHashService.h"
#include "../../../../include/partons/ServiceObjectRegistry.h"

namespace PARTONS {

const std::string IoffeTimeKinematic::IOFFE_TIME_KINEMATIC_CLASS_NAME =
        "IoffeTimeKinematic";

const std::string IoffeTimeKinematic::KINEMATIC_PARAMETER_NAME_NU = "nu";
const std::string IoffeTimeKinematic::KINEMATIC_PARAMETER_NAME_Z2 = "z2";
const std::string IoffeTimeKinematic::KINEMATIC_PARAMETER_NAME_XI = "xi";
const std::string IoffeTimeKinematic::KINEMATIC_PARAMETER_NAME_T = "t";
const std::string IoffeTimeKinematic::KINEMATIC_PARAMETER_NAME_MUF2 = "MuF2";
const std::string IoffeTimeKinematic::KINEMATIC_PARAMETER_NAME_MUR2 = "MuR2";

const std::string IoffeTimeKinematic::KINEMATIC_PARAMETER_NAME_NU_UNIT =
        "nu_unit";
const std::string IoffeTimeKinematic::KINEMATIC_PARAMETER_NAME_Z2_UNIT =
        "z2_unit";
const std::string IoffeTimeKinematic::KINEMATIC_PARAMETER_NAME_XI_UNIT =
        "xi_unit";
const std::string IoffeTimeKinematic::KINEMATIC_PARAMETER_NAME_T_UNIT =
        "t_unit";
const std::string IoffeTimeKinematic::KINEMATIC_PARAMETER_NAME_MUF2_UNIT =
        "MuF2_unit";
const std::string IoffeTimeKinematic::KINEMATIC_PARAMETER_NAME_MUR2_UNIT =
        "MuR2_unit";

IoffeTimeKinematic::IoffeTimeKinematic() :
        Kinematic("IoffeTimeKinematic", ChannelType::UNDEFINED), m_nu(
                PhysicalType<double>(PhysicalUnit::NONE)), m_z2(
                PhysicalType<double>(PhysicalUnit::GEVm2)), m_xi(
                PhysicalType<double>(PhysicalUnit::NONE)), m_t(
                PhysicalType<double>(PhysicalUnit::GEV2)), m_MuF2(
                PhysicalType<double>(PhysicalUnit::GEV2)), m_MuR2(
                PhysicalType<double>(PhysicalUnit::GEV2)) {
}

IoffeTimeKinematic::IoffeTimeKinematic(double nu, double z2, double xi,
        double t, double MuF2, double MuR2) :
        Kinematic("IoffeTimeKinematic", ChannelType::UNDEFINED), m_nu(
                PhysicalType<double>(nu, PhysicalUnit::NONE)), m_z2(
                PhysicalType<double>(z2, PhysicalUnit::GEVm2)), m_xi(
                PhysicalType<double>(xi, PhysicalUnit::NONE)), m_t(
                PhysicalType<double>(t, PhysicalUnit::GEV2)), m_MuF2(
                PhysicalType<double>(MuF2, PhysicalUnit::GEV2)), m_MuR2(
                PhysicalType<double>(MuR2, PhysicalUnit::GEV2)) {
}

IoffeTimeKinematic::IoffeTimeKinematic(const PhysicalType<double> &nu,
        const PhysicalType<double> &z2, const PhysicalType<double> &xi,
        const PhysicalType<double> &t, const PhysicalType<double> &MuF2,
        const PhysicalType<double> &MuR2) :
        Kinematic("IoffeTimeKinematic", ChannelType::UNDEFINED), m_nu(
                PhysicalType<double>(PhysicalUnit::NONE)), m_z2(
                PhysicalType<double>(PhysicalUnit::GEVm2)), m_xi(
                PhysicalType<double>(PhysicalUnit::NONE)), m_t(
                PhysicalType<double>(PhysicalUnit::GEV2)), m_MuF2(
                PhysicalType<double>(PhysicalUnit::GEV2)), m_MuR2(
                PhysicalType<double>(PhysicalUnit::GEV2)) {

    m_nu.checkIfSameUnitCategoryAs(nu);
    m_z2.checkIfSameUnitCategoryAs(z2);
    m_xi.checkIfSameUnitCategoryAs(xi);
    m_t.checkIfSameUnitCategoryAs(t);
    m_MuF2.checkIfSameUnitCategoryAs(MuF2);
    m_MuR2.checkIfSameUnitCategoryAs(MuR2);

    m_nu = nu;
    m_z2 = z2;
    m_xi = xi;
    m_t = t;
    m_MuF2 = MuF2;
    m_MuR2 = MuR2;
}

IoffeTimeKinematic::IoffeTimeKinematic(const ElemUtils::GenericType &nu,
        const ElemUtils::GenericType &z2, const ElemUtils::GenericType &xi,
        const ElemUtils::GenericType &t, const ElemUtils::GenericType &MuF2,
        const ElemUtils::GenericType &MuR2) :
        Kinematic("IoffeTimeKinematic", ChannelType::UNDEFINED), m_nu(
                PhysicalType<double>(nu, PhysicalUnit::NONE)), m_z2(
                PhysicalType<double>(z2, PhysicalUnit::GEVm2)), m_xi(
                PhysicalType<double>(xi, PhysicalUnit::NONE)), m_t(
                PhysicalType<double>(t, PhysicalUnit::GEV2)), m_MuF2(
                PhysicalType<double>(MuF2, PhysicalUnit::GEV2)), m_MuR2(
                PhysicalType<double>(MuR2, PhysicalUnit::GEV2)) {
}

IoffeTimeKinematic::IoffeTimeKinematic(const IoffeTimeKinematic &other) :
        Kinematic(other), m_nu(other.m_nu), m_z2(other.m_z2), m_xi(other.m_xi), m_t(
                other.m_t), m_MuF2(other.m_MuF2), m_MuR2(other.m_MuR2) {
}

IoffeTimeKinematic::~IoffeTimeKinematic() {
}

void IoffeTimeKinematic::configureVariable(
        const ElemUtils::Parameters &parameters, const std::string &name,
        const std::string &unitName, PhysicalUnit::Type defaultUnit,
        void (IoffeTimeKinematic::*setter)(double, PhysicalUnit::Type)) {

    if (parameters.isAvailable(name)) {

        double value = parameters.getLastAvailable().toDouble();

        if (parameters.isAvailable(unitName)) {
            PhysicalUnit::Type unit = PhysicalUnit(
                    parameters.getLastAvailable().getString()).getType();
            ((*this).*setter)(value, unit);
        } else {
            ((*this).*setter)(value, defaultUnit);
        }
    } else {
        errorMissingParameter(name);
    }
}

void IoffeTimeKinematic::configure(const ElemUtils::Parameters &parameters) {

    //run for mother
    Kinematic::configure(parameters);

    configureVariable(parameters, KINEMATIC_PARAMETER_NAME_NU,
            KINEMATIC_PARAMETER_NAME_NU_UNIT, PhysicalUnit::NONE,
            &IoffeTimeKinematic::setNu);

    //z2 is optional (only needed for pseudo-distributions), default is 0
    if (parameters.isAvailable(KINEMATIC_PARAMETER_NAME_Z2)) {
        configureVariable(parameters, KINEMATIC_PARAMETER_NAME_Z2,
                KINEMATIC_PARAMETER_NAME_Z2_UNIT, PhysicalUnit::GEVm2,
                &IoffeTimeKinematic::setZ2);
    } else {
        setZ2(0., PhysicalUnit::GEVm2);
    }

    //xi and t are optional (forward limit), default is 0
    if (parameters.isAvailable(KINEMATIC_PARAMETER_NAME_XI)) {
        configureVariable(parameters, KINEMATIC_PARAMETER_NAME_XI,
                KINEMATIC_PARAMETER_NAME_XI_UNIT, PhysicalUnit::NONE,
                &IoffeTimeKinematic::setXi);
    } else {
        setXi(0., PhysicalUnit::NONE);
    }

    if (parameters.isAvailable(KINEMATIC_PARAMETER_NAME_T)) {
        configureVariable(parameters, KINEMATIC_PARAMETER_NAME_T,
                KINEMATIC_PARAMETER_NAME_T_UNIT, PhysicalUnit::GEV2,
                &IoffeTimeKinematic::setT);
    } else {
        setT(0., PhysicalUnit::GEV2);
    }

    configureVariable(parameters, KINEMATIC_PARAMETER_NAME_MUF2,
            KINEMATIC_PARAMETER_NAME_MUF2_UNIT, PhysicalUnit::GEV2,
            &IoffeTimeKinematic::setMuF2);

    configureVariable(parameters, KINEMATIC_PARAMETER_NAME_MUR2,
            KINEMATIC_PARAMETER_NAME_MUR2_UNIT, PhysicalUnit::GEV2,
            &IoffeTimeKinematic::setMuR2);
}

std::string IoffeTimeKinematic::toString() const {

    ElemUtils::Formatter formatter;

    formatter << Kinematic::toString() << '\n';

    if (m_nu.isInitialized())
        formatter << "nu: " << m_nu.toString() << ' ';
    if (m_z2.isInitialized())
        formatter << "z2: " << m_z2.toString() << ' ';
    if (m_xi.isInitialized())
        formatter << "xi: " << m_xi.toString() << ' ';
    if (m_t.isInitialized())
        formatter << "t: " << m_t.toString() << ' ';
    if (m_MuF2.isInitialized())
        formatter << "muF2: " << m_MuF2.toString() << ' ';
    if (m_MuR2.isInitialized())
        formatter << "muR2: " << m_MuR2.toString() << ' ';

    return formatter.str();
}

void IoffeTimeKinematic::serialize(ElemUtils::Packet &packet) const {

    Kinematic::serialize(packet);

    packet << m_nu << m_z2 << m_xi << m_t << m_MuF2 << m_MuR2;
}

void IoffeTimeKinematic::unserialize(ElemUtils::Packet &packet) {

    Kinematic::unserialize(packet);

    packet >> m_nu;
    packet >> m_z2;
    packet >> m_xi;
    packet >> m_t;
    packet >> m_MuF2;
    packet >> m_MuR2;

    updateHashSum();
}

void IoffeTimeKinematic::serializeIntoStdVector(
        std::vector<double>& vec) const {

    Kinematic::serializeIntoStdVector(vec);

    m_nu.serializeIntoStdVector(vec);
    m_z2.serializeIntoStdVector(vec);
    m_xi.serializeIntoStdVector(vec);
    m_t.serializeIntoStdVector(vec);
    m_MuF2.serializeIntoStdVector(vec);
    m_MuR2.serializeIntoStdVector(vec);
}

void IoffeTimeKinematic::unserializeFromStdVector(
        std::vector<double>::const_iterator& it,
        const std::vector<double>::const_iterator& end) {

    Kinematic::unserializeFromStdVector(it, end);

    m_nu.unserializeFromStdVector(it, end);
    m_z2.unserializeFromStdVector(it, end);
    m_xi.unserializeFromStdVector(it, end);
    m_t.unserializeFromStdVector(it, end);
    m_MuF2.unserializeFromStdVector(it, end);
    m_MuR2.unserializeFromStdVector(it, end);

    updateHashSum();
}

bool IoffeTimeKinematic::operator ==(const IoffeTimeKinematic& other) const {
    return m_nu == other.getNu() && m_z2 == other.getZ2()
            && m_xi == other.getXi() && m_t == other.getT()
            && m_MuF2 == other.getMuF2() && m_MuR2 == other.getMuR2();
}

bool IoffeTimeKinematic::operator !=(const IoffeTimeKinematic& other) const {
    return !((*this) == other);
}

void IoffeTimeKinematic::updateHashSum() const {
    setHashSum(
            Partons::getInstance()->getServiceObjectRegistry()->getCryptographicHashService()->generateSHA1HashSum(
                    ElemUtils::Formatter() << m_nu.toStdString()
                            << m_z2.toStdString() << m_xi.toStdString()
                            << m_t.toStdString() << m_MuF2.toStdString()
                            << m_MuR2.toStdString()));
}

const PhysicalType<double>& IoffeTimeKinematic::getNu() const {
    return m_nu;
}

const PhysicalType<double>& IoffeTimeKinematic::getZ2() const {
    return m_z2;
}

const PhysicalType<double>& IoffeTimeKinematic::getXi() const {
    return m_xi;
}

const PhysicalType<double>& IoffeTimeKinematic::getT() const {
    return m_t;
}

const PhysicalType<double>& IoffeTimeKinematic::getMuF2() const {
    return m_MuF2;
}

const PhysicalType<double>& IoffeTimeKinematic::getMuR2() const {
    return m_MuR2;
}

void IoffeTimeKinematic::setNu(const PhysicalType<double>& nu) {
    m_nu.checkIfSameUnitCategoryAs(nu);
    m_nu = nu;
    updateHashSum();
}

void IoffeTimeKinematic::setZ2(const PhysicalType<double>& z2) {
    m_z2.checkIfSameUnitCategoryAs(z2);
    m_z2 = z2;
    updateHashSum();
}

void IoffeTimeKinematic::setXi(const PhysicalType<double>& xi) {
    m_xi.checkIfSameUnitCategoryAs(xi);
    m_xi = xi;
    updateHashSum();
}

void IoffeTimeKinematic::setT(const PhysicalType<double>& t) {
    m_t.checkIfSameUnitCategoryAs(t);
    m_t = t;
    updateHashSum();
}

void IoffeTimeKinematic::setMuF2(const PhysicalType<double>& muF2) {
    m_MuF2.checkIfSameUnitCategoryAs(muF2);
    m_MuF2 = muF2;
    updateHashSum();
}

void IoffeTimeKinematic::setMuR2(const PhysicalType<double>& muR2) {
    m_MuR2.checkIfSameUnitCategoryAs(muR2);
    m_MuR2 = muR2;
    updateHashSum();
}

void IoffeTimeKinematic::setNu(double nu, PhysicalUnit::Type unit) {
    setNu(PhysicalType<double>(nu, unit));
}

void IoffeTimeKinematic::setZ2(double z2, PhysicalUnit::Type unit) {
    setZ2(PhysicalType<double>(z2, unit));
}

void IoffeTimeKinematic::setXi(double xi, PhysicalUnit::Type unit) {
    setXi(PhysicalType<double>(xi, unit));
}

void IoffeTimeKinematic::setT(double t, PhysicalUnit::Type unit) {
    setT(PhysicalType<double>(t, unit));
}

void IoffeTimeKinematic::setMuF2(double muF2, PhysicalUnit::Type unit) {
    setMuF2(PhysicalType<double>(muF2, unit));
}

void IoffeTimeKinematic::setMuR2(double muR2, PhysicalUnit::Type unit) {
    setMuR2(PhysicalType<double>(muR2, unit));
}

ElemUtils::Packet& operator <<(ElemUtils::Packet& packet,
        IoffeTimeKinematic& kinematic) {
    kinematic.serialize(packet);
    return packet;
}

ElemUtils::Packet& operator >>(ElemUtils::Packet& packet,
        IoffeTimeKinematic& kinematic) {
    kinematic.unserialize(packet);
    return packet;
}

} /* namespace PARTONS */
