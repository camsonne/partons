#include "../../../../../include/partons/modules/convol_coeff_function/DVCS/DVCSCFFStandard.h"
#include "../../../../../include/partons/modules/convol_coeff_function/DVCS/DVCSCFFKernels.h"

#include <ElementaryUtils/logger/CustomException.h>
#include <ElementaryUtils/string_utils/Formatter.h>
#include <NumA/functor/one_dimension/Functor1D.h>
#include <NumA/integration/one_dimension/Integrator1D.h>
#include <NumA/integration/one_dimension/IntegratorType1D.h>
#include <NumA/utils/MathUtils.h>
#include <cmath>
#include <utility>

#include "../../../../../include/partons/beans/gpd/GPDKinematic.h"
#include "../../../../../include/partons/beans/gpd/GPDType.h"
#include "../../../../../include/partons/beans/parton_distribution/GluonDistribution.h"
#include "../../../../../include/partons/beans/parton_distribution/QuarkDistribution.h"
#include "../../../../../include/partons/beans/PerturbativeQCDOrderType.h"
#include "../../../../../include/partons/beans/QuarkFlavor.h"
#include "../../../../../include/partons/BaseObjectRegistry.h"
#include "../../../../../include/partons/FundamentalPhysicalConstants.h"
#include "../../../../../include/partons/modules/gpd/GPDModule.h"
#include "../../../../../include/partons/modules/running_alpha_strong/RunningAlphaStrongStandard.h"
#include "../../../../../include/partons/ModuleObjectFactory.h"
#include "../../../../../include/partons/Partons.h"

