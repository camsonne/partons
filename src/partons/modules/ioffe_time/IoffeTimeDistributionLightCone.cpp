#include "../../../../include/partons/modules/ioffe_time/IoffeTimeDistributionLightCone.h"

#include "../../../../include/partons/BaseObjectRegistry.h"

namespace PARTONS {

const unsigned int IoffeTimeDistributionLightCone::classId =
        BaseObjectRegistry::getInstance()->registerBaseObject(
                new IoffeTimeDistributionLightCone(
                        "IoffeTimeDistributionLightCone"));

IoffeTimeDistributionLightCone::IoffeTimeDistributionLightCone(
        const std::string &className) :
        IoffeTimeDistributionModule(className) {
}

IoffeTimeDistributionLightCone::IoffeTimeDistributionLightCone(
        const IoffeTimeDistributionLightCone &other) :
        IoffeTimeDistributionModule(other) {
}

IoffeTimeDistributionLightCone::~IoffeTimeDistributionLightCone() {
}

IoffeTimeDistributionLightCone* IoffeTimeDistributionLightCone::clone() const {
    return new IoffeTimeDistributionLightCone(*this);
}

} /* namespace PARTONS */
