#include "../../../../../include/partons/modules/convol_coeff_function/DVCS/DVCSCFFTorch.h"

#ifdef PARTONS_WITH_TORCH

#include <ElementaryUtils/logger/CustomException.h>
#include <ElementaryUtils/parameters/Parameters.h>
#include <ElementaryUtils/string_utils/Formatter.h>
#include <NumA/utils/MathUtils.h>
#include <cmath>
#include <utility>

#include "../../../../../include/partons/BaseObjectRegistry.h"
#include "../../../../../include/partons/beans/gpd/GPDKinematic.h"
#include "../../../../../include/partons/beans/parton_distribution/GluonDistribution.h"
#include "../../../../../include/partons/beans/parton_distribution/PartonDistribution.h"
#include "../../../../../include/partons/beans/parton_distribution/QuarkDistribution.h"
#include "../../../../../include/partons/beans/QuarkFlavor.h"
#include "../../../../../include/partons/FundamentalPhysicalConstants.h"
#include "../../../../../include/partons/ModuleObjectFactory.h"
#include "../../../../../include/partons/modules/convol_coeff_function/DVCS/DVCSCFFKernels.h"
#include "../../../../../include/partons/modules/convol_coeff_function/DVCS/DVCSCFFKernelsTorch.h"
#include "../../../../../include/partons/modules/gpd/GPDModule.h"
#include "../../../../../include/partons/modules/running_alpha_strong/RunningAlphaStrongModule.h"
#include "../../../../../include/partons/modules/running_alpha_strong/RunningAlphaStrongStandard.h"
#include "../../../../../include/partons/Partons.h"

namespace PARTONS {

const unsigned int DVCSCFFTorch::classId =
        BaseObjectRegistry::getInstance()->registerBaseObject(
                new DVCSCFFTorch("DVCSCFFTorch"));

const std::string DVCSCFFTorch::PARAMETER_NAME_QUADRATURE_LEVEL =
        "torch_quadrature_level";

DVCSCFFTorch::DVCSCFFTorch(const std::string &className) :
        DVCSConvolCoeffFunctionModule(className), m_level(5), m_nf(3), m_logQ2OverMu2(
                0.), m_alphaSOver2Pi(0.), m_pRunningAlphaStrongModule(0) {

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
}

DVCSCFFTorch::DVCSCFFTorch(const DVCSCFFTorch &other) :
        DVCSConvolCoeffFunctionModule(other), m_level(other.m_level), m_nf(
                other.m_nf), m_logQ2OverMu2(other.m_logQ2OverMu2), m_alphaSOver2Pi(
                other.m_alphaSOver2Pi), m_pRunningAlphaStrongModule(0) {

    if (other.m_pRunningAlphaStrongModule != 0) {
        m_pRunningAlphaStrongModule =
                m_pModuleObjectFactory->cloneModuleObject(
                        other.m_pRunningAlphaStrongModule);
    }
}

DVCSCFFTorch* DVCSCFFTorch::clone() const {
    return new DVCSCFFTorch(*this);
}

DVCSCFFTorch::~DVCSCFFTorch() {
    if (m_pRunningAlphaStrongModule != 0) {
        setRunningAlphaStrongModule(0);
        m_pRunningAlphaStrongModule = 0;
    }
}

void DVCSCFFTorch::resolveObjectDependencies() {

    DVCSConvolCoeffFunctionModule::resolveObjectDependencies();

    m_pRunningAlphaStrongModule =
            Partons::getInstance()->getModuleObjectFactory()->newRunningAlphaStrongModule(
                    RunningAlphaStrongStandard::classId);
}

void DVCSCFFTorch::configure(const ElemUtils::Parameters &parameters) {

    DVCSConvolCoeffFunctionModule::configure(parameters);

    if (parameters.isAvailable(PARAMETER_NAME_QUADRATURE_LEVEL)) {
        setQuadratureLevel(parameters.getLastAvailable().toUInt());
        info(__func__,
                ElemUtils::Formatter() << PARAMETER_NAME_QUADRATURE_LEVEL
                        << " configured with value = " << m_level);
    }
}

void DVCSCFFTorch::prepareSubModules(
        const std::map<std::string, BaseObjectData>& subModulesData) {

    DVCSConvolCoeffFunctionModule::prepareSubModules(subModulesData);

    std::map<std::string, BaseObjectData>::const_iterator it =
            subModulesData.find(
                    RunningAlphaStrongModule::RUNNING_ALPHA_STRONG_MODULE_CLASS_NAME);

    if (it != subModulesData.end()) {

        if (m_pRunningAlphaStrongModule != 0) {
            setRunningAlphaStrongModule(0);
            m_pRunningAlphaStrongModule = 0;
        }

        m_pRunningAlphaStrongModule =
                Partons::getInstance()->getModuleObjectFactory()->newRunningAlphaStrongModule(
                        (it->second).getModuleClassName());
        info(__func__,
                ElemUtils::Formatter()
                        << "Configure with RunningAlphaStrongModule = "
                        << m_pRunningAlphaStrongModule->getClassName());
        m_pRunningAlphaStrongModule->configure((it->second).getParameters());
    }
}

RunningAlphaStrongModule* DVCSCFFTorch::getRunningAlphaStrongModule() const {
    return m_pRunningAlphaStrongModule;
}

void DVCSCFFTorch::setRunningAlphaStrongModule(
        RunningAlphaStrongModule* pRunningAlphaStrongModule) {
    m_pModuleObjectFactory->updateModulePointerReference(
            m_pRunningAlphaStrongModule, pRunningAlphaStrongModule);
    m_pRunningAlphaStrongModule = pRunningAlphaStrongModule;
}

unsigned int DVCSCFFTorch::getQuadratureLevel() const {
    return m_level;
}

void DVCSCFFTorch::setQuadratureLevel(unsigned int level) {
    if (level < 1 || level > 10) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter() << "Quadrature level must be in [1, 10], got "
                        << level);
    }
    m_level = level;
}