namespace PARTONS {

const unsigned int DVCSCFFStandard::classId =
        BaseObjectRegistry::getInstance()->registerBaseObject(
                new DVCSCFFStandard("DVCSCFFStandard"));

DVCSCFFStandard::DVCSCFFStandard(const std::string &className) :
        DVCSConvolCoeffFunctionModule(className), m_nf(0), m_pRunningAlphaStrongModule(
                0), m_Zeta(0.), m_logQ2OverMu2(0.), m_Q(0.), m_alphaSOver2Pi(
                0.), m_quarkDiagonal(0.), m_gluonDiagonal(0.), m_realPartSubtractQuark(
                0.), m_imaginaryPartSubtractQuark(0.), m_realPartSubtractGluon(
                0.), m_imaginaryPartSubtractGluon(0.), m_CF(4. / 3.) {
    m_listOfCFFComputeFunctionAvailable.insert(
            std::make_pair(GPDType::H,
                    &DVCSConvolCoeffFunctionModule::computeUnpolarized));
    m_listOfCFFComputeFunctionAvailable.insert(
            std::make_pair(GPDType::E,
                    &DVCSConvolCoeffFunctionModule::computeUnpolarized));
    m_listOfCFFComputeFunctionAvailable.insert(
            std::make_pair(GPDType::Ht,
                    &DVCSConvolCoeffFunctionModule::computePolarized));
    m_listOfCFFComputeFunctionAvailable.insert(
            std::make_pair(GPDType::Et,
                    &DVCSConvolCoeffFunctionModule::computePolarized));

    initFunctorsForIntegrations();
}

//TODO Call mother init function
void DVCSCFFStandard::resolveObjectDependencies() {

    DVCSConvolCoeffFunctionModule::resolveObjectDependencies();

    setIntegrator(NumA::IntegratorType1D::DEXP);

    m_pRunningAlphaStrongModule =
            Partons::getInstance()->getModuleObjectFactory()->newRunningAlphaStrongModule(
                    RunningAlphaStrongStandard::classId);
}

void DVCSCFFStandard::prepareSubModules(
        const std::map<std::string, BaseObjectData>& subModulesData) {

    DVCSConvolCoeffFunctionModule::prepareSubModules(subModulesData);

    std::map<std::string, BaseObjectData>::const_iterator it;

    it = subModulesData.find(
            RunningAlphaStrongModule::RUNNING_ALPHA_STRONG_MODULE_CLASS_NAME);

    if (it != subModulesData.end()) {

        if (m_pRunningAlphaStrongModule != 0) {
            setRunningAlphaStrongModule(0);
            m_pRunningAlphaStrongModule = 0;
        }

        if (!m_pRunningAlphaStrongModule) {
            m_pRunningAlphaStrongModule =
                    Partons::getInstance()->getModuleObjectFactory()->newRunningAlphaStrongModule(
                            (it->second).getModuleClassName());
            info(__func__,
                    ElemUtils::Formatter()
                            << "Configure with RunningAlphaStrongModule = "
                            << m_pRunningAlphaStrongModule->getClassName());
            m_pRunningAlphaStrongModule->configure(
                    (it->second).getParameters());
        }
    }
}

RunningAlphaStrongModule* DVCSCFFStandard::getRunningAlphaStrongModule() const {
    return m_pRunningAlphaStrongModule;
}

void DVCSCFFStandard::setRunningAlphaStrongModule(
        RunningAlphaStrongModule* pRunningAlphaStrongModule) {
    m_pModuleObjectFactory->updateModulePointerReference(
            m_pRunningAlphaStrongModule, pRunningAlphaStrongModule);
    m_pRunningAlphaStrongModule = pRunningAlphaStrongModule;
}

void DVCSCFFStandard::initFunctorsForIntegrations() {
    m_pConvolReKernelQuark1V = NumA::Integrator1D::newIntegrationFunctor(this,
            &DVCSCFFStandard::ConvolReKernelQuark1V);
    m_pConvolReKernelQuark2V = NumA::Integrator1D::newIntegrationFunctor(this,
            &DVCSCFFStandard::ConvolReKernelQuark2V);
    m_pConvolImKernelQuarkV = NumA::Integrator1D::newIntegrationFunctor(this,
            &DVCSCFFStandard::ConvolImKernelQuarkV);
    m_pConvolReKernelGluon1V = NumA::Integrator1D::newIntegrationFunctor(this,
            &DVCSCFFStandard::ConvolReKernelGluon1V);
    m_pConvolReKernelGluon2V = NumA::Integrator1D::newIntegrationFunctor(this,
            &DVCSCFFStandard::ConvolReKernelGluon2V);
    m_pConvolImKernelGluonV = NumA::Integrator1D::newIntegrationFunctor(this,
            &DVCSCFFStandard::ConvolImKernelGluonV);
    m_pConvolReKernelQuark1A = NumA::Integrator1D::newIntegrationFunctor(this,
            &DVCSCFFStandard::ConvolReKernelQuark1A);
    m_pConvolReKernelQuark2A = NumA::Integrator1D::newIntegrationFunctor(this,
            &DVCSCFFStandard::ConvolReKernelQuark2A);
    m_pConvolImKernelQuarkA = NumA::Integrator1D::newIntegrationFunctor(this,
            &DVCSCFFStandard::ConvolImKernelQuarkA);
    m_pConvolReKernelGluon1A = NumA::Integrator1D::newIntegrationFunctor(this,
            &DVCSCFFStandard::ConvolReKernelGluon1A);
    m_pConvolReKernelGluon2A = NumA::Integrator1D::newIntegrationFunctor(this,
            &DVCSCFFStandard::ConvolReKernelGluon2A);
    m_pConvolImKernelGluonA = NumA::Integrator1D::newIntegrationFunctor(this,
            &DVCSCFFStandard::ConvolImKernelGluonA);
}

DVCSCFFStandard::DVCSCFFStandard(const DVCSCFFStandard &other) :
        DVCSConvolCoeffFunctionModule(other) {

    m_nf = other.m_nf;

    if (other.m_pRunningAlphaStrongModule != 0) {
        m_pRunningAlphaStrongModule =
                m_pModuleObjectFactory->cloneModuleObject(other.m_pRunningAlphaStrongModule);
    } else {
        m_pRunningAlphaStrongModule = 0;
    }

    m_Zeta = other.m_Zeta;
    m_logQ2OverMu2 = other.m_logQ2OverMu2;
    m_Q = other.m_Q;
    m_alphaSOver2Pi = other.m_alphaSOver2Pi;
    m_quarkDiagonal = other.m_quarkDiagonal;
    m_gluonDiagonal = other.m_gluonDiagonal;

    m_realPartSubtractQuark = other.m_realPartSubtractQuark;
    m_imaginaryPartSubtractQuark = other.m_imaginaryPartSubtractQuark;
    m_realPartSubtractGluon = other.m_realPartSubtractGluon;
    m_imaginaryPartSubtractGluon = other.m_imaginaryPartSubtractGluon;

    m_CF = other.m_CF;

    initFunctorsForIntegrations();
}

DVCSCFFStandard* DVCSCFFStandard::clone() const {
    return new DVCSCFFStandard(*this);
}

//TODO comment gérer le cycle de vie des modules membres
DVCSCFFStandard::~DVCSCFFStandard() {

    // destroy alphaS
    if (m_pRunningAlphaStrongModule != 0) {
        setRunningAlphaStrongModule(0);
        m_pRunningAlphaStrongModule = 0;
    }

    // destroy functors
    if (m_pConvolReKernelQuark1V) {
        delete m_pConvolReKernelQuark1V;
        m_pConvolReKernelQuark1V = 0;
    }

    if (m_pConvolReKernelQuark2V) {
        delete m_pConvolReKernelQuark2V;
        m_pConvolReKernelQuark2V = 0;
    }

    if (m_pConvolImKernelQuarkV) {
        delete m_pConvolImKernelQuarkV;
        m_pConvolImKernelQuarkV = 0;
    }

    if (m_pConvolReKernelGluon1V) {
        delete m_pConvolReKernelGluon1V;
        m_pConvolReKernelGluon1V = 0;
    }

    if (m_pConvolReKernelGluon2V) {
        delete m_pConvolReKernelGluon2V;
        m_pConvolReKernelGluon2V = 0;
    }

    if (m_pConvolImKernelGluonV) {
        delete m_pConvolImKernelGluonV;
        m_pConvolImKernelGluonV = 0;
    }

    if (m_pConvolReKernelQuark1A) {
        delete m_pConvolReKernelQuark1A;
        m_pConvolReKernelQuark1A = 0;
    }

    if (m_pConvolReKernelQuark2A) {
        delete m_pConvolReKernelQuark2A;
        m_pConvolReKernelQuark2A = 0;
    }

    if (m_pConvolImKernelQuarkA) {
        delete m_pConvolImKernelQuarkA;
        m_pConvolImKernelQuarkA = 0;
    }

    if (m_pConvolReKernelGluon1A) {
        delete m_pConvolReKernelGluon1A;
        m_pConvolReKernelGluon1A = 0;
    }

    if (m_pConvolReKernelGluon2A) {
        delete m_pConvolReKernelGluon2A;
        m_pConvolReKernelGluon2A = 0;
    }

    if (m_pConvolImKernelGluonA) {
        delete m_pConvolImKernelGluonA;
        m_pConvolImKernelGluonA = 0;
    }
}

void DVCSCFFStandard::initModule() {
    // init parent module before
    DVCSConvolCoeffFunctionModule::initModule();

    m_nf = 3;
    m_Q = sqrt(m_Q2);
    m_Zeta = 2. * m_xi / (1 + m_xi);
    m_logQ2OverMu2 = log(m_Q2 / m_MuF2);

    m_alphaSOver2Pi = m_pRunningAlphaStrongModule->compute(m_MuR2)
            / (2. * Constant::PI);

    debug(__func__,
            ElemUtils::Formatter() << "m_Q2=" << m_Q2 << " m_Q= " << m_Q
                    << " m_MuF2=" << m_MuF2 << " m_Zeta= " << m_Zeta
                    << " m_logQ2OverMu2=" << m_logQ2OverMu2
                    << " m_nbOfActiveFlavour=" << m_nf << " m_alphaSOver2Pi="
                    << m_alphaSOver2Pi);
}

void DVCSCFFStandard::isModuleWellConfigured() {
    // check parent module before
    DVCSConvolCoeffFunctionModule::isModuleWellConfigured();

    if (m_pGPDModule == 0) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                "GPDModule* is NULL");
    }
    if (m_qcdOrderType == PerturbativeQCDOrderType::UNDEFINED) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                "QCDOrderType is UNDEFINED");
    }
    if (m_qcdOrderType != PerturbativeQCDOrderType::LO
            && m_qcdOrderType != PerturbativeQCDOrderType::NLO) {

        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter()
                        << "Erroneous input, perturbative QCD order can only be LO or NLO. Here Order = "
                        << PerturbativeQCDOrderType(m_qcdOrderType).toString());
    }
    if (getMathIntegrator() == 0) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                "MathIntegrationMode is UNDEFINED");
    }
    if (m_pRunningAlphaStrongModule == 0) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                "RunningAlphaStrongModule* is NULL");
    }
}

