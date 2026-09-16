#include "../../include/partons/ServiceObjectRegistry.h"

#include <ElementaryUtils/logger/CustomException.h>

#include "../../include/partons/BaseObjectRegistry.h"
#include "../../include/partons/services/automation/AutomationService.h"
#include "../../include/partons/services/CollinearDistributionService.h"
#include "../../include/partons/services/DDVCSConvolCoeffFunctionService.h"
#include "../../include/partons/services/DDVCSObservableService.h"
#include "../../include/partons/services/DVCSConvolCoeffFunctionService.h"
#include "../../include/partons/services/DVCSObservableService.h"
#include "../../include/partons/services/DVMPConvolCoeffFunctionService.h"
#include "../../include/partons/services/DVMPObservableService.h"
#include "../../include/partons/services/GPDService.h"
#include "../../include/partons/services/IoffeTimeDistributionService.h"
#include "../../include/partons/services/hash_sum/CryptographicHashService.h"
#include "../../include/partons/services/TCSConvolCoeffFunctionService.h"
#include "../../include/partons/services/TCSObservableService.h"

namespace PARTONS {

ServiceObjectRegistry::ServiceObjectRegistry(
        BaseObjectRegistry* m_pBaseObjectRegistry) :
        m_pBaseObjectRegistry(m_pBaseObjectRegistry) {
}

ServiceObjectRegistry::~ServiceObjectRegistry() {
    // Nothing to destroy
    // m_pBaseObjectRegistry pointer will be deleted by the Partons class
}

ServiceObject* ServiceObjectRegistry::get(unsigned int classId) const {
    // Check if m_pBaseObjectRegistry is NULL pointer before trying to get related classId object from it.
    checkBaseObjectRegistryNullPointer();
    return static_cast<ServiceObject*>(ServiceObjectRegistry::m_pBaseObjectRegistry->get(
            classId));
}

ServiceObject* ServiceObjectRegistry::get(const std::string &className) const {
    // Check if m_pBaseObjectRegistry is NULL pointer before trying to get related classId object from it.
    checkBaseObjectRegistryNullPointer();
    return static_cast<ServiceObject*>(ServiceObjectRegistry::m_pBaseObjectRegistry->get(
            className));
}

void ServiceObjectRegistry::checkBaseObjectRegistryNullPointer() const {
    if (!m_pBaseObjectRegistry) {
        throw ElemUtils::CustomException("ServiceObjectRegistry", __func__,
                "m_pBaseObjectRegistry is NULL pointer, cannot run application !");
    }
}

GPDService* ServiceObjectRegistry::getGPDService() const {
    return static_cast<GPDService*>(get(GPDService::classId));
}

CollinearDistributionService* ServiceObjectRegistry::getCollinearDistributionService() const {
    return static_cast<CollinearDistributionService*>(get(
            CollinearDistributionService::classId));
}

IoffeTimeDistributionService* ServiceObjectRegistry::getIoffeTimeDistributionService() const {
    return static_cast<IoffeTimeDistributionService*>(get(
            IoffeTimeDistributionService::classId));
}

DVCSConvolCoeffFunctionService* ServiceObjectRegistry::getDVCSConvolCoeffFunctionService() const {
    return static_cast<DVCSConvolCoeffFunctionService*>(get(
            DVCSConvolCoeffFunctionService::classId));
}

TCSConvolCoeffFunctionService* ServiceObjectRegistry::getTCSConvolCoeffFunctionService() const {
    return static_cast<TCSConvolCoeffFunctionService*>(get(
            TCSConvolCoeffFunctionService::classId));
}

DVMPConvolCoeffFunctionService* ServiceObjectRegistry::getDVMPConvolCoeffFunctionService() const {
    return static_cast<DVMPConvolCoeffFunctionService*>(get(
            DVMPConvolCoeffFunctionService::classId));
}

DDVCSConvolCoeffFunctionService* ServiceObjectRegistry::getDDVCSConvolCoeffFunctionService() const {
    return static_cast<DDVCSConvolCoeffFunctionService*>(get(
            DDVCSConvolCoeffFunctionService::classId));
}

DVCSObservableService* ServiceObjectRegistry::getDVCSObservableService() const {
    return static_cast<DVCSObservableService*>(get(
            DVCSObservableService::classId));
}

TCSObservableService* ServiceObjectRegistry::getTCSObservableService() const {
    return static_cast<TCSObservableService*>(get(TCSObservableService::classId));
}

DVMPObservableService* ServiceObjectRegistry::getDVMPObservableService() const {
    return static_cast<DVMPObservableService*>(get(
            DVMPObservableService::classId));
}

DDVCSObservableService* ServiceObjectRegistry::getDDVCSObservableService() const {
    return static_cast<DDVCSObservableService*>(get(
            DDVCSObservableService::classId));
}

AutomationService* ServiceObjectRegistry::getAutomationService() const {
    return static_cast<AutomationService*>(m_pBaseObjectRegistry->get(
            AutomationService::classId));
}

CryptographicHashService* ServiceObjectRegistry::getCryptographicHashService() const {
    return static_cast<CryptographicHashService*>(m_pBaseObjectRegistry->get(
            CryptographicHashService::classId));
}

} /* namespace PARTONS */
