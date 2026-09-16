#include "../../../include/partons/services/IoffeTimeDistributionService.h"

#include <ElementaryUtils/logger/CustomException.h>
#include <ElementaryUtils/parameters/GenericType.h>
#include <ElementaryUtils/parameters/Parameters.h>
#include <ElementaryUtils/PropertiesManager.h>
#include <ElementaryUtils/string_utils/Formatter.h>
#include <ElementaryUtils/string_utils/StringUtils.h>
#include <ElementaryUtils/thread/Packet.h>

#include "../../../include/partons/beans/automation/BaseObjectData.h"
#include "../../../include/partons/beans/automation/Task.h"
#include "../../../include/partons/beans/KinematicUtils.h"
#include "../../../include/partons/BaseObjectRegistry.h"
#include "../../../include/partons/modules/ioffe_time/IoffeTimeDistributionModule.h"
#include "../../../include/partons/ModuleObjectFactory.h"
#include "../../../include/partons/Partons.h"
#include "../../../include/partons/utils/VectorUtils.h"

namespace PARTONS {

const std::string IoffeTimeDistributionService::IOFFE_TIME_DISTRIBUTION_SERVICE_COMPUTE_SINGLE_KINEMATIC =
        "computeSingleKinematic";
const std::string IoffeTimeDistributionService::IOFFE_TIME_DISTRIBUTION_SERVICE_COMPUTE_MANY_KINEMATIC =
        "computeManyKinematic";

const std::string IoffeTimeDistributionService::PROPERTY_NAME_BATCH_SIZE =
        "ioffe_time.service.batch.size";
const unsigned int IoffeTimeDistributionService::DEFAULT_BATCH_SIZE = 1000;

const unsigned int IoffeTimeDistributionService::classId =
        Partons::getInstance()->getBaseObjectRegistry()->registerBaseObject(
                new IoffeTimeDistributionService(
                        "IoffeTimeDistributionService"));

IoffeTimeDistributionService::IoffeTimeDistributionService(
        const std::string &className) :
        ServiceObjectTyped<IoffeTimeKinematic, IoffeTimeDistributionResult>(
                className) {
}

IoffeTimeDistributionService::~IoffeTimeDistributionService() {
}

void IoffeTimeDistributionService::resolveObjectDependencies() {

    ServiceObjectTyped<IoffeTimeKinematic, IoffeTimeDistributionResult>::resolveObjectDependencies();

    //the property is optional, so that existing partons.properties files keep working
    try {
        m_batchSize = ElemUtils::GenericType(
                ElemUtils::PropertiesManager::getInstance()->getString(
                        PROPERTY_NAME_BATCH_SIZE)).toUInt();
    } catch (const std::exception &e) {
        m_batchSize = DEFAULT_BATCH_SIZE;
        info(__func__,
                ElemUtils::Formatter() << "Property " << PROPERTY_NAME_BATCH_SIZE
                        << " not found, using default batch size "
                        << DEFAULT_BATCH_SIZE);
    }
}

void IoffeTimeDistributionService::computeTask(Task &task) {

    ServiceObjectTyped<IoffeTimeKinematic, IoffeTimeDistributionResult>::computeTask(
            task);

    List<IoffeTimeDistributionResult> resultList;

    if (ElemUtils::StringUtils::equals(task.getFunctionName(),
            IoffeTimeDistributionService::IOFFE_TIME_DISTRIBUTION_SERVICE_COMPUTE_MANY_KINEMATIC)) {
        resultList.add(computeManyKinematicTask(task));
    }

    else if (ElemUtils::StringUtils::equals(task.getFunctionName(),
            IoffeTimeDistributionService::IOFFE_TIME_DISTRIBUTION_SERVICE_COMPUTE_SINGLE_KINEMATIC)) {
        resultList.add(computeSingleKinematicTask(task));
    }

    else if (!computeGeneralTask(task)) {
        errorUnknownMethod(task);
    }

    updateResultInfo(resultList, m_resultInfo);

    m_resultListBuffer = resultList;
}

IoffeTimeDistributionResult IoffeTimeDistributionService::computeSingleKinematic(
        const IoffeTimeKinematic &kinematic,
        IoffeTimeDistributionModule* pModule,
        const List<GPDType>& gpdTypeList) const {

    //get list of GPD types
    List<GPDType> finalGPDTypeList = getFinalGPDTypeList(pModule, gpdTypeList);

    //return
    return pModule->compute(kinematic, finalGPDTypeList);
}

List<IoffeTimeDistributionResult> IoffeTimeDistributionService::computeManyKinematic(
        const List<IoffeTimeKinematic> &kinematicList,
        IoffeTimeDistributionModule* pModule,
        const List<GPDType>& gpdTypeList) {

    //debug information
    debug(__func__,
            ElemUtils::Formatter() << kinematicList.size()
                    << " Ioffe-time kinematic(s) will be computed with "
                    << pModule->getClassName());

    //initialize
    List<IoffeTimeDistributionResult> results;
    List<ElemUtils::Packet> listOfPacket;
    List<GPDType> finalGPDTypeList = getFinalGPDTypeList(pModule, gpdTypeList);

    //if to be computed
    if (finalGPDTypeList.size() != 0) {

        //init thread
        initComputationalThread(pModule);

        //print info
        info(__func__, "Thread(s) running ...");

        //batch feature
        unsigned int i = 0;
        unsigned int j = 0;

        //divide to packets
        while (i != kinematicList.size()) {

            listOfPacket.clear();
            j = 0;

            while ((j != m_batchSize) && (i != kinematicList.size())) {
                ElemUtils::Packet packet;
                IoffeTimeKinematic kinematic;
                kinematic = kinematicList[i];
                packet << kinematic << finalGPDTypeList;
                listOfPacket.add(packet);
                i++;
                j++;
            }

            //add, launch and sort
            addTasks(listOfPacket);
            launchAllThreadAndWaitingFor();
            sortResultList();

            //print info
            info(__func__,
                    ElemUtils::Formatter() << "Kinematic(s) already computed : "
                            << i);

            //update result info
            updateResultInfo(getResultList(), m_resultInfo);

            //add to output
            results.add(getResultList());

            //clear buffer
            clearResultListBuffer();
        }

        //clear threads
        clearAllThread();

    } else {
        info(__func__,
                "Nothing to compute with your computation configuration ; there is no GPDType available");
    }

    return results;
}

IoffeTimeDistributionResult IoffeTimeDistributionService::computeSingleKinematicTask(
        Task& task) {

    //kinematics
    IoffeTimeKinematic kinematic = newKinematicFromTask(task);

    //GPD types
    List<GPDType> gpdTypeList = getGPDTypeListFromTask(task);

    //module
    IoffeTimeDistributionModule* pModule =
            newIoffeTimeDistributionModuleFromTask(task);

    //make computation
    IoffeTimeDistributionResult result = computeSingleKinematic(kinematic,
            pModule, gpdTypeList);

    //remove reference to pointer
    m_pModuleObjectFactory->updateModulePointerReference(pModule, 0);
    pModule = 0;

    //return
    return result;
}

List<IoffeTimeDistributionResult> IoffeTimeDistributionService::computeManyKinematicTask(
        Task& task) {

    //kinematics
    List<IoffeTimeKinematic> listOfKinematic = newListOfKinematicFromTask(task);

    //GPD types
    List<GPDType> gpdTypeList = getGPDTypeListFromTask(task);

    //module
    IoffeTimeDistributionModule* pModule =
            newIoffeTimeDistributionModuleFromTask(task);

    //make computation
    List<IoffeTimeDistributionResult> result = computeManyKinematic(
            listOfKinematic, pModule, gpdTypeList);

    //remove reference to pointer
    m_pModuleObjectFactory->updateModulePointerReference(pModule, 0);
    pModule = 0;

    //return
    return result;
}

List<GPDType> IoffeTimeDistributionService::getFinalGPDTypeList(
        IoffeTimeDistributionModule* pModule,
        const List<GPDType> &gpdTypeList) const {

    //get list of GPD types available
    List<GPDType> finalGPDTypeList =
            pModule->getListOfAvailableGPDTypeForComputation();

    //intersection between available GPDType and GPDType asked
    if (!gpdTypeList.isEmpty()) {
        finalGPDTypeList = VectorUtils::intersection(finalGPDTypeList,
                gpdTypeList);
    }

    //debug info
    debug(__func__,
            ElemUtils::Formatter() << finalGPDTypeList.size()
                    << " GPDType will be computed");

    //return
    return finalGPDTypeList;
}

IoffeTimeKinematic IoffeTimeDistributionService::newKinematicFromTask(
        const Task& task) const {

    IoffeTimeKinematic kinematic;

    if (ElemUtils::StringUtils::equals(
            task.getKinematicsData().getModuleClassName(),
            IoffeTimeKinematic::IOFFE_TIME_KINEMATIC_CLASS_NAME)) {
        kinematic.configure(task.getKinematicsData().getParameters());
    } else {
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter()
                        << "Missing object : <IoffeTimeKinematic> for method "
                        << task.getFunctionName());
    }