std::complex<double> DVCSCFFStandard::computeUnpolarized() {

    computeDiagonalGPD();
    computeSubtractionFunctionsV();
    return computeIntegralsV();
}

std::complex<double> DVCSCFFStandard::computePolarized() {

    computeDiagonalGPD();
    computeSubtractionFunctionsA();
    return computeIntegralsA();
}

void DVCSCFFStandard::computeDiagonalGPD() {
    PartonDistribution partonDistribution = m_pGPDModule->compute(
            GPDKinematic(m_xi, m_xi, m_t, m_MuF2, m_MuR2),
            m_currentGPDComputeType);

    //TODO compute CFF singlet; FAIT; vérifier le résultat du calcul
    m_quarkDiagonal = computeSquareChargeAveragedGPD(partonDistribution);
    m_gluonDiagonal = 2.
            * partonDistribution.getGluonDistribution().getGluonDistribution();

    //   debug( __func__,
    //      	                ElemUtils::Formatter()<<"    q diagonal = "<< m_quarkDiagonal <<"   g diagonal = "<< m_gluonDiagonal);

}

double DVCSCFFStandard::computeSquareChargeAveragedGPD(
        const PartonDistribution &partonDistribution) {
//TODO comment faire evoluer le calcul si de nouvelles saveurs de quark entrent en jeux
    double result = 0.;

    result +=
            (partonDistribution.getQuarkDistribution(QuarkFlavor::UP).getQuarkDistributionPlus())
                    * Constant::U2_ELEC_CHARGE;

    result +=
            (partonDistribution.getQuarkDistribution(QuarkFlavor::DOWN).getQuarkDistributionPlus())
                    * Constant::D2_ELEC_CHARGE;
    result +=
            (partonDistribution.getQuarkDistribution(QuarkFlavor::STRANGE).getQuarkDistributionPlus())
                    * Constant::S2_ELEC_CHARGE;

//    std::vector<> = gpdResultData.listQuarkTypeComputed()
//
//    for()
//    {
//        if( MuF || Q >< c.getSinglet() )
//    }

    return result;
}

