#include "../../../../include/partons/modules/ioffe_time/IoffeTimeDistributionModule.h"

#include <ElementaryUtils/logger/CustomException.h>
#include <ElementaryUtils/string_utils/Formatter.h>
#include <ElementaryUtils/thread/Packet.h>
#include <NumA/functor/one_dimension/Functor1D.h>
#include <NumA/integration/one_dimension/Integrator1D.h>
#include <NumA/integration/one_dimension/IntegratorType1D.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

#include "../../../../include/partons/beans/channel/ChannelType.h"
#include "../../../../include/partons/beans/gpd/GPDKinematic.h"
#include "../../../../include/partons/beans/parton_distribution/GluonDistribution.h"
#include "../../../../include/partons/beans/parton_distribution/QuarkDistribution.h"
#include "../../../../include/partons/beans/QuarkFlavor.h"
#include "../../../../include/partons/modules/gpd/GPDModule.h"
#include "../../../../include/partons/ModuleObjectFactory.h"
#include "../../../../include/partons/Partons.h"
#include "../../../../include/partons/services/IoffeTimeDistributionService.h"
#include "../../../../include/partons/ServiceObjectRegistry.h"
#include "../../../../include/partons/utils/type/PhysicalType.h"
#include "../../../../include/partons/utils/type/PhysicalUnit.h"
#include "../../../../include/partons/utils/VectorUtils.h"