    return kinematic;
}

List<IoffeTimeKinematic> IoffeTimeDistributionService::newListOfKinematicFromTask(
        const Task& task) const {

    List<IoffeTimeKinematic> listOfKinematic;

    if (ElemUtils::StringUtils::equals(
            task.getKinematicsData().getModuleClassName(),
            IoffeTimeKinematic::IOFFE_TIME_KINEMATIC_CLASS_NAME)) {

        if (task.getKinematicsData().getParameters().isAvailable("file")) {
            listOfKinematic = KinematicUtils().getIoffeTimeKinematicFromFile(
                    task.getKinematicsData().getParameters().getLastAvailable().getString());
        } else {
            throw ElemUtils::CustomException(getClassName(), __func__,
                    ElemUtils::Formatter()
                            << "Missing parameter file in object <IoffeTimeKinematic> for method "
                            << task.getFunctionName());
        }
    } else {
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter()
                        << "Missing object : <IoffeTimeKinematic> for method "
                        << task.getFunctionName());
    }

    return listOfKinematic;
}

IoffeTimeDistributionModule* IoffeTimeDistributionService::newIoffeTimeDistributionModuleFromTask(
        const Task& task) const {

    //initialize
    IoffeTimeDistributionModule* pModule = 0;

    //check if available
    if (ElemUtils::StringUtils::equals(
            task.getModuleComputationConfiguration().getModuleType(),
            IoffeTimeDistributionModule::IOFFE_TIME_DISTRIBUTION_MODULE_CLASS_NAME)) {

        //configure
        pModule =
                Partons::getInstance()->getModuleObjectFactory()->newIoffeTimeDistributionModule(
                        task.getModuleComputationConfiguration().getModuleClassName());

        pModule->configure(
                task.getModuleComputationConfiguration().getParameters());

        pModule->prepareSubModules(
                task.getModuleComputationConfiguration().getSubModules());
    } else {
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter() << "You have not provided any "
                        << IoffeTimeDistributionModule::IOFFE_TIME_DISTRIBUTION_MODULE_CLASS_NAME);
    }

    //return
    return pModule;
}

} /* namespace PARTONS */