void DVCSCFFStandard::computeSubtractionFunctionsV() {

    // Appendix B, shared with the LibTorch backend (DVCSCFFKernels.h).
    const DVCSCFFKernels::SubtractionConstants s =
            DVCSCFFKernels::subtractionConstants(m_Zeta, m_xi, m_logQ2OverMu2,
                    m_alphaSOver2Pi, NumA::MathUtils::DiLog(1. - 1. / m_Zeta),
                    m_qcdOrderType == PerturbativeQCDOrderType::NLO,
                    /*polarized=*/false);

    m_realPartSubtractQuark = s.realQuark;
    m_imaginaryPartSubtractQuark = s.imaginaryQuark;
    m_realPartSubtractGluon = s.realGluon;
    m_imaginaryPartSubtractGluon = s.imaginaryGluon;
}

void DVCSCFFStandard::computeSubtractionFunctionsA() {

    // Appendix B, shared with the LibTorch backend (DVCSCFFKernels.h).
    const DVCSCFFKernels::SubtractionConstants s =
            DVCSCFFKernels::subtractionConstants(m_Zeta, m_xi, m_logQ2OverMu2,
                    m_alphaSOver2Pi, NumA::MathUtils::DiLog(1. - 1. / m_Zeta),
                    m_qcdOrderType == PerturbativeQCDOrderType::NLO,
                    /*polarized=*/true);

    m_realPartSubtractQuark = s.realQuark;
    m_imaginaryPartSubtractQuark = s.imaginaryQuark;
    m_realPartSubtractGluon = s.realGluon;
    m_imaginaryPartSubtractGluon = s.imaginaryGluon;
}

std::complex<double> DVCSCFFStandard::computeIntegralsV() {
    double IntegralRealPartKernelQuark1 = 0.; // Integral between 0 and fZeta in real part of amplitude
    double IntegralRealPartKernelQuark2 = 0.; // Integral between fZeta and 1 in real part of amplitude
    double IntegralImaginaryPartKernelQuark = 0.; // Integral between 0 and fZeta in imaginary part of amplitude

    double IntegralRealPartKernelGluon1 = 0.; // Integral between 0 and fZeta in real part of amplitude
    double IntegralRealPartKernelGluon2 = 0.; // Integral between fZeta and 1 in real part of amplitude
    double IntegralImaginaryPartKernelGluon = 0.; // Integral between 0 and fZeta in imaginary part of amplitude

    double SumSqrCharges; // Sum of square of electric charges of active quark flavours

    // Sums up integrals and subtraction constants to get real and imaginary parts of CFF.

    if (m_qcdOrderType != PerturbativeQCDOrderType::LO
            && m_qcdOrderType != PerturbativeQCDOrderType::NLO) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter()
                        << "Erroneous input perturbative QCD order can only be LO or NLO. Here Order = "
                        << PerturbativeQCDOrderType(m_qcdOrderType).toString());
    }

    // Compute sum of active quark electric charges squared

    switch (m_nf) {

    case 3:
        SumSqrCharges = 2. / 3.;
        break;

    case 4:
        SumSqrCharges = 10. / 9.;
        break;

    case 5:
        SumSqrCharges = 11. / 9.;
        break;

    case 6:
        SumSqrCharges = 15. / 9.;
        break;

    default:
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter()
                        << "Erroneous input number of active quark flavours should be an integer between 3 and 6. Number of active quark flavours = "
                        << m_nf);
    }

    // Quark sector

    std::vector<double> emptyParameters;

    IntegralRealPartKernelQuark1 = integrate(m_pConvolReKernelQuark1V, 0.,
            +m_xi, emptyParameters);

    IntegralRealPartKernelQuark2 = integrate(m_pConvolReKernelQuark2V, +m_xi,
            +1, emptyParameters);

    IntegralImaginaryPartKernelQuark = integrate(m_pConvolImKernelQuarkV, +m_xi,
            +1, emptyParameters);

    // Gluon sector

    if (m_qcdOrderType == PerturbativeQCDOrderType::NLO) {

        IntegralRealPartKernelGluon1 = integrate(m_pConvolReKernelGluon1V, 0.,
                +m_xi, emptyParameters);

        IntegralRealPartKernelGluon2 = integrate(m_pConvolReKernelGluon2V,
                +m_xi, +1, emptyParameters);

        IntegralImaginaryPartKernelGluon = integrate(m_pConvolImKernelGluonV,
                +m_xi, +1, emptyParameters);
    }

    // Compute Subtraction constants (different at LO or NLO)

    computeSubtractionFunctionsV();

    // Compute real and imaginary part of CFF according to eq. (8) and eq. (9)

    double fRealPartCFF = IntegralRealPartKernelQuark1
            + IntegralRealPartKernelQuark2
            + m_quarkDiagonal * m_realPartSubtractQuark;
    fRealPartCFF += SumSqrCharges
            * (IntegralRealPartKernelGluon1 + IntegralRealPartKernelGluon2
                    + m_gluonDiagonal * m_realPartSubtractGluon);
    // Multiplication by SumSqrCharges corrects in mistake in eq. (9)

    double fImaginaryPartCFF = IntegralImaginaryPartKernelQuark
            + m_quarkDiagonal * m_imaginaryPartSubtractQuark;
    fImaginaryPartCFF += SumSqrCharges
            * (IntegralImaginaryPartKernelGluon
                    + m_gluonDiagonal * m_imaginaryPartSubtractGluon);
    // Multiplication by SumSqrCharges corrects in mistake in eq. (9)

    return std::complex<double>(fRealPartCFF, fImaginaryPartCFF);
}