void DVCSCFFTorch::initModule() {

    DVCSConvolCoeffFunctionModule::initModule();

    m_nf = 3;
    m_logQ2OverMu2 = log(m_Q2 / m_MuF2);
    m_alphaSOver2Pi = m_pRunningAlphaStrongModule->compute(m_MuR2)
            / (2. * Constant::PI);
}

void DVCSCFFTorch::isModuleWellConfigured() {

    DVCSConvolCoeffFunctionModule::isModuleWellConfigured();

    if (m_pGPDModule == 0) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                "GPDModule* is NULL");
    }
    if (m_qcdOrderType != PerturbativeQCDOrderType::LO
            && m_qcdOrderType != PerturbativeQCDOrderType::NLO) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                ElemUtils::Formatter()
                        << "Perturbative QCD order can only be LO or NLO. Here Order = "
                        << PerturbativeQCDOrderType(m_qcdOrderType).toString());
    }
    if (m_pRunningAlphaStrongModule == 0) {
        throw ElemUtils::CustomException(getClassName(), __func__,
                "RunningAlphaStrongModule* is NULL");
    }
}

std::complex<double> DVCSCFFTorch::computeUnpolarized() {
    return computeCurrent(/*polarized=*/false);
}

std::complex<double> DVCSCFFTorch::computePolarized() {
    return computeCurrent(/*polarized=*/true);
}

