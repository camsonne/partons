#include "../../../../include/partons/modules/ioffe_time/IoffeTimeDistributionPseudoNLO.h"

#include <ElementaryUtils/logger/CustomException.h>
#include <ElementaryUtils/parameters/GenericType.h>
#include <ElementaryUtils/string_utils/Formatter.h>
#include <ElementaryUtils/string_utils/StringUtils.h>
#include <NumA/functor/one_dimension/Functor1D.h>
#include <NumA/integration/one_dimension/Integrator1D.h>
#include <NumA/integration/one_dimension/IntegratorType1D.h>
#include <cmath>

#include "../../../../include/partons/BaseObjectRegistry.h"
#include "../../../../include/partons/FundamentalPhysicalConstants.h"
#include "../../../../include/partons/modules/running_alpha_strong/RunningAlphaStrongModule.h"
#include "../../../../include/partons/ModuleObjectFactory.h"
#include "../../../../include/partons/Partons.h"

namespace PARTONS {

namespace {

const double COLOR_FACTOR_CF = 4. / 3.; ///< Casimir of the fundamental representation of SU(3).
const double EULER_GAMMA = 0.57721566490153286; ///< Euler-Mascheroni constant.

} /* namespace */

const unsigned int IoffeTimeDistributionPseudoNLO::classId =
        BaseObjectRegistry::getInstance()->registerBaseObject(
                new IoffeTimeDistributionPseudoNLO(
                        "IoffeTimeDistributionPseudoNLO"));

const std::string IoffeTimeDistributionPseudoNLO::PARAM_NAME_ALPHA_S =
        "alphaS";
const std::string IoffeTimeDistributionPseudoNLO::PARAM_NAME_MATCHING_KERNEL =
        "matchingKernel";

IoffeTimeDistributionPseudoNLO::IoffeTimeDistributionPseudoNLO(
        const std::string &className) :
        IoffeTimeDistributionModule(className), m_alphaS(0.), m_isAlphaSFixed(
                false), m_pRunningAlphaStrongModule(0), m_matchingKernelType(
                AUTO), m_activeKernelType(UNPOLARIZED), m_alphaSUsed(0.), m_prefactor(
                0.), m_logZ2Mu2(0.), m_pMatchingIntegrator(0), m_pMatchingIntegrand(
                0) {

    m_pMatchingIntegrator = NumA::Integrator1D::newIntegrator(
            NumA::IntegratorType1D::GK21_ADAPTIVE);

    initFunctorsForIntegrations();
}

IoffeTimeDistributionPseudoNLO::IoffeTimeDistributionPseudoNLO(
        const IoffeTimeDistributionPseudoNLO &other) :
        IoffeTimeDistributionModule(other), m_alphaS(other.m_alphaS), m_isAlphaSFixed(
                other.m_isAlphaSFixed), m_pRunningAlphaStrongModule(0), m_matchingKernelType(
                other.m_matchingKernelType), m_activeKernelType(
                other.m_activeKernelType), m_alphaSUsed(other.m_alphaSUsed), m_prefactor(
                other.m_prefactor), m_logZ2Mu2(other.m_logZ2Mu2), m_pMatchingIntegrator(
                0), m_pMatchingIntegrand(0) {

    if (other.m_pRunningAlphaStrongModule != 0) {
        m_pRunningAlphaStrongModule = m_pModuleObjectFactory->cloneModuleObject(
                other.m_pRunningAlphaStrongModule);
    }

    if (other.m_pMatchingIntegrator != 0) {
        m_pMatchingIntegrator = other.m_pMatchingIntegrator->clone();
    } else {
        m_pMatchingIntegrator = NumA::Integrator1D::newIntegrator(
                NumA::IntegratorType1D::GK21_ADAPTIVE);
    }

    initFunctorsForIntegrations();
}

IoffeTimeDistributionPseudoNLO::~IoffeTimeDistributionPseudoNLO() {

    if (m_pRunningAlphaStrongModule != 0) {
        setRunningAlphaStrongModule(0);
        m_pRunningAlphaStrongModule = 0;
    }

    if (m_pMatchingIntegrator) {
        delete m_pMatchingIntegrator;
        m_pMatchingIntegrator = 0;
    }

    if (m_pMatchingIntegrand) {
        delete m_pMatchingIntegrand;
        m_pMatchingIntegrand = 0;
    }
}

void IoffeTimeDistributionPseudoNLO::initFunctorsForIntegrations() {
    m_pMatchingIntegrand = NumA::Integrator1D::newIntegrationFunctor(this,
            &IoffeTimeDistributionPseudoNLO::matchingIntegrand);
}

IoffeTimeDistributionPseudoNLO* IoffeTimeDistributionPseudoNLO::clone() const {
    return new IoffeTimeDistributionPseudoNLO(*this);
}

std::string IoffeTimeDistributionPseudoNLO::toString() const {

    ElemUtils::Formatter formatter;

    formatter << IoffeTimeDistributionModule::toString() << '\n';
    formatter << "matching kernel: "
            << matchingKernelTypeToString(m_matchingKernelType) << '\n';

    if (m_isAlphaSFixed) {
        formatter << "alpha_s: fixed to " << m_alphaS << '\n';
    } else if (m_pRunningAlphaStrongModule != 0) {
        formatter << "alpha_s: from "
                << m_pRunningAlphaStrongModule->getClassName() << '\n';
    } else {
        formatter << "alpha_s: not set" << '\n';
    }

    return formatter.str();
}

void IoffeTimeDistributionPseudoNLO::configure(
        const ElemUtils::Parameters &parameters) {

    //run for mother
    IoffeTimeDistributionModule::configure(parameters);

    //alpha_s
    if (parameters.isAvailable(
            IoffeTimeDistributionPseudoNLO::PARAM_NAME_ALPHA_S)) {

        setAlphaS(parameters.getLastAvailable().toDouble());

        info(__func__,
                ElemUtils::Formatter()
                        << IoffeTimeDistributionPseudoNLO::PARAM_NAME_ALPHA_S
                        << " configured with value = " << m_alphaS);
    }

    //matching kernel
    if (parameters.isAvailable(
            IoffeTimeDistributionPseudoNLO::PARAM_NAME_MATCHING_KERNEL)) {

        //try to set by standard way
        try {
            m_matchingKernelType =
                    static_cast<MatchingKernelType>(parameters.getLastAvailable().toUInt());
        }
        //if an exception is raised it means that it's a string configuration value
        catch (const std::exception &e) {
            m_matchingKernelType = matchingKernelTypeFromString(
                    parameters.getLastAvailable().getString());
        }

        info(__func__,
                ElemUtils::Formatter()
                        << IoffeTimeDistributionPseudoNLO::PARAM_NAME_MATCHING_KERNEL
                        << " configured with value = "
                        << matchingKernelTypeToString(m_matchingKernelType));
    }
}

void IoffeTimeDistributionPseudoNLO::prepareSubModules(
        const std::map<std::string, BaseObjectData>& subModulesData) {

    //run for mother (GPD module)
    IoffeTimeDistributionModule::prepareSubModules(subModulesData);

    //search for running alpha_s module (optional)
    std::map<std::string, BaseObjectData>::const_iterator it =
            subModulesData.find(
                    RunningAlphaStrongModule::RUNNING_ALPHA_STRONG_MODULE_CLASS_NAME);

    if (it != subModulesData.end()) {

        if (m_pRunningAlphaStrongModule != 0) {
            setRunningAlphaStrongModule(0);
            m_pRunningAlphaStrongModule = 0;
        }

        RunningAlphaStrongModule* pModule =
                Partons::getInstance()->getModuleObjectFactory()->newRunningAlphaStrongModule(
                        (it->second).getModuleClassName());

        info(__func__,
                ElemUtils::Formatter()
                        << "Configured with RunningAlphaStrongModule = "
                        << pModule->getClassName());

        pModule->configure((it->second).getParameters());
        pModule->prepareSubModules((it->second).getSubModules());

        setRunningAlphaStrongModule(pModule);
    }
}

IoffeTimeDistributionPseudoNLO::MatchingKernelType IoffeTimeDistributionPseudoNLO::matchingKernelTypeFromString(
        const std::string &name) {

    if (ElemUtils::StringUtils::equals(name, "auto"))
        return AUTO;
    if (ElemUtils::StringUtils::equals(name, "unpolarized"))
        return UNPOLARIZED;
    if (ElemUtils::StringUtils::equals(name, "helicity"))
        return HELICITY;
    if (ElemUtils::StringUtils::equals(name, "transversity"))
        return TRANSVERSITY;

    throw ElemUtils::CustomException("IoffeTimeDistributionPseudoNLO",
            __func__,
            ElemUtils::Formatter() << "Unknown matching kernel type: " << name
                    << " (allowed: auto, unpolarized, helicity, transversity)");
}

std::string IoffeTimeDistributionPseudoNLO::matchingKernelTypeToString(
        MatchingKernelType type) {

    switch (type) {
    case AUTO:
        return "auto";
    case UNPOLARIZED:
        return "unpolarized";
    case HELICITY:
        return "helicity";
    case TRANSVERSITY:
        return "transversity";
    default:
        return "UNDEFINED";
    }
}

double IoffeTimeDistributionPseudoNLO::getAlphaS() const {
    return m_alphaS;
}

void IoffeTimeDistributionPseudoNLO::setAlphaS(double alphaS) {
    m_alphaS = alphaS;
    m_isAlphaSFixed = true;
}

bool IoffeTimeDistributionPseudoNLO::isAlphaSFixed() const {
    return m_isAlphaSFixed;
}

RunningAlphaStrongModule* IoffeTimeDistributionPseudoNLO::getRunningAlphaStrongModule() const {
    return m_pRunningAlphaStrongModule;
}

void IoffeTimeDistributionPseudoNLO::setRunningAlphaStrongModule(
        RunningAlphaStrongModule* pRunningAlphaStrongModule) {

    m_pModuleObjectFactory->updateModulePointerReference(
            m_pRunningAlphaStrongModule, pRunningAlphaStrongModule);
    m_pRunningAlphaStrongModule = pRunningAlphaStrongModule;

    if (m_pRunningAlphaStrongModule != 0) {
        m_isAlphaSFixed = false;
    }
}

IoffeTimeDistributionPseudoNLO::MatchingKernelType IoffeTimeDistributionPseudoNLO::getMatchingKernelType() const {
    return m_matchingKernelType;
}

void IoffeTimeDistributionPseudoNLO::setMatchingKernelType(
        MatchingKernelType matchingKernelType) {
    m_matchingKernelType = matchingKernelType;
}

double IoffeTimeDistributionPseudoNLO::getAlphaSUsed() const {
    return m_alphaSUsed;
}

void IoffeTimeDistributionPseudoNLO::initModule() {

    IoffeTimeDistributionModule::initModule();

    //alpha_s
    if (m_isAlphaSFixed) {
        m_alphaSUsed = m_alphaS;
    } else if (m_pRunningAlphaStrongModule != 0) {
        m_alphaSUsed = m_pRunningAlphaStrongModule->compute(m_MuR2);
    } else {
        m_alphaSUsed = 0.;
    }

    m_prefactor = m_alphaSUsed * COLOR_FACTOR_CF / (2. * Constant::PI);

    //matching logarithm: ln(z2 mu2 exp(2 gamma_E + 1) / 4), only defined for z2 > 0 and mu2 > 0
    if (m_z2 > 0. && m_MuF2 > 0.) {
        m_logZ2Mu2 = log(m_z2 * m_MuF2) + 2. * EULER_GAMMA + 1. - log(4.);
    } else {
        m_logZ2Mu2 = 0.;
    }

    //reset cache of kernel values (depends on kinematics)
    m_kernelCache.clear();
}

void IoffeTimeDistributionPseudoNLO::isModuleWellConfigured() {

    IoffeTimeDistributionModule::isModuleWellConfigured();

    if (!m_isAlphaSFixed && m_pRunningAlphaStrongModule == 0) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter()
                        << "No value of alpha_s available: set parameter "
                        << PARAM_NAME_ALPHA_S
                        << " or provide a RunningAlphaStrongModule");
    }

    if (fabs(m_xi) > 1.E-10) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter()
                        << "The one-loop matching implemented in this module is the forward one, xi must be 0 (got xi = "
                        << m_xi
                        << "). Use IoffeTimeDistributionLightCone for skewed kinematics.");
    }

    if (m_z2 <= 0.) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter() << "Pseudo-distributions need z2 > 0 (got z2 = "
                        << m_z2
                        << " GeV^-2). Use IoffeTimeDistributionLightCone for the light-cone distribution.");
    }

    if (m_MuF2 <= 0.) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                "The matching scale MuF2 must be > 0");
    }
}