namespace PARTONS {

const std::string IoffeTimeDistributionModule::IOFFE_TIME_DISTRIBUTION_MODULE_CLASS_NAME =
        "IoffeTimeDistributionModule";

IoffeTimeDistributionModule::IoffeTimeDistributionModule(
        const std::string &className) :
        ModuleObject(className, ChannelType::UNDEFINED), MathIntegratorModule(), m_nu(
                0.), m_z2(0.), m_xi(0.), m_t(0.), m_MuF2(0.), m_MuR2(0.), m_currentGPDComputeType(
                GPDType::UNDEFINED), m_pGPDModule(0), m_pIntegrand(0) {

    //default integration routine: adaptive Gauss-Kronrod, well suited for oscillatory integrands
    setIntegrator(NumA::IntegratorType1D::GK21_ADAPTIVE);

    //by default all twist-2 GPDs can be transformed
    m_listGPDTypeAvailable.add(GPDType::H);
    m_listGPDTypeAvailable.add(GPDType::E);
    m_listGPDTypeAvailable.add(GPDType::Ht);
    m_listGPDTypeAvailable.add(GPDType::Et);
    m_listGPDTypeAvailable.add(GPDType::HTrans);
    m_listGPDTypeAvailable.add(GPDType::ETrans);
    m_listGPDTypeAvailable.add(GPDType::HtTrans);
    m_listGPDTypeAvailable.add(GPDType::EtTrans);

    initFunctorsForIntegrations();
}

IoffeTimeDistributionModule::IoffeTimeDistributionModule(
        const IoffeTimeDistributionModule &other) :
        ModuleObject(other), MathIntegratorModule(other), m_nu(other.m_nu), m_z2(
                other.m_z2), m_xi(other.m_xi), m_t(other.m_t), m_MuF2(
                other.m_MuF2), m_MuR2(other.m_MuR2), m_currentGPDComputeType(
                other.m_currentGPDComputeType), m_listGPDTypeAvailable(
                other.m_listGPDTypeAvailable), m_pGPDModule(0), m_pIntegrand(0) {

    if (other.m_pGPDModule != 0) {
        m_pGPDModule = m_pModuleObjectFactory->cloneModuleObject(
                other.m_pGPDModule);
    }

    initFunctorsForIntegrations();
}

IoffeTimeDistributionModule::~IoffeTimeDistributionModule() {

    if (m_pGPDModule != 0) {
        setGPDModule(0);
        m_pGPDModule = 0;
    }

    if (m_pIntegrand) {
        delete m_pIntegrand;
        m_pIntegrand = 0;
    }
}

void IoffeTimeDistributionModule::initFunctorsForIntegrations() {
    m_pIntegrand = NumA::Integrator1D::newIntegrationFunctor(this,
            &IoffeTimeDistributionModule::integrand);
}

std::string IoffeTimeDistributionModule::toString() const {
    return ModuleObject::toString();
}

void IoffeTimeDistributionModule::resolveObjectDependencies() {
    ModuleObject::resolveObjectDependencies();
}

void IoffeTimeDistributionModule::run() {

    try {

        //get service
        IoffeTimeDistributionService* pService =
                Partons::getInstance()->getServiceObjectRegistry()->getIoffeTimeDistributionService();

        //run until empty
        while (!(pService->isEmptyTaskQueue())) {

            //kinematics
            IoffeTimeKinematic kinematic;

            //list of GPD types
            List<GPDType> gpdTypeList;

            //set
            ElemUtils::Packet packet = pService->popTaskFormQueue();
            packet >> kinematic;
            packet >> gpdTypeList;

            //debug information
            debug(__func__,
                    ElemUtils::Formatter() << "objectId = " << getObjectId()
                            << " " << kinematic.toString());

            //object to be returned
            IoffeTimeDistributionResult result = compute(kinematic,
                    gpdTypeList);

            //helpful to sort later
            result.setIndexId(kinematic.getIndexId());

            //add
            pService->add(result);
        }
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void IoffeTimeDistributionModule::configure(
        const ElemUtils::Parameters &parameters) {
    ModuleObject::configure(parameters);
    configureIntegrator(parameters);
}

void IoffeTimeDistributionModule::prepareSubModules(
        const std::map<std::string, BaseObjectData>& subModulesData) {

    //run for mother
    ModuleObject::prepareSubModules(subModulesData);

    //search for GPD module
    std::map<std::string, BaseObjectData>::const_iterator it =
            subModulesData.find(GPDModule::GPD_MODULE_CLASS_NAME);

    if (it != subModulesData.end()) {

        if (m_pGPDModule != 0) {
            setGPDModule(0);
            m_pGPDModule = 0;
        }

        m_pGPDModule =
                Partons::getInstance()->getModuleObjectFactory()->newGPDModule(
                        (it->second).getModuleClassName());

        info(__func__,
                ElemUtils::Formatter() << "Configured with GPDModule = "
                        << m_pGPDModule->getClassName());

        m_pGPDModule->configure((it->second).getParameters());
        m_pGPDModule->prepareSubModules((it->second).getSubModules());
    } else {
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter() << getClassName()
                        << " is GPDModule dependent and you have not provided one");
    }
}

IoffeTimeDistributionResult IoffeTimeDistributionModule::compute(
        const IoffeTimeKinematic &kinematic, const List<GPDType>& gpdType) {

    //reset kinematics (virtuality)
    setKinematics(kinematic);

    //execute last child function (virtuality)
    initModule();

    //execute last child function (virtuality)
    isModuleWellConfigured();

    //GPD types to be computed
    List<GPDType> availableTypes = getListOfAvailableGPDTypeForComputation();
    List<GPDType> typesToCompute = gpdType;

    if (typesToCompute.isEmpty()) {
        typesToCompute = availableTypes;
    }

    //object to be returned
    IoffeTimeDistributionResult result(kinematic);

    //loop over GPD types
    for (size_t i = 0; i < typesToCompute.size(); i++) {

        //check if available
        bool isAvailable = false;

        for (size_t j = 0; j < availableTypes.size(); j++) {
            if (availableTypes[j].getType() == typesToCompute[i].getType()) {
                isAvailable = true;
                break;
            }
        }

        if (!isAvailable) {
            throw ElemUtils::CustomException(getClassName(), __func__,
                    ElemUtils::Formatter() << "GPD("
                            << GPDType(typesToCompute[i]).toString()
                            << ") is not available for this model");
        }

        //set current type and reset cache
        m_currentGPDComputeType = typesToCompute[i].getType();
        m_GPDCache.clear();

        //evaluate
        PartonDistribution realPart;
        PartonDistribution imaginaryPart;

        computeDistribution(m_currentGPDComputeType, realPart, imaginaryPart);

        //add
        result.addDistribution(m_currentGPDComputeType, realPart,
                imaginaryPart);
    }

    //free memory
    m_GPDCache.clear();

    //set module name
    result.setComputationModuleName(getClassName());

    //return
    return result;
}

List<GPDType> IoffeTimeDistributionModule::getListOfAvailableGPDTypeForComputation() const {

    if (m_pGPDModule == 0) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                "GPD module not set");
    }