std::complex<double> DVCSCFFTorch::computeCurrent(bool polarized) {

    torch::NoGradGuard noGrad;
    const torch::TensorOptions opts = torch::TensorOptions().dtype(
            torch::kDouble);

    const torch::Tensor xi = torch::tensor( { m_xi }, opts);
    const Grid grid = makeGrid(xi, m_level);

    const Samples samples = sampleGPD(m_pGPDModule, grid,
            std::vector<double>(1, m_xi), std::vector<double>(1, m_t),
            std::vector<double>(1, m_MuF2), std::vector<double>(1, m_MuR2),
            m_currentGPDComputeType);

    Scalars scalars;
    scalars.xi = xi;
    scalars.alphaSOver2Pi = torch::tensor( { m_alphaSOver2Pi }, opts);
    scalars.logQ2OverMu2 = torch::tensor( { m_logQ2OverMu2 }, opts);
    scalars.nf = m_nf;

    const std::pair<torch::Tensor, torch::Tensor> cff = evaluate(grid, samples,
            scalars, m_qcdOrderType, polarized);

    return std::complex<double>(cff.first.item<double>(),
            cff.second.item<double>());
}

// ##### BATCHED TENSOR API #####

DVCSCFFTorch::Grid DVCSCFFTorch::makeGrid(const torch::Tensor& xiIn,
        unsigned int level) {

    // tanh-sinh (double exponential) rule on [-1, 1]: u = tanh( pi/2 sinh t ),
    // t = k h, |t| <= 3, weights h pi/2 cosh t / cosh^2( pi/2 sinh t ). This is
    // the rule of NumA::DExpIntegrator1D frozen at one refinement level.
    const torch::Tensor xi = xiIn.to(torch::kDouble).reshape( { -1, 1 }); // {P, 1}
    const double h = std::ldexp(1.0, -static_cast<int>(level));
    const int K = static_cast<int>(std::floor(3.0 / h + 1e-9));

    const torch::Tensor k = torch::arange(-K, K + 1, xi.options());       // {N}
    const torch::Tensor t = k * h;
    const torch::Tensor s = torch::sinh(t) * (M_PI / 2.);
    const torch::Tensor u = torch::tanh(s).unsqueeze(0);                  // {1, N}
    const torch::Tensor omega = (torch::cosh(t) * (h * M_PI / 2.)
            / torch::cosh(s).pow(2)).unsqueeze(0);                        // {1, N}

    Grid g;
    // [0, xi]: x = c u + d with c = d = xi / 2
    const torch::Tensor c1 = xi * 0.5;
    g.x1 = c1 * u + c1;
    g.w1 = c1 * omega;
    // [xi, 1]: c = (1 - xi) / 2, d = (1 + xi) / 2
    const torch::Tensor c2 = (1. - xi) * 0.5;
    const torch::Tensor d2 = (1. + xi) * 0.5;
    g.x2 = c2 * u + d2;
    g.w2 = c2 * omega;
    return g;
}

double DVCSCFFTorch::squareChargeAveragedGPD(
        const PartonDistribution& partonDistribution) {

    double result = 0.;
    result += partonDistribution.getQuarkDistribution(QuarkFlavor::UP).getQuarkDistributionPlus()
            * Constant::U2_ELEC_CHARGE;
    result += partonDistribution.getQuarkDistribution(QuarkFlavor::DOWN).getQuarkDistributionPlus()
            * Constant::D2_ELEC_CHARGE;
    result += partonDistribution.getQuarkDistribution(QuarkFlavor::STRANGE).getQuarkDistributionPlus()
            * Constant::S2_ELEC_CHARGE;
    return result;
}