IoffeTimeDistributionPseudoNLO::MatchingKernelType IoffeTimeDistributionPseudoNLO::resolveMatchingKernelType(
        GPDType::Type gpdType) const {

    if (m_matchingKernelType != AUTO) {
        return m_matchingKernelType;
    }

    switch (gpdType) {
    case GPDType::H:
    case GPDType::E:
        return UNPOLARIZED;
    case GPDType::Ht:
    case GPDType::Et:
        return HELICITY;
    case GPDType::HTrans:
    case GPDType::ETrans:
    case GPDType::HtTrans:
    case GPDType::EtTrans:
        return TRANSVERSITY;
    default:
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter()
                        << "No default matching kernel for GPD type "
                        << GPDType(gpdType).toString()
                        << ", set parameter " << PARAM_NAME_MATCHING_KERNEL);
    }
}

void IoffeTimeDistributionPseudoNLO::computeDistribution(GPDType::Type gpdType,
        PartonDistribution& realPart, PartonDistribution& imaginaryPart) {

    //select kernel for this GPD type
    m_activeKernelType = resolveMatchingKernelType(gpdType);

    debug(__func__,
            ElemUtils::Formatter() << "GPD " << GPDType(gpdType).toString()
                    << " matched with " << matchingKernelTypeToString(m_activeKernelType)
                    << " kernel, alpha_s = " << m_alphaSUsed << ", ln(z2 mu2 e^(2gE+1)/4) = "
                    << m_logZ2Mu2);

    //kernel values depend on the kernel type
    m_kernelCache.clear();

    //transform
    transform(gpdType, realPart, imaginaryPart);
}

