#include "../../include/partons/ModuleObjectFactory.h"

#include <ElementaryUtils/string_utils/Formatter.h>
#include <utility>

#include "../../include/partons/BaseObjectFactory.h"
#include "../../include/partons/modules/active_flavors_thresholds/ActiveFlavorsThresholdsModule.h"
#include "../../include/partons/modules/collinear_distribution/CollinearDistributionModule.h"
#include "../../include/partons/modules/convol_coeff_function/DDVCS/DDVCSConvolCoeffFunctionModule.h"
#include "../../include/partons/modules/convol_coeff_function/DVCS/DVCSConvolCoeffFunctionModule.h"
#include "../../include/partons/modules/convol_coeff_function/DVMP/DVMPConvolCoeffFunctionModule.h"
#include "../../include/partons/modules/convol_coeff_function/TCS/TCSConvolCoeffFunctionModule.h"
#include "../../include/partons/modules/convol_coeff_function/GAM2/GAM2ConvolCoeffFunctionModule.h"
#include "../../include/partons/modules/evolution/collinear_distribution/CollinearDistributionEvolutionModule.h"
#include "../../include/partons/modules/evolution/gpd/GPDEvolutionModule.h"
#include "../../include/partons/modules/gpd/GPDModule.h"
#include "../../include/partons/modules/gpd_subtraction_constant/GPDSubtractionConstantModule.h"
#include "../../include/partons/modules/ioffe_time/IoffeTimeDistributionModule.h"
#include "../../include/partons/modules/observable/DDVCS/DDVCSObservable.h"
#include "../../include/partons/modules/observable/DVCS/DVCSObservable.h"
#include "../../include/partons/modules/observable/DVMP/DVMPObservable.h"
#include "../../include/partons/modules/observable/TCS/TCSObservable.h"
#include "../../include/partons/modules/process/DDVCS/DDVCSProcessModule.h"
#include "../../include/partons/modules/process/DVCS/DVCSProcessModule.h"
#include "../../include/partons/modules/process/DVMP/DVMPProcessModule.h"
#include "../../include/partons/modules/process/GAM2/GAM2ProcessModule.h"
#include "../../include/partons/modules/process/TCS/TCSProcessModule.h"
#include "../../include/partons/modules/running_alpha_strong/RunningAlphaStrongModule.h"
#include "../../include/partons/modules/scales/DDVCS/DDVCSScalesModule.h"
#include "../../include/partons/modules/scales/DVCS/DVCSScalesModule.h"
#include "../../include/partons/modules/scales/TCS/TCSScalesModule.h"
#include "../../include/partons/modules/scales/DVMP/DVMPScalesModule.h"
#include "../../include/partons/modules/scales/GAM2/GAM2ScalesModule.h"
#include "../../include/partons/modules/xi_converter/DDVCS/DDVCSXiConverterModule.h"
#include "../../include/partons/modules/xi_converter/DVCS/DVCSXiConverterModule.h"
#include "../../include/partons/modules/xi_converter/DVMP/DVMPXiConverterModule.h"
#include "../../include/partons/modules/xi_converter/TCS/TCSXiConverterModule.h"
#include "../../include/partons/modules/xi_converter/GAM2/GAM2XiConverterModule.h"