std::complex<double> DVCSCFFStandard::computeIntegralsA() {
    double IntegralRealPartKernelQuark1 = 0.; // Integral between 0 and fZeta in real part of amplitude
    double IntegralRealPartKernelQuark2 = 0.; // Integral between fZeta and 1 in real part of amplitude
    double IntegralImaginaryPartKernelQuark = 0.; // Integral between 0 and fZeta in imaginary part of amplitude

    double IntegralRealPartKernelGluon1 = 0.; // Integral between 0 and fZeta in real part of amplitude
    double IntegralRealPartKernelGluon2 = 0.; // Integral between fZeta and 1 in real part of amplitude
    double IntegralImaginaryPartKernelGluon = 0.; // Integral between 0 and fZeta in imaginary part of amplitude

    double SumSqrCharges; // Sum of square of electric charges of active quark flavours

    // Sums up integrals and subtraction constants to get real and imaginary parts of CFF.

    if (m_qcdOrderType != PerturbativeQCDOrderType::LO
            && m_qcdOrderType != PerturbativeQCDOrderType::NLO) {

        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter()
                        << "Erroneous input perturbative QCD order can only be LO or NLO. Here Order = "
                        << PerturbativeQCDOrderType(m_qcdOrderType).toString());

    }

    // Compute sum of active quark electric charges squared

    switch (m_nf) {

    case 3:
        SumSqrCharges = 2. / 3.;
        break;

    case 4:
        SumSqrCharges = 10. / 9.;
        break;

    case 5:
        SumSqrCharges = 11. / 9.;
        break;

    case 6:
        SumSqrCharges = 15. / 9.;
        break;

    default:
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter()
                        << "Erroneous input number of active quark flavours should be an integer between 3 and 6. Number of active quark flavours = "
                        << m_nf);
    }

    // Quark sector

    std::vector<double> emptyParameters;

    IntegralRealPartKernelQuark1 = integrate(m_pConvolReKernelQuark1A, 0.,
            +m_xi, emptyParameters);

    IntegralRealPartKernelQuark2 = integrate(m_pConvolReKernelQuark2A, +m_xi,
            +1, emptyParameters);

    IntegralImaginaryPartKernelQuark = integrate(m_pConvolImKernelQuarkA, +m_xi,
            +1, emptyParameters);

    // Gluon sector

    if (m_qcdOrderType == PerturbativeQCDOrderType::NLO) {
        IntegralRealPartKernelGluon1 = integrate(m_pConvolReKernelGluon1A, 0.,
                +m_xi, emptyParameters);

        IntegralRealPartKernelGluon2 = integrate(m_pConvolReKernelGluon2A,
                +m_xi, +1, emptyParameters);

        IntegralImaginaryPartKernelGluon = integrate(m_pConvolImKernelGluonA,
                +m_xi, +1, emptyParameters);
    }

    // Compute Subtraction constants (different at LO or NLO)

    computeSubtractionFunctionsA();

    // Compute real and imaginary part of CFF according to eq. (8) and eq. (9)

    double realPartCFF = IntegralRealPartKernelQuark1
            + IntegralRealPartKernelQuark2
            + m_quarkDiagonal * m_realPartSubtractQuark;
    //	cout << fpQCDOrder << "      RealPartCFF Quark = " << fRealPartCFF << endl ;
    realPartCFF += SumSqrCharges
            * (IntegralRealPartKernelGluon1 + IntegralRealPartKernelGluon2
                    + m_gluonDiagonal * m_realPartSubtractGluon);
    //	cout << fpQCDOrder << "      RealPartCFF Gluon = " << SumSqrCharges * ( IntegralRealPartKernelGluon1 + IntegralRealPartKernelGluon2 + fGluonDiagonal * fRealPartSubtractGluon ) << endl ;
    // Multiplication by SumSqrCharges corrects in mistake in eq. (9)

    double imaginaryPartCFF = IntegralImaginaryPartKernelQuark
            + m_quarkDiagonal * m_imaginaryPartSubtractQuark;
    //	cout << fpQCDOrder << "      ImaginaryPartCFF Quark = " << fImaginaryPartCFF << endl ;
    imaginaryPartCFF += SumSqrCharges
            * (IntegralImaginaryPartKernelGluon
                    + m_gluonDiagonal * m_imaginaryPartSubtractGluon);
    //	cout << fpQCDOrder << "      ImaginaryPartCFF Gluon = " << SumSqrCharges * ( IntegralImaginaryPartKernelGluon + fGluonDiagonal * fImaginaryPartSubtractGluon ) << endl ;
    // Multiplication by SumSqrCharges corrects in mistake in eq. (9)
    debug(__func__,
            ElemUtils::Formatter() << "    integral RE = " << realPartCFF
                    << "   Integral IM = " << imaginaryPartCFF);

    return std::complex<double>(realPartCFF, imaginaryPartCFF);
}