double IoffeTimeDistributionPseudoNLO::kernelB(double u) const {

    switch (m_activeKernelType) {
    case TRANSVERSITY:
        return 2. * u / (1. - u);
    default:
        return (1. + u * u) / (1. - u);
    }
}

double IoffeTimeDistributionPseudoNLO::kernelD(double u) const {

    switch (m_activeKernelType) {
    case TRANSVERSITY:
        return 4. * log(1. - u) / (1. - u);
    default:
        return 4. * log(1. - u) / (1. - u) - 2. * (1. - u);
    }
}

double IoffeTimeDistributionPseudoNLO::matchingIntegrand(double u,
        std::vector<double>& params) {

    //end-point (measure zero, avoid 1/(1-u))
    if (1. - u < 1.E-14) {
        return 0.;
    }

    double w = params.at(0);
    int component = static_cast<int>(params.at(1));

    //plus prescription: (e^{iuw} - e^{iw})
    double oscillatory =
            (component == 0) ?
                    (cos(u * w) - cos(w)) : (sin(u * w) - sin(w));

    return (m_logZ2Mu2 * kernelB(u) + kernelD(u)) * oscillatory;
}

const std::pair<double, double>& IoffeTimeDistributionPseudoNLO::matchedKernel(
        double x) {

    std::map<double, std::pair<double, double> >::const_iterator it =
            m_kernelCache.find(x);

    if (it == m_kernelCache.end()) {

        double w = x * m_nu;

        double re = cos(w);
        double im = sin(w);

        //the plus-prescription convolution vanishes identically at w = 0
        if (w != 0. && m_prefactor != 0.) {

            std::vector<double> params(2, 0.);
            params[0] = w;

            params[1] = 0.;
            re -= m_prefactor
                    * m_pMatchingIntegrator->integrate(m_pMatchingIntegrand, 0.,
                            1., params);

            params[1] = 1.;
            im -= m_prefactor
                    * m_pMatchingIntegrator->integrate(m_pMatchingIntegrand, 0.,
                            1., params);
        }

        it = m_kernelCache.insert(
                std::pair<double, std::pair<double, double> >(x,
                        std::pair<double, double>(re, im))).first;
    }

    return it->second;
}

double IoffeTimeDistributionPseudoNLO::kernelRealPart(double x, bool isQuark) {

    //gluons: leading order only
    if (!isQuark) {
        return IoffeTimeDistributionModule::kernelRealPart(x, isQuark);
    }

    return matchedKernel(x).first;
}

double IoffeTimeDistributionPseudoNLO::kernelImaginaryPart(double x,
        bool isQuark) {

    //gluons: leading order only
    if (!isQuark) {
        return IoffeTimeDistributionModule::kernelImaginaryPart(x, isQuark);
    }

    return matchedKernel(x).second;
}

} /* namespace PARTONS */