namespace PARTONS {

ModuleObjectFactory::ModuleObjectFactory(BaseObjectFactory* pBaseObjectFactory) :
        BaseObject("ModuleObjectFactory"), m_pBaseObjectFactory(
                pBaseObjectFactory) {
}

ModuleObjectFactory::~ModuleObjectFactory() {

    std::map<unsigned int, ModuleObjectReference*>::iterator it;

    for (it = m_instantiatedModuleObject.begin();
            it != m_instantiatedModuleObject.end(); it++) {
        delete (it->second);
    }

    // m_pBaseObjectFactory pointer will be deleted by the Partons class
}

void ModuleObjectFactory::updateModulePointerReference(
        ModuleObject* pModuleObjectTarget, ModuleObject* pModuleObjectSource) {
    std::map<unsigned int, ModuleObjectReference*>::iterator m_it;

    // Check if source pointer is not NULL
    if (pModuleObjectSource != 0) {
        // If not, retrieve the referenced object from the map
        m_it = m_instantiatedModuleObject.find(
                pModuleObjectSource->getReferenceModuleId());

        // If there is no match
        if (m_it == m_instantiatedModuleObject.end()) {
            warn(__func__,
                    ElemUtils::Formatter()
                            << "Missing reference in m_pInstantiatedModuleObject for referenceObjectId = "
                            << pModuleObjectSource->getReferenceModuleId()
                            << " with className = "
                            << pModuleObjectSource->getClassName()
                            << " ; Check your code implementation maybe there is a problem with your pointer memory handling ; This missing pointer reference will be add to the ModuleObjectFactory instanciated object list");

            // Missing reference pointer need to be add to the ObjectModuleFactory instanciated object list.
            store(pModuleObjectSource);
        }

        // Increment the number of the reference to the source object ; because we will assign it to the target pointer.
        (m_it->second)->incCounter();
    }

    // If target pointer is not NULL.
    // It seems that it's referenced and we need to remove one reference to this object because we will assign from the source object.
    if (pModuleObjectTarget != 0) {
        m_it = m_instantiatedModuleObject.find(
                pModuleObjectTarget->getReferenceModuleId());

        if (m_it != m_instantiatedModuleObject.end()) {
            (m_it->second)->decCounter();

            // Add check if the number of reference for the target pointer reach 0 (zero)
            if ((m_it->second)->getNumberOfReference() == 0) {
                debug(__func__,
                        ElemUtils::Formatter() << "Object("
                                << (m_it->second)->getModuleObjectPointer()->getClassName()
                                << ") referenced with referenceObjectId("
                                << (m_it->second)->getModuleObjectPointer()->getReferenceModuleId()
                                << ") removed from ModuleObjectFactory instanciated object list");

                // Delete target object from memory
                if (m_it->second) {

                    delete (m_it->second);
                    (m_it->second) = 0;
                }

                m_instantiatedModuleObject.erase(m_it);
            }
        }
    }
}

void ModuleObjectFactory::store(ModuleObject* pModuleObject) {
    debug(__func__,
            ElemUtils::Formatter() << "Storing ModuleObject ("
                    << pModuleObject->getClassName()
                    << ") with referenceObjectId("
                    << pModuleObject->getReferenceModuleId() << ")");

    m_instantiatedModuleObject.insert(
            std::make_pair(pModuleObject->getReferenceModuleId(),
                    new ModuleObjectReference(pModuleObject)));
}

std::string ModuleObjectFactory::toString() const {
    ElemUtils::Formatter formatter;

    std::map<unsigned int, ModuleObjectReference*>::const_iterator it;

    for (it = m_instantiatedModuleObject.begin();
            it != m_instantiatedModuleObject.end(); it++) {
        formatter << (it->second)->toString() << '\n';
    }

    return formatter.str();
}

ModuleObject* ModuleObjectFactory::newModuleObject(unsigned int classId) {
    ModuleObject* pModuleObject =
            static_cast<ModuleObject*>(m_pBaseObjectFactory->newBaseObject(
                    classId));

    store(pModuleObject);

    return pModuleObject;
}

ModuleObject* ModuleObjectFactory::newModuleObject(
        const std::string& className) {
    ModuleObject* pModuleObject =
            static_cast<ModuleObject*>(m_pBaseObjectFactory->newBaseObject(
                    className));

    store(pModuleObject);

    return pModuleObject;
}

GPDEvolutionModule* ModuleObjectFactory::newGPDEvolutionModule(
        unsigned int classId) {
    return static_cast<GPDEvolutionModule*>(newModuleObject(classId));
}

GPDEvolutionModule* ModuleObjectFactory::newGPDEvolutionModule(
        const std::string& className) {
    return static_cast<GPDEvolutionModule*>(newModuleObject(className));
}

GPDModule* ModuleObjectFactory::newGPDModule(unsigned int classId) {
    return static_cast<GPDModule*>(newModuleObject(classId));
}

GPDModule* ModuleObjectFactory::newGPDModule(const std::string& className) {
    return static_cast<GPDModule*>(newModuleObject(className));
}

CollinearDistributionEvolutionModule* ModuleObjectFactory::newCollinearDistributionEvolutionModule(
        unsigned int classId) {
    return static_cast<CollinearDistributionEvolutionModule*>(newModuleObject(
            classId));
}

CollinearDistributionEvolutionModule* ModuleObjectFactory::newCollinearDistributionEvolutionModule(
        const std::string& className) {
    return static_cast<CollinearDistributionEvolutionModule*>(newModuleObject(
            className));
}

CollinearDistributionModule* ModuleObjectFactory::newCollinearDistributionModule(
        unsigned int classId) {
    return static_cast<CollinearDistributionModule*>(newModuleObject(classId));
}

CollinearDistributionModule* ModuleObjectFactory::newCollinearDistributionModule(
        const std::string& className) {
    return static_cast<CollinearDistributionModule*>(newModuleObject(className));
}

GPDSubtractionConstantModule* ModuleObjectFactory::newGPDSubtractionConstantModule(
        unsigned int classId) {
    return static_cast<GPDSubtractionConstantModule*>(newModuleObject(classId));
}

GPDSubtractionConstantModule* ModuleObjectFactory::newGPDSubtractionConstantModule(
        const std::string& className) {
    return static_cast<GPDSubtractionConstantModule*>(newModuleObject(className));
}

IoffeTimeDistributionModule* ModuleObjectFactory::newIoffeTimeDistributionModule(
        unsigned int classId) {
    return static_cast<IoffeTimeDistributionModule*>(newModuleObject(classId));
}

IoffeTimeDistributionModule* ModuleObjectFactory::newIoffeTimeDistributionModule(
        const std::string& className) {
    return static_cast<IoffeTimeDistributionModule*>(newModuleObject(className));
}

DVCSConvolCoeffFunctionModule* ModuleObjectFactory::newDVCSConvolCoeffFunctionModule(
        unsigned int classId) {
    return static_cast<DVCSConvolCoeffFunctionModule*>(newModuleObject(classId));
}

DVCSConvolCoeffFunctionModule* ModuleObjectFactory::newDVCSConvolCoeffFunctionModule(
        const std::string& className) {
    return static_cast<DVCSConvolCoeffFunctionModule*>(newModuleObject(
            className));
}

TCSConvolCoeffFunctionModule* ModuleObjectFactory::newTCSConvolCoeffFunctionModule(
        unsigned int classId) {
    return static_cast<TCSConvolCoeffFunctionModule*>(newModuleObject(classId));
}

TCSConvolCoeffFunctionModule* ModuleObjectFactory::newTCSConvolCoeffFunctionModule(
        const std::string& className) {
    return static_cast<TCSConvolCoeffFunctionModule*>(newModuleObject(className));
}

DVMPConvolCoeffFunctionModule* ModuleObjectFactory::newDVMPConvolCoeffFunctionModule(
        unsigned int classId) {
    return static_cast<DVMPConvolCoeffFunctionModule*>(newModuleObject(classId));
}

DVMPConvolCoeffFunctionModule* ModuleObjectFactory::newDVMPConvolCoeffFunctionModule(
        const std::string& className) {
    return static_cast<DVMPConvolCoeffFunctionModule*>(newModuleObject(
            className));
}

GAM2ConvolCoeffFunctionModule* ModuleObjectFactory::newGAM2ConvolCoeffFunctionModule(
        unsigned int classId) {
    return static_cast<GAM2ConvolCoeffFunctionModule*>(newModuleObject(classId));
}

DDVCSConvolCoeffFunctionModule* ModuleObjectFactory::newDDVCSConvolCoeffFunctionModule(
        unsigned int classId) {
    return static_cast<DDVCSConvolCoeffFunctionModule*>(newModuleObject(classId));
}

GAM2ConvolCoeffFunctionModule* ModuleObjectFactory::newGAM2ConvolCoeffFunctionModule(
        const std::string& className) {
    return static_cast<GAM2ConvolCoeffFunctionModule*>(newModuleObject(
            className));
}

DDVCSConvolCoeffFunctionModule* ModuleObjectFactory::newDDVCSConvolCoeffFunctionModule(
        const std::string& className) {
    return static_cast<DDVCSConvolCoeffFunctionModule*>(newModuleObject(
            className));
}

DVCSProcessModule* ModuleObjectFactory::newDVCSProcessModule(
        unsigned int classId) {
    return static_cast<DVCSProcessModule*>(newModuleObject(classId));
}

DVCSProcessModule* ModuleObjectFactory::newDVCSProcessModule(
        const std::string& className) {
    return static_cast<DVCSProcessModule*>(newModuleObject(className));
}

TCSProcessModule* ModuleObjectFactory::newTCSProcessModule(
        unsigned int classId) {
    return static_cast<TCSProcessModule*>(newModuleObject(classId));
}

TCSProcessModule* ModuleObjectFactory::newTCSProcessModule(
        const std::string& className) {
    return static_cast<TCSProcessModule*>(newModuleObject(className));
}

DVMPProcessModule* ModuleObjectFactory::newDVMPProcessModule(
        unsigned int classId) {
    return static_cast<DVMPProcessModule*>(newModuleObject(classId));
}

DVMPProcessModule* ModuleObjectFactory::newDVMPProcessModule(
        const std::string& className) {
    return static_cast<DVMPProcessModule*>(newModuleObject(className));
}

GAM2ProcessModule* ModuleObjectFactory::newGAM2ProcessModule(
        unsigned int classId) {
    return static_cast<GAM2ProcessModule*>(newModuleObject(classId));
}

GAM2ProcessModule* ModuleObjectFactory::newGAM2ProcessModule(
        const std::string& className) {
    return static_cast<GAM2ProcessModule*>(newModuleObject(className));
}

DDVCSProcessModule* ModuleObjectFactory::newDDVCSProcessModule(
        unsigned int classId) {
    return static_cast<DDVCSProcessModule*>(newModuleObject(classId));
}

DDVCSProcessModule* ModuleObjectFactory::newDDVCSProcessModule(
        const std::string& className) {
    return static_cast<DDVCSProcessModule*>(newModuleObject(className));
}

RunningAlphaStrongModule* ModuleObjectFactory::newRunningAlphaStrongModule(
        unsigned int classId) {
    return static_cast<RunningAlphaStrongModule*>(newModuleObject(classId));
}

RunningAlphaStrongModule* ModuleObjectFactory::newRunningAlphaStrongModule(
        const std::string& className) {
    return static_cast<RunningAlphaStrongModule*>(newModuleObject(className));
}

ActiveFlavorsThresholdsModule* ModuleObjectFactory::newActiveFlavorsThresholdsModule(
        unsigned int classId) {
    return static_cast<ActiveFlavorsThresholdsModule*>(newModuleObject(classId));
}

ActiveFlavorsThresholdsModule* ModuleObjectFactory::newActiveFlavorsThresholdsModule(
        const std::string &className) {
    return static_cast<ActiveFlavorsThresholdsModule*>(newModuleObject(
            className));
}

DVCSScalesModule* ModuleObjectFactory::newDVCSScalesModule(
        unsigned int classId) {
    return static_cast<DVCSScalesModule*>(newModuleObject(classId));
}

DVCSScalesModule* ModuleObjectFactory::newDVCSScalesModule(
        const std::string &className) {
    return static_cast<DVCSScalesModule*>(newModuleObject(className));
}

TCSScalesModule* ModuleObjectFactory::newTCSScalesModule(unsigned int classId) {
    return static_cast<TCSScalesModule*>(newModuleObject(classId));
}

TCSScalesModule* ModuleObjectFactory::newTCSScalesModule(
        const std::string &className) {
    return static_cast<TCSScalesModule*>(newModuleObject(className));
}

DVMPScalesModule* ModuleObjectFactory::newDVMPScalesModule(
        unsigned int classId) {
    return static_cast<DVMPScalesModule*>(newModuleObject(classId));
}

DVMPScalesModule* ModuleObjectFactory::newDVMPScalesModule(
        const std::string &className) {
    return static_cast<DVMPScalesModule*>(newModuleObject(className));
}

GAM2ScalesModule* ModuleObjectFactory::newGAM2ScalesModule(
        unsigned int classId) {
    return static_cast<GAM2ScalesModule*>(newModuleObject(classId));
}

GAM2ScalesModule* ModuleObjectFactory::newGAM2ScalesModule(
        const std::string &className) {
    return static_cast<GAM2ScalesModule*>(newModuleObject(className));
}

DDVCSScalesModule* ModuleObjectFactory::newDDVCSScalesModule(
        unsigned int classId) {
    return static_cast<DDVCSScalesModule*>(newModuleObject(classId));
}

DDVCSScalesModule* ModuleObjectFactory::newDDVCSScalesModule(
        const std::string &className) {
    return static_cast<DDVCSScalesModule*>(newModuleObject(className));

}

DVCSXiConverterModule* ModuleObjectFactory::newDVCSXiConverterModule(
        unsigned int classId) {
    return static_cast<DVCSXiConverterModule*>(newModuleObject(classId));
}

DVCSXiConverterModule* ModuleObjectFactory::newDVCSXiConverterModule(
        const std::string &className) {
    return static_cast<DVCSXiConverterModule*>(newModuleObject(className));
}

TCSXiConverterModule* ModuleObjectFactory::newTCSXiConverterModule(
        unsigned int classId) {
    return static_cast<TCSXiConverterModule*>(newModuleObject(classId));
}

TCSXiConverterModule* ModuleObjectFactory::newTCSXiConverterModule(
        const std::string &className) {
    return static_cast<TCSXiConverterModule*>(newModuleObject(className));
}

DVMPXiConverterModule* ModuleObjectFactory::newDVMPXiConverterModule(
        unsigned int classId) {
    return static_cast<DVMPXiConverterModule*>(newModuleObject(classId));
}

DVMPXiConverterModule* ModuleObjectFactory::newDVMPXiConverterModule(
        const std::string &className) {
    return static_cast<DVMPXiConverterModule*>(newModuleObject(className));
}

GAM2XiConverterModule* ModuleObjectFactory::newGAM2XiConverterModule(
        unsigned int classId) {
    return static_cast<GAM2XiConverterModule*>(newModuleObject(classId));
}

GAM2XiConverterModule* ModuleObjectFactory::newGAM2XiConverterModule(
        const std::string &className) {
    return static_cast<GAM2XiConverterModule*>(newModuleObject(className));
}

DDVCSXiConverterModule* ModuleObjectFactory::newDDVCSXiConverterModule(
        unsigned int classId) {
    return static_cast<DDVCSXiConverterModule*>(newModuleObject(classId));
}

DDVCSXiConverterModule* ModuleObjectFactory::newDDVCSXiConverterModule(
        const std::string &className) {
    return static_cast<DDVCSXiConverterModule*>(newModuleObject(className));
}

DVCSObservable* ModuleObjectFactory::newDVCSObservable(unsigned int classId) {
    return static_cast<DVCSObservable*>(newModuleObject(classId));
}

DVCSObservable* ModuleObjectFactory::newDVCSObservable(
        const std::string& className) {
    return static_cast<DVCSObservable*>(newModuleObject(className));
}

TCSObservable* ModuleObjectFactory::newTCSObservable(unsigned int classId) {
    return static_cast<TCSObservable*>(newModuleObject(classId));
}

TCSObservable* ModuleObjectFactory::newTCSObservable(
        const std::string& className) {
    return static_cast<TCSObservable*>(newModuleObject(className));
}

DVMPObservable* ModuleObjectFactory::newDVMPObservable(unsigned int classId) {
    return static_cast<DVMPObservable*>(newModuleObject(classId));
}

DVMPObservable* ModuleObjectFactory::newDVMPObservable(
        const std::string& className) {
    return static_cast<DVMPObservable*>(newModuleObject(className));
}

DDVCSObservable* ModuleObjectFactory::newDDVCSObservable(unsigned int classId) {
    return static_cast<DDVCSObservable*>(newModuleObject(classId));
}

DDVCSObservable* ModuleObjectFactory::newDDVCSObservable(
        const std::string& className) {
    return static_cast<DDVCSObservable*>(newModuleObject(className));
}

} /* namespace PARTONS */
