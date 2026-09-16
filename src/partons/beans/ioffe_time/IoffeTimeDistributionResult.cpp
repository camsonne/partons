#include "../../../../include/partons/beans/ioffe_time/IoffeTimeDistributionResult.h"

#include <ElementaryUtils/logger/CustomException.h>
#include <ElementaryUtils/string_utils/Formatter.h>
#include <utility>

#include "../../../../include/partons/beans/channel/ChannelType.h"
#include "../../../../include/partons/beans/parton_distribution/GluonDistribution.h"
#include "../../../../include/partons/beans/parton_distribution/QuarkDistribution.h"

namespace PARTONS {

IoffeTimeDistributionResult::IoffeTimeDistributionResult() :
        Result<IoffeTimeKinematic>("IoffeTimeDistributionResult",
                ChannelType::UNDEFINED) {
}

IoffeTimeDistributionResult::IoffeTimeDistributionResult(
        const IoffeTimeKinematic& kinematic) :
        Result<IoffeTimeKinematic>("IoffeTimeDistributionResult",
                ChannelType::UNDEFINED, kinematic) {
}

IoffeTimeDistributionResult::IoffeTimeDistributionResult(
        const IoffeTimeDistributionResult &other) :
        Result<IoffeTimeKinematic>(other), m_realParts(other.m_realParts), m_imaginaryParts(
                other.m_imaginaryParts) {
}

IoffeTimeDistributionResult::~IoffeTimeDistributionResult() {
}

std::string IoffeTimeDistributionResult::toString() const {

    ElemUtils::Formatter formatter;

    formatter << '\n';
    formatter << Result::toString();
    formatter << '\n';

    std::map<GPDType::Type, PartonDistribution>::const_iterator it;

    for (it = m_realParts.begin(); it != m_realParts.end(); it++) {

        formatter << '\n';
        formatter << "Result: " << "Ioffe-time distribution of GPD "
                << GPDType(it->first).toString();
        formatter << '\n';
        formatter << "Real part:" << '\n';
        formatter << (it->second).toString();
        formatter << "Imaginary part:" << '\n';
        formatter << getImaginaryPart(it->first).toString();

        const std::map<QuarkFlavor::Type, QuarkDistribution>& quarks =
                (it->second).getQuarkDistributions();

        if (quarks.find(QuarkFlavor::UP) != quarks.end()
                && quarks.find(QuarkFlavor::DOWN) != quarks.end()) {

            std::complex<double> isovector = getIsovector(it->first);

            formatter << "Isovector (u - d): Re = " << isovector.real()
                    << " Im = " << isovector.imag() << '\n';
        }
    }

    return formatter.str();
}

void IoffeTimeDistributionResult::addDistribution(GPDType::Type gpdType,
        const PartonDistribution& realPart,
        const PartonDistribution& imaginaryPart) {

    if (m_realParts.find(gpdType) != m_realParts.end()) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter() << "Result for GPDType = "
                        << GPDType(gpdType).toString() << " exists");
    }

    m_realParts.insert(
            std::pair<GPDType::Type, PartonDistribution>(gpdType, realPart));
    m_imaginaryParts.insert(
            std::pair<GPDType::Type, PartonDistribution>(gpdType,
                    imaginaryPart));
}

const PartonDistribution& IoffeTimeDistributionResult::find(
        const std::map<GPDType::Type, PartonDistribution>& map,
        GPDType::Type gpdType, const std::string &funcName) const {

    std::map<GPDType::Type, PartonDistribution>::const_iterator it = map.find(
            gpdType);

    if (it == map.end()) {
        throw ElemUtils::CustomException(getClassName(), funcName,
                ElemUtils::Formatter()
                        << "Cannot find Ioffe-time distribution for GPDType = "
                        << GPDType(gpdType).toString());
    }

    return it->second;
}

const PartonDistribution& IoffeTimeDistributionResult::getRealPart(
        GPDType::Type gpdType) const {
    return find(m_realParts, gpdType, __func__);
}

const PartonDistribution& IoffeTimeDistributionResult::getImaginaryPart(
        GPDType::Type gpdType) const {
    return find(m_imaginaryParts, gpdType, __func__);
}

std::complex<double> IoffeTimeDistributionResult::getQuarkDistribution(
        GPDType::Type gpdType, QuarkFlavor::Type quarkFlavor) const {
    return std::complex<double>(
            getRealPart(gpdType).getQuarkDistribution(quarkFlavor).getQuarkDistribution(),
            getImaginaryPart(gpdType).getQuarkDistribution(quarkFlavor).getQuarkDistribution());
}

std::complex<double> IoffeTimeDistributionResult::getGluonDistribution(
        GPDType::Type gpdType) const {
    return std::complex<double>(
            getRealPart(gpdType).getGluonDistribution().getGluonDistribution(),
            getImaginaryPart(gpdType).getGluonDistribution().getGluonDistribution());
}

std::complex<double> IoffeTimeDistributionResult::getIsovector(
        GPDType::Type gpdType) const {
    return getQuarkDistribution(gpdType, QuarkFlavor::UP)
            - getQuarkDistribution(gpdType, QuarkFlavor::DOWN);
}

bool IoffeTimeDistributionResult::isAvailable(GPDType::Type gpdType) const {
    return (m_realParts.find(gpdType) != m_realParts.end());
}

std::vector<GPDType> IoffeTimeDistributionResult::listGPDTypeComputed() const {

    std::vector<GPDType> list;

    std::map<GPDType::Type, PartonDistribution>::const_iterator it;

    for (it = m_realParts.begin(); it != m_realParts.end(); ++it) {
        list.push_back(it->first);
    }

    return list;
}

const std::map<GPDType::Type, PartonDistribution>& IoffeTimeDistributionResult::getRealParts() const {
    return m_realParts;
}

const std::map<GPDType::Type, PartonDistribution>& IoffeTimeDistributionResult::getImaginaryParts() const {
    return m_imaginaryParts;
}

} /* namespace PARTONS */
