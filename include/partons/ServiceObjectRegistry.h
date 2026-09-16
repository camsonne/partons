#ifndef SERVICE_OBJECT_REGISTRY_H
#define SERVICE_OBJECT_REGISTRY_H

/**
 * @file ServiceObjectRegistry.h
 * @author Bryan BERTHOU (SPhN / CEA Saclay)
 * @date 01 July 2015
 * @version 1.0
 */

#include <string>

namespace PARTONS {
class AutomationService;
class BaseObjectRegistry;
class CollinearDistributionService;
class CryptographicHashService;
class DDVCSConvolCoeffFunctionService;
class DDVCSObservableService;
class DVCSConvolCoeffFunctionService;
class DVCSObservableService;
class DVMPConvolCoeffFunctionService;
class DVMPObservableService;
class GPDService;
class IoffeTimeDistributionService;
class ServiceObject;
class TCSConvolCoeffFunctionService;
class TCSObservableService;
} /* namespace PARTONS */

namespace PARTONS {

/**
 * @class ServiceObjectRegistry
 *
 * @brief
 */
class ServiceObjectRegistry {
public:
    virtual ~ServiceObjectRegistry();

    ServiceObject* get(unsigned int classId) const;
    ServiceObject* get(const std::string &className) const;

    GPDService* getGPDService() const;
    CollinearDistributionService* getCollinearDistributionService() const;

    /**
     * Get IoffeTimeDistributionService.
     */
    IoffeTimeDistributionService* getIoffeTimeDistributionService() const;
    DVCSConvolCoeffFunctionService* getDVCSConvolCoeffFunctionService() const;
    TCSConvolCoeffFunctionService* getTCSConvolCoeffFunctionService() const;
    DVMPConvolCoeffFunctionService* getDVMPConvolCoeffFunctionService() const;
    DDVCSConvolCoeffFunctionService* getDDVCSConvolCoeffFunctionService() const;
    DVCSObservableService* getDVCSObservableService() const;
    TCSObservableService* getTCSObservableService() const;
    DVMPObservableService* getDVMPObservableService() const;
    DDVCSObservableService* getDDVCSObservableService() const;
    AutomationService* getAutomationService() const;
    CryptographicHashService* getCryptographicHashService() const;

private:
    // To allow only Partons class to create a new instance of this class.
    // Used to avoid multiple singleton class and to avoid multithreading problem especially when getInstance() is called.
    // There is a bad behaviour with first instance initialization and mutex.
    friend class Partons;

    /**
     * Private default constructor to ensure the creation of a single instance of the class, managed by Parton's class.
     *
     * @param m_pBaseObjectRegistry
     */
    ServiceObjectRegistry(BaseObjectRegistry* m_pBaseObjectRegistry);

    BaseObjectRegistry* m_pBaseObjectRegistry;

    void checkBaseObjectRegistryNullPointer() const;
};

} /* namespace PARTONS */

#endif /* SERVICE_OBJECT_REGISTRY_H */