std::complex<double> DVCSCFFStandard::KernelQuarkNLOV(double x) {
    // Appendix A eq. (A2), shared with the LibTorch backend (DVCSCFFKernels.h).
    return DVCSCFFKernels::toStdComplex(
            DVCSCFFKernels::quarkNLOV(x, m_xi, m_logQ2OverMu2));
}

/*!
 * \fn KernelQuark( double x )
 *
 * T^{q, V/A} in appendix A, eq. (A1).
 *
 */
std::complex<double> DVCSCFFStandard::KernelQuarkV(double x) {
    //std::complex<double> z = std::complex<double>(x / m_xi, 0.);
    //std::complex<double> quark();

    // Appendix A eq. (A1), shared with the LibTorch backend (DVCSCFFKernels.h).
    std::complex<double> quark(DVCSCFFKernels::quarkLO(x, m_xi), 0.);

    if (m_qcdOrderType == PerturbativeQCDOrderType::NLO) {
        quark += m_alphaSOver2Pi * KernelQuarkNLOV(x);
    }

    return quark;
}

std::complex<double> DVCSCFFStandard::KernelQuarkA(double x) {
    //std::complex<double> z = std::complex<double>(x / m_xi, 0.);
    //std::complex<double> quark();

    // Appendix A eq. (A1), shared with the LibTorch backend (DVCSCFFKernels.h).
    std::complex<double> quark(DVCSCFFKernels::quarkLO(x, m_xi), 0.);

    if (m_qcdOrderType == PerturbativeQCDOrderType::NLO) {
        quark += m_alphaSOver2Pi * KernelQuarkNLOA(x);
    }

    return quark;
}

/*!
 * \fn KernelGluon( double x )
 *
 * T^{g, V/A} in appendix A, eq. (A1).
 *
 */
std::complex<double> DVCSCFFStandard::KernelGluonV(double x) {

    std::complex<double> gluon(0., 0.);

    if (m_qcdOrderType == PerturbativeQCDOrderType::NLO) {
        gluon += m_alphaSOver2Pi * KernelGluonNLOV(x);
    }

    return gluon;
}

std::complex<double> DVCSCFFStandard::KernelGluonA(double x) {

    std::complex<double> gluon(0., 0.);

    if (m_qcdOrderType == PerturbativeQCDOrderType::NLO) {
        gluon += m_alphaSOver2Pi * KernelGluonNLOA(x);
    }

    return gluon;
}

/*
 * \fn double ConvolReKernelQuark1( double* x, double* Parameters );
 *
 * eq. (8), real part of amplitude, terms integrated for X between 0 and fZeta.
 * Equivalently x integration domain ranges between -fXi and +fXi.
 * Warning : grid is defined for x > 0.
 *
 * Expressions are modified in order to integrate between 0 and fXi, hence explicitely avoiding GPD behaviour at x = 0.
 *
 */
double DVCSCFFStandard::ConvolReKernelQuark1V(double x,
        std::vector<double> params) {
    PartonDistribution partonDistribution = m_pGPDModule->compute(
            GPDKinematic(x, m_xi, m_t, m_MuF2, m_MuR2),
            m_currentGPDComputeType);

    // GPD evaluated at x = x[ 0 ]
    double EvalGPD = computeSquareChargeAveragedGPD(partonDistribution);

    // Integrated function
    double Convol = (EvalGPD - m_quarkDiagonal) * KernelQuarkV(x).real();
    Convol += (-EvalGPD - m_quarkDiagonal) * KernelQuarkV(-x).real();

    Convol /= m_xi; // In eq. (8), ( 2 - fZeta ) / fZeta = 1 / fXi

    debug(__func__,
            ElemUtils::Formatter() << "x = " << x << " | convol = " << Convol);

    return Convol;
}

/*
 * \fn double ConvolReKernelQuark2( double* x, double* Parameters );
 *
 * eq. (8), real part of amplitude, terms integrated between fZeta and 1.
 * Equivalently x integration domain ranges between fXi and 1.
 *
 */
double DVCSCFFStandard::ConvolReKernelQuark2V(double x,
        std::vector<double> params) {

    PartonDistribution partonDistribution = m_pGPDModule->compute(
            GPDKinematic(x, m_xi, m_t, m_MuF2, m_MuR2),
            m_currentGPDComputeType);

    // GPD evaluated at x = x[ 0 ]
    double EvalGPD = computeSquareChargeAveragedGPD(partonDistribution);

    double Convol = EvalGPD - m_quarkDiagonal;
    Convol *= KernelQuarkV(x).real();

    Convol += -KernelQuarkV(-x).real() * EvalGPD;

    Convol /= m_xi; // In eq. (8), ( 2 - fZeta ) / fZeta = 1 / fXi

    debug(__func__,
            ElemUtils::Formatter() << "x = " << x << " | convol = " << Convol);

    return Convol;
}

/*
 * \fn double ConvolImKernelQuark( double* x, double* Parameters );
 *
 * eq. (8), imaginary part of amplitude, terms integrated between fZeta and 1.
 * Equivalently x integration domain ranges between fXi and 1.
 *
 */