DVCSCFFTorch::Samples DVCSCFFTorch::sampleGPD(GPDModule* pGPDModule,
        const Grid& grid, const std::vector<double>& xi,
        const std::vector<double>& t, const std::vector<double>& MuF2,
        const std::vector<double>& MuR2, GPDType::Type gpdType) {

    if (pGPDModule == 0) {
        throw ElemUtils::CustomException("DVCSCFFTorch", __func__,
                "GPDModule* is NULL");
    }

    const int64_t P = grid.x1.size(0);
    const int64_t N = grid.x1.size(1);

    if (static_cast<int64_t>(xi.size()) != P || t.size() != xi.size()
            || MuF2.size() != xi.size() || MuR2.size() != xi.size()) {
        throw ElemUtils::CustomException("DVCSCFFTorch", __func__,
                "Kinematic vectors do not match the grid");
    }

    const torch::Tensor x1 = grid.x1.to(torch::kCPU).contiguous();
    const torch::Tensor x2 = grid.x2.to(torch::kCPU).contiguous();

    std::vector<double> q1(P * N), q2(P * N), qd(P), g1(P * N), g2(P * N),
            gd(P);

    for (int64_t p = 0; p < P; ++p) {
        const double* px1 = x1[p].data_ptr<double>();
        const double* px2 = x2[p].data_ptr<double>();
        for (int64_t n = 0; n < N; ++n) {
            PartonDistribution pd1 = pGPDModule->compute(
                    GPDKinematic(px1[n], xi[p], t[p], MuF2[p], MuR2[p]),
                    gpdType);
            q1[p * N + n] = squareChargeAveragedGPD(pd1);
            g1[p * N + n] = 2.
                    * pd1.getGluonDistribution().getGluonDistribution();

            PartonDistribution pd2 = pGPDModule->compute(
                    GPDKinematic(px2[n], xi[p], t[p], MuF2[p], MuR2[p]),
                    gpdType);
            q2[p * N + n] = squareChargeAveragedGPD(pd2);
            g2[p * N + n] = 2.
                    * pd2.getGluonDistribution().getGluonDistribution();
        }
        PartonDistribution pdd = pGPDModule->compute(
                GPDKinematic(xi[p], xi[p], t[p], MuF2[p], MuR2[p]), gpdType);
        qd[p] = squareChargeAveragedGPD(pdd);
        gd[p] = 2. * pdd.getGluonDistribution().getGluonDistribution();
    }

    const torch::TensorOptions opts = torch::TensorOptions().dtype(
            torch::kDouble);
    const torch::Device device = grid.x1.device();

    Samples s;
    s.quark1 = torch::tensor(q1, opts).reshape( { P, N }).to(device);
    s.quark2 = torch::tensor(q2, opts).reshape( { P, N }).to(device);
    s.quarkDiagonal = torch::tensor(qd, opts).to(device);
    s.gluon1 = torch::tensor(g1, opts).reshape( { P, N }).to(device);
    s.gluon2 = torch::tensor(g2, opts).reshape( { P, N }).to(device);
    s.gluonDiagonal = torch::tensor(gd, opts).to(device);
    return s;
}

namespace {

// Real and imaginary parts of a kernel evaluated on a node tensor x ({P, N}),
// for +x and -x, at the requested order. xi and logQ2OverMu2 are {P, 1}.
struct KernelOnGrid {
    torch::Tensor rePlus;   // Re K(+x)
    torch::Tensor reMinus;  // Re K(-x)
    torch::Tensor imPlus;   // Im K(+x)
};

KernelOnGrid quarkKernel(const torch::Tensor& x, const torch::Tensor& xi,
        const torch::Tensor& logQ2OverMu2, const torch::Tensor& alphaSOver2Pi,
        bool nlo, bool polarized) {
    using namespace DVCSCFFKernels;

    KernelOnGrid k;
    k.rePlus = quarkLO(x, xi);
    k.reMinus = quarkLO(-x, xi);
    k.imPlus = torch::zeros_like(x);

    if (nlo) {
        const Cplx<torch::Tensor> plus =
                polarized ? quarkNLOA(x, xi, logQ2OverMu2) : quarkNLOV(x, xi,
                                    logQ2OverMu2);
        const Cplx<torch::Tensor> minus =
                polarized ? quarkNLOA(-x, xi, logQ2OverMu2) : quarkNLOV(-x, xi,
                                    logQ2OverMu2);
        k.rePlus = k.rePlus + alphaSOver2Pi * plus.re;
        k.imPlus = k.imPlus + alphaSOver2Pi * plus.im;
        k.reMinus = k.reMinus + alphaSOver2Pi * minus.re;
    }
    return k;
}

KernelOnGrid gluonKernel(const torch::Tensor& x, const torch::Tensor& xi,
        const torch::Tensor& logQ2OverMu2, const torch::Tensor& alphaSOver2Pi,
        double nf, bool polarized) {
    using namespace DVCSCFFKernels;

    // The gluon kernel vanishes at LO; only called for NLO.
    const Cplx<torch::Tensor> plus =
            polarized ? gluonNLOA(x, xi, logQ2OverMu2, nf) : gluonNLOV(x, xi,
                                logQ2OverMu2, nf);
    const Cplx<torch::Tensor> minus =
            polarized ? gluonNLOA(-x, xi, logQ2OverMu2, nf) : gluonNLOV(-x, xi,
                                logQ2OverMu2, nf);
    KernelOnGrid k;
    k.rePlus = alphaSOver2Pi * plus.re;
    k.imPlus = alphaSOver2Pi * plus.im;
    k.reMinus = alphaSOver2Pi * minus.re;
    return k;
}

}  // namespace