    return VectorUtils::intersection(m_listGPDTypeAvailable,
            m_pGPDModule->getListOfAvailableGPDTypeForComputation());
}

GPDModule* IoffeTimeDistributionModule::getGPDModule() const {
    return m_pGPDModule;
}

void IoffeTimeDistributionModule::setGPDModule(GPDModule* pGPDModule) {
    m_pModuleObjectFactory->updateModulePointerReference(m_pGPDModule,
            pGPDModule);
    m_pGPDModule = pGPDModule;
}

void IoffeTimeDistributionModule::setKinematics(
        const IoffeTimeKinematic& kinematic) {
    m_nu = kinematic.getNu().makeSameUnitAs(PhysicalUnit::NONE).getValue();
    m_z2 = kinematic.getZ2().makeSameUnitAs(PhysicalUnit::GEVm2).getValue();
    m_xi = kinematic.getXi().makeSameUnitAs(PhysicalUnit::NONE).getValue();
    m_t = kinematic.getT().makeSameUnitAs(PhysicalUnit::GEV2).getValue();
    m_MuF2 = kinematic.getMuF2().makeSameUnitAs(PhysicalUnit::GEV2).getValue();
    m_MuR2 = kinematic.getMuR2().makeSameUnitAs(PhysicalUnit::GEV2).getValue();
}

void IoffeTimeDistributionModule::initModule() {
}

void IoffeTimeDistributionModule::isModuleWellConfigured() {

    if (m_pGPDModule == 0) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                "m_pGPDModule is NULL");
    }

    if (getMathIntegrator() == 0) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                "Integration routine not set");
    }

    if (m_xi < 0. || m_xi > 1.) {
        warn(__func__,
                ElemUtils::Formatter() << "Input value of xi = " << m_xi
                        << " does not lay between 0 and 1.");
    }

    if (m_t > 0.) {
        warn(__func__,
                ElemUtils::Formatter() << "Input value of t = " << m_t
                        << " is not <= 0.");
    }

    if (m_z2 < 0.) {
        warn(__func__,
                ElemUtils::Formatter() << "Input value of z2 = " << m_z2
                        << " is not >= 0.");
    }

    if (m_MuF2 <= 0.) {
        warn(__func__,
                ElemUtils::Formatter() << "Input value of muF2 = " << m_MuF2
                        << " is not > 0.");
    }

    if (m_MuR2 <= 0.) {
        warn(__func__,
                ElemUtils::Formatter() << "Input value of muR2 = " << m_MuR2
                        << " is not > 0.");
    }
}

void IoffeTimeDistributionModule::computeDistribution(GPDType::Type gpdType,
        PartonDistribution& realPart, PartonDistribution& imaginaryPart) {
    transform(gpdType, realPart, imaginaryPart);
}

double IoffeTimeDistributionModule::kernelRealPart(double x, bool isQuark) {
    return cos(x * m_nu);
}

double IoffeTimeDistributionModule::kernelImaginaryPart(double x,
        bool isQuark) {
    return sin(x * m_nu);
}

const PartonDistribution& IoffeTimeDistributionModule::evaluateGPD(double x) {

    std::map<double, PartonDistribution>::const_iterator it = m_GPDCache.find(
            x);

    if (it == m_GPDCache.end()) {
        it = m_GPDCache.insert(
                std::pair<double, PartonDistribution>(x,
                        m_pGPDModule->compute(
                                GPDKinematic(x, m_xi, m_t, m_MuF2, m_MuR2),
                                m_currentGPDComputeType))).first;
    }

    return it->second;
}