double DVCSCFFStandard::ConvolImKernelQuarkV(double x,
        std::vector<double> params) {
    PartonDistribution partonDistribution = m_pGPDModule->compute(
            GPDKinematic(x, m_xi, m_t, m_MuF2, m_MuR2),
            m_currentGPDComputeType);

    // GPD evaluated at x = x[ 0 ]
    double EvalGPD = computeSquareChargeAveragedGPD(partonDistribution);

    double Convol = EvalGPD - m_quarkDiagonal;
    Convol *= KernelQuarkV(x).imag();
    Convol /= m_xi; // In eq. (8), ( 2 - fZeta ) / fZeta = 1 / fXi

    debug(__func__,
            ElemUtils::Formatter() << "x = " << x << " | convol = " << Convol);

    return Convol;
}

/*
 * \fn double ConvolReKernelGluon1( double* x, double* Parameters );
 *
 * eq. (9), real part of amplitude, terms integrated between 0 and fZeta.
 * Equivalently x integration domain ranges between -fXi and +fXi.
 * Warning : grid is defined for x > 0.
 *
 * Expressions are modified in order to integrate between 0 and fXi, hence explicitely avoiding GPD behaviour at x = 0.
 *
 */
double DVCSCFFStandard::ConvolReKernelGluon1V(double x,
        std::vector<double> params) {
    PartonDistribution partonDistribution = m_pGPDModule->compute(
            GPDKinematic(x, m_xi, m_t, m_MuF2, m_MuR2),
            m_currentGPDComputeType);

    // GPD evaluated at x = x[ 0 ]
    double EvalGPD = 2
            * partonDistribution.getGluonDistribution().getGluonDistribution();

    double Convol = (EvalGPD - m_gluonDiagonal) * KernelGluonV(x).real();
    Convol += (+EvalGPD - m_gluonDiagonal) * KernelGluonV(-x).real();

    Convol /= (m_xi * m_xi * m_nf);

    debug(__func__,
            ElemUtils::Formatter() << "x = " << x << " | convol = " << Convol);

    return Convol;
}

/*
 * \fn double ConvolReKernelGluon2( double* x, double* Parameters );
 *
 * eq. (8), real part of amplitude, terms integrated between fZeta and 1.
 * Equivalently x integration domain ranges between fXi and 1.
 *
 */
double DVCSCFFStandard::ConvolReKernelGluon2V(double x,
        std::vector<double> params) {
    PartonDistribution partonDistribution = m_pGPDModule->compute(
            GPDKinematic(x, m_xi, m_t, m_MuF2, m_MuR2),
            m_currentGPDComputeType);

    double EvalGPD = 2
            * partonDistribution.getGluonDistribution().getGluonDistribution();

    double Convol = EvalGPD - m_gluonDiagonal;
    Convol *= KernelGluonV(x).real();

    Convol += +KernelGluonV(-x).real() * EvalGPD;

    Convol /= (m_xi * m_xi * m_nf); // In eq. (8), ( ( 2 - fZeta ) / fZeta )^2 = 1 / fXi^2

    debug(__func__,
            ElemUtils::Formatter() << "x = " << x << " | convol = " << Convol);

    return Convol;
}

/*
 * \fn double ConvolImKernelGluon( double* x, double* Parameters );
 *
 * eq. (9), imaginary part of amplitude, terms integrated between fZeta and 1.
 * Equivalently x integration domain ranges between fXi and 1.
 *
 */
double DVCSCFFStandard::ConvolImKernelGluonV(double x,
        std::vector<double> params) {
    PartonDistribution partonDistribution = m_pGPDModule->compute(
            GPDKinematic(x, m_xi, m_t, m_MuF2, m_MuR2),
            m_currentGPDComputeType);

    double EvalGPD = 2
            * partonDistribution.getGluonDistribution().getGluonDistribution();

    double Convol = EvalGPD - m_gluonDiagonal;
    Convol *= KernelGluonV(x).imag();
    Convol /= (m_xi * m_xi * m_nf); // In eq. (8), ( ( 2 - fZeta ) / fZeta )^2 = 1 / fXi^2

    debug(__func__,
            ElemUtils::Formatter() << "x = " << x << " | convol = " << Convol);

    return Convol;
}

std::complex<double> DVCSCFFStandard::KernelGluonNLOV(double x) {
    // Appendix A eq. (A2), shared with the LibTorch backend (DVCSCFFKernels.h).
    return DVCSCFFKernels::toStdComplex(
            DVCSCFFKernels::gluonNLOV(x, m_xi, m_logQ2OverMu2,
                    static_cast<double>(m_nf)));
}

std::complex<double> DVCSCFFStandard::KernelGluonNLOA(double x) {
    // Appendix A eq. (A2), shared with the LibTorch backend (DVCSCFFKernels.h).
    return DVCSCFFKernels::toStdComplex(
            DVCSCFFKernels::gluonNLOA(x, m_xi, m_logQ2OverMu2,
                    static_cast<double>(m_nf)));
}

std::complex<double> DVCSCFFStandard::KernelQuarkNLOA(double x) {
    // Appendix A eq. (A2), shared with the LibTorch backend (DVCSCFFKernels.h).
    return DVCSCFFKernels::toStdComplex(
            DVCSCFFKernels::quarkNLOA(x, m_xi, m_logQ2OverMu2));
}