std::pair<torch::Tensor, torch::Tensor> DVCSCFFTorch::evaluate(
        const Grid& grid, const Samples& samples, const Scalars& scalars,
        PerturbativeQCDOrderType::Type qcdOrder, bool polarized) {

    if (qcdOrder != PerturbativeQCDOrderType::LO
            && qcdOrder != PerturbativeQCDOrderType::NLO) {
        throw ElemUtils::CustomException("DVCSCFFTorch", __func__,
                "Perturbative QCD order can only be LO or NLO");
    }
    const bool nlo = (qcdOrder == PerturbativeQCDOrderType::NLO);

    const double sumSqrCharges = DVCSCFFKernels::sumSquaredCharges(scalars.nf);
    if (sumSqrCharges < 0.) {
        throw ElemUtils::CustomException("DVCSCFFTorch", __func__,
                ElemUtils::Formatter()
                        << "Number of active quark flavours should be between 3 and 6, got "
                        << scalars.nf);
    }

    const torch::Tensor xi = scalars.xi.to(torch::kDouble).reshape( { -1, 1 }); // {P,1}
    const torch::Tensor lq = scalars.logQ2OverMu2.to(torch::kDouble).reshape( {
            -1, 1 });
    const torch::Tensor as = scalars.alphaSOver2Pi.to(torch::kDouble).reshape( {
            -1, 1 });
    const torch::Tensor xiFlat = xi.reshape( { -1 });                     // {P}

    // Sub-interval integrals, eq. (8): the quark sector. The sign of the K(-x)
    // terms distinguishes vector (H, E) from axial (Ht, Et), exactly as in
    // DVCSCFFStandard::ConvolReKernelQuark{1,2}{V,A}.
    const double sV = polarized ? +1. : -1.;

    const torch::Tensor D = samples.quarkDiagonal.reshape( { -1, 1 });     // {P,1}
    const KernelOnGrid k1 = quarkKernel(grid.x1, xi, lq, as, nlo, polarized);
    const KernelOnGrid k2 = quarkKernel(grid.x2, xi, lq, as, nlo, polarized);

    // [0, xi]:  (E - D) K(x) + (sV E - D) K(-x)
    const torch::Tensor integrand1 = (samples.quark1 - D) * k1.rePlus
            + (sV * samples.quark1 - D) * k1.reMinus;
    // [xi, 1]:  (E - D) K(x) + sV K(-x) E
    const torch::Tensor integrand2 = (samples.quark2 - D) * k2.rePlus
            + sV * k2.reMinus * samples.quark2;
    // imaginary part, [xi, 1]:  (E - D) Im K(x)
    const torch::Tensor integrandIm = (samples.quark2 - D) * k2.imPlus;

    torch::Tensor re = ((grid.w1 * integrand1).sum(1)
            + (grid.w2 * integrand2).sum(1)) / xiFlat;
    torch::Tensor im = (grid.w2 * integrandIm).sum(1) / xiFlat;

    // Subtraction constants, appendix B: scalar per point.
    const int64_t P = xiFlat.size(0);
    std::vector<double> subReQ(P), subImQ(P), subReG(P), subImG(P);
    {
        const torch::Tensor xiCpu = xiFlat.to(torch::kCPU).contiguous();
        const torch::Tensor lqCpu = lq.reshape( { -1 }).to(torch::kCPU).contiguous();
        const torch::Tensor asCpu = as.reshape( { -1 }).to(torch::kCPU).contiguous();
        // Plain accessors: this loop runs once per evaluate() call, and a
        // training loop calls evaluate() at every step for thousands of points.
        const auto xiA = xiCpu.accessor<double, 1>();
        const auto lqA = lqCpu.accessor<double, 1>();
        const auto asA = asCpu.accessor<double, 1>();
        for (int64_t p = 0; p < P; ++p) {
            const double xiP = xiA[p];
            const double zeta = 2. * xiP / (1. + xiP);
            const DVCSCFFKernels::SubtractionConstants s =
                    DVCSCFFKernels::subtractionConstants(zeta, xiP, lqA[p],
                            asA[p], NumA::MathUtils::DiLog(1. - 1. / zeta), nlo,
                            polarized);
            subReQ[p] = s.realQuark;
            subImQ[p] = s.imaginaryQuark;
            subReG[p] = s.realGluon;
            subImG[p] = s.imaginaryGluon;
        }
    }
    const torch::TensorOptions opts = xiFlat.options();
    const torch::Tensor Dflat = samples.quarkDiagonal.reshape( { -1 });
    re = re + Dflat * torch::tensor(subReQ, torch::kDouble).to(opts.device());
    im = im + Dflat * torch::tensor(subImQ, torch::kDouble).to(opts.device());

    // Gluon sector, eq. (9): NLO only (the LO gluon kernel is zero).
    if (nlo) {
        if (!samples.gluon1.defined() || !samples.gluon2.defined()
                || !samples.gluonDiagonal.defined()) {
            throw ElemUtils::CustomException("DVCSCFFTorch", __func__,
                    "NLO evaluation needs the gluon samples");
        }
        // The gluon K(-x) sign pattern is the mirror of the quark one, as in
        // DVCSCFFStandard::ConvolReKernelGluon{1,2}{V,A}.
        const double sG = polarized ? -1. : +1.;
        const torch::Tensor Dg = samples.gluonDiagonal.reshape( { -1, 1 });
        const KernelOnGrid g1 = gluonKernel(grid.x1, xi, lq, as, scalars.nf,
                polarized);
        const KernelOnGrid g2 = gluonKernel(grid.x2, xi, lq, as, scalars.nf,
                polarized);

        const torch::Tensor gIntegrand1 = (samples.gluon1 - Dg) * g1.rePlus
                + (sG * samples.gluon1 - Dg) * g1.reMinus;
        const torch::Tensor gIntegrand2 = (samples.gluon2 - Dg) * g2.rePlus
                + sG * g2.reMinus * samples.gluon2;
        const torch::Tensor gIntegrandIm = (samples.gluon2 - Dg) * g2.imPlus;

        const torch::Tensor norm = xiFlat * xiFlat
                * static_cast<double>(scalars.nf);
        const torch::Tensor Dgflat = samples.gluonDiagonal.reshape( { -1 });
        torch::Tensor reG = ((grid.w1 * gIntegrand1).sum(1)
                + (grid.w2 * gIntegrand2).sum(1)) / norm;
        torch::Tensor imG = (grid.w2 * gIntegrandIm).sum(1) / norm;
        reG = reG + Dgflat * torch::tensor(subReG, torch::kDouble).to(opts.device());
        imG = imG + Dgflat * torch::tensor(subImG, torch::kDouble).to(opts.device());

        // Multiplication by the charge sum corrects a mistake in eq. (9), as
        // in DVCSCFFStandard.
        re = re + sumSqrCharges * reG;
        im = im + sumSqrCharges * imG;
    }

    return std::make_pair(re, im);
}

} /* namespace PARTONS */

#endif /* PARTONS_WITH_TORCH */