double IoffeTimeDistributionModule::integrand(double x,
        std::vector<double>& params) {

    int partonCode = static_cast<int>(params.at(0));
    int componentCode = static_cast<int>(params.at(1));
    int fieldCode = static_cast<int>(params.at(2));

    bool isQuark = (partonCode != GLUON);

    //kernel
    double kernel =
            (componentCode == REAL_PART) ?
                    kernelRealPart(x, isQuark) : kernelImaginaryPart(x, isQuark);

    //GPD
    const PartonDistribution& partonDistribution = evaluateGPD(x);

    double value = 0.;

    if (!isQuark) {
        value = partonDistribution.getGluonDistribution().getGluonDistribution();
    } else {

        const QuarkDistribution& quarkDistribution =
                partonDistribution.getQuarkDistribution(
                        static_cast<QuarkFlavor::Type>(partonCode));

        switch (fieldCode) {
        case FULL:
            value = quarkDistribution.getQuarkDistribution();
            break;
        case PLUS:
            value = quarkDistribution.getQuarkDistributionPlus();
            break;
        case MINUS:
            value = quarkDistribution.getQuarkDistributionMinus();
            break;
        default:
            throw ElemUtils::CustomException(getClassName(), __func__,
                    ElemUtils::Formatter() << "Unknown field code "
                            << fieldCode);
        }
    }

    return kernel * value;
}

double IoffeTimeDistributionModule::integrateWithBreakPoints(double xMin,
        double xMax, std::vector<double>& params) {

    //points where GPDs are usually not smooth
    std::vector<double> points;

    points.push_back(xMin);

    double candidates[3] = { -m_xi, 0., m_xi };

    for (int i = 0; i < 3; i++) {
        if (candidates[i] > xMin && candidates[i] < xMax) {
            points.push_back(candidates[i]);
        }
    }

    points.push_back(xMax);

    std::sort(points.begin(), points.end());

    //integrate piecewise
    double result = 0.;

    for (size_t i = 0; i + 1 < points.size(); i++) {
        if (points[i + 1] - points[i] > 0.) {
            result += integrate(m_pIntegrand, points[i], points[i + 1], params);
        }
    }

    return result;
}

void IoffeTimeDistributionModule::transform(GPDType::Type gpdType,
        PartonDistribution& realPart, PartonDistribution& imaginaryPart) {

    //parameters of the integrand
    std::vector<double> params(3, 0.);

    //gluons
    params[0] = GLUON;
    params[2] = FULL;

    params[1] = REAL_PART;
    double gluonRe = integrateWithBreakPoints(-1., 1., params);

    params[1] = IMAGINARY_PART;
    double gluonIm = integrateWithBreakPoints(-1., 1., params);

    realPart.setGluonDistribution(GluonDistribution(gluonRe));
    imaginaryPart.setGluonDistribution(GluonDistribution(gluonIm));

    //quark flavors provided by the GPD model (taken from the cache filled by the gluon integrals)
    std::vector<QuarkFlavor::Type> flavors;

    if (!m_GPDCache.empty()) {
        flavors = m_GPDCache.begin()->second.listTypeOfQuarkFlavor();
    }

    //loop over flavors
    for (size_t i = 0; i < flavors.size(); i++) {

        params[0] = static_cast<double>(flavors[i]);

        //full range: quarks and antiquarks
        params[2] = FULL;

        params[1] = REAL_PART;
        double fullRe = integrateWithBreakPoints(-1., 1., params);

        params[1] = IMAGINARY_PART;
        double fullIm = integrateWithBreakPoints(-1., 1., params);

        //singlet combination over x in [0, 1]
        params[2] = PLUS;

        params[1] = REAL_PART;
        double plusRe = integrateWithBreakPoints(0., 1., params);

        params[1] = IMAGINARY_PART;
        double plusIm = integrateWithBreakPoints(0., 1., params);

        //non-singlet combination over x in [0, 1]
        params[2] = MINUS;

        params[1] = REAL_PART;
        double minusRe = integrateWithBreakPoints(0., 1., params);

        params[1] = IMAGINARY_PART;
        double minusIm = integrateWithBreakPoints(0., 1., params);

        //store
        realPart.addQuarkDistribution(
                QuarkDistribution(flavors[i], fullRe, plusRe, minusRe));
        imaginaryPart.addQuarkDistribution(
                QuarkDistribution(flavors[i], fullIm, plusIm, minusIm));
    }
}

} /* namespace PARTONS */