double DVCSCFFStandard::ConvolReKernelQuark1A(double x,
        std::vector<double> params) {
    PartonDistribution partonDistribution = m_pGPDModule->compute(
            GPDKinematic(x, m_xi, m_t, m_MuF2, m_MuR2),
            m_currentGPDComputeType);

    // GPD evaluated at x = x[ 0 ]
    double EvalGPD = computeSquareChargeAveragedGPD(partonDistribution);

    double Convol = (EvalGPD - m_quarkDiagonal) * KernelQuarkA(x).real();
    Convol += (+EvalGPD - m_quarkDiagonal) * KernelQuarkA(-x).real();

    Convol /= m_xi; // In eq. (8), ( 2 - fZeta ) / fZeta = 1 / fXi

    debug(__func__,
            ElemUtils::Formatter() << "x = " << x << " | convol = " << Convol);

    return Convol;
}

double DVCSCFFStandard::ConvolReKernelQuark2A(double x,
        std::vector<double> params) {
    PartonDistribution partonDistribution = m_pGPDModule->compute(
            GPDKinematic(x, m_xi, m_t, m_MuF2, m_MuR2),
            m_currentGPDComputeType);

    // GPD evaluated at x = x[ 0 ]
    double EvalGPD = computeSquareChargeAveragedGPD(partonDistribution);

    double Convol = EvalGPD - m_quarkDiagonal;
    Convol *= KernelQuarkA(x).real();

    Convol += +KernelQuarkA(-x).real() * EvalGPD;

    Convol /= m_xi; // In eq. (8), ( 2 - fZeta ) / fZeta = 1 / fXi

    debug(__func__,
            ElemUtils::Formatter() << "x = " << x << " | convol = " << Convol);

    return Convol;
}

double DVCSCFFStandard::ConvolImKernelQuarkA(double x,
        std::vector<double> params) {
    PartonDistribution partonDistribution = m_pGPDModule->compute(
            GPDKinematic(x, m_xi, m_t, m_MuF2, m_MuR2),
            m_currentGPDComputeType);

    // GPD evaluated at x = x[ 0 ]
    double EvalGPD = computeSquareChargeAveragedGPD(partonDistribution);

    double Convol = EvalGPD - m_quarkDiagonal;
    Convol *= KernelQuarkA(x).imag();
    Convol /= m_xi; // In eq. (8), ( 2 - fZeta ) / fZeta = 1 / fXi

    debug(__func__,
            ElemUtils::Formatter() << "x = " << x << " | convol = " << Convol);

    return Convol;
}

double DVCSCFFStandard::ConvolReKernelGluon1A(double x,
        std::vector<double> params) {

    debug(__func__, "Entered");

    PartonDistribution partonDistribution = m_pGPDModule->compute(
            GPDKinematic(x, m_xi, m_t, m_MuF2, m_MuR2),
            m_currentGPDComputeType);

    double EvalGPD = 2
            * partonDistribution.getGluonDistribution().getGluonDistribution();

    double Convol = (EvalGPD - m_gluonDiagonal) * KernelGluonA(x).real();
    Convol += (-EvalGPD - m_gluonDiagonal) * KernelGluonA(-x).real();

    Convol /= (m_xi * m_xi * m_nf);

    debug(__func__,
            ElemUtils::Formatter() << "x = " << x << " | convol = " << Convol);

    return Convol;
}

double DVCSCFFStandard::ConvolReKernelGluon2A(double x,
        std::vector<double> params) {
    PartonDistribution partonDistribution = m_pGPDModule->compute(
            GPDKinematic(x, m_xi, m_t, m_MuF2, m_MuR2),
            m_currentGPDComputeType);

    double EvalGPD = 2
            * partonDistribution.getGluonDistribution().getGluonDistribution();

    double Convol = EvalGPD - m_gluonDiagonal;
    Convol *= KernelGluonA(x).real();

    Convol += -KernelGluonA(-x).real() * EvalGPD;

    Convol /= (m_xi * m_xi * m_nf); // In eq. (8), ( ( 2 - fZeta ) / fZeta )^2 = 1 / fXi^2

    debug(__func__,
            ElemUtils::Formatter() << "x = " << x << " | convol = " << Convol);

    return Convol;
}

double DVCSCFFStandard::ConvolImKernelGluonA(double x,
        std::vector<double> params) {
    PartonDistribution partonDistribution = m_pGPDModule->compute(
            GPDKinematic(x, m_xi, m_t, m_MuF2, m_MuR2),
            m_currentGPDComputeType);

    double EvalGPD = 2
            * partonDistribution.getGluonDistribution().getGluonDistribution();

    double Convol = EvalGPD - m_gluonDiagonal;
    Convol *= KernelGluonA(x).imag();
    Convol /= (m_xi * m_xi * m_nf); // In eq. (8), ( ( 2 - fZeta ) / fZeta )^2 = 1 / fXi^2

    debug(__func__,
            ElemUtils::Formatter() << "x = " << x << " | convol = " << Convol);

    return Convol;
}

} /* namespace PARTONS */
