/*
 * Check program for the LibTorch backend (built only with PARTONS_WITH_TORCH).
 *
 * Runs, on the standard GPD model GPDGK16 over a grid of kinematics, at LO and
 * NLO, for H, E, Ht and Et:
 *
 *   1. DVCSCFFStandard (NumA's adaptive DEXP integration) and DVCSCFFTorch (the
 *      same physics on a fixed tanh-sinh grid, as tensors) through the ordinary
 *      module interface, and compares them. The two must agree to the accuracy
 *      of the adaptive oracle, which targets 1e-3 absolute on each integral and
 *      warns when it cannot reach it.
 *
 *   2. DVCSCFFTorch's batched tensor API (makeGrid / sampleGPD / evaluate) on
 *      all kinematic points at once against the per-point module results:
 *      identical math, so they must agree to rounding.
 *
 *   3. Autograd through evaluate(): the gradient of the CFF w.r.t. a GPD sample
 *      against a finite difference, which is what a neural-network GPD fitted
 *      through these coefficient functions relies on.
 *
 * Options:
 *   --dump   print DVCSCFFStandard's results only ("order type xi t Q2 Re Im"),
 *            for regression checks of the shared kernels across refactors.
 *   --level N  tanh-sinh refinement level for DVCSCFFTorch (default 5).
 *
 * Exit status: the number of failed checks.
 */

#include <ElementaryUtils/logger/CustomException.h>
#include <ElementaryUtils/parameters/Parameters.h>
#include <partons/Partons.h>
#include <partons/ModuleObjectFactory.h>
#include <partons/ServiceObjectRegistry.h>
#include <partons/services/DVCSConvolCoeffFunctionService.h>
#include <partons/modules/convol_coeff_function/DVCS/DVCSCFFStandard.h>
#include <partons/modules/convol_coeff_function/DVCS/DVCSCFFTorch.h>
#include <partons/modules/gpd/GPDGK16.h>
#include <partons/modules/running_alpha_strong/RunningAlphaStrongStandard.h>
#include <partons/beans/convol_coeff_function/DVCS/DVCSConvolCoeffFunctionKinematic.h>
#include <partons/beans/convol_coeff_function/DVCS/DVCSConvolCoeffFunctionResult.h>
#include <partons/beans/PerturbativeQCDOrderType.h>
#include <partons/FundamentalPhysicalConstants.h>
#include <torch/torch.h>

#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct Kin {
    double xi, t, Q2;
};

const double kXis[] = { 0.05, 0.1, 0.2, 0.35, 0.5 };
const double kTs[] = { -0.1, -0.3 };
const double kQ2s[] = { 2.0, 4.0 };
const PARTONS::GPDType::Type kTypes[] = { PARTONS::GPDType::H,
        PARTONS::GPDType::E, PARTONS::GPDType::Ht, PARTONS::GPDType::Et };
const char* kTypeNames[] = { "H", "E", "Ht", "Et" };

std::vector<Kin> kinematics() {
    std::vector<Kin> k;
    for (double xi : kXis)
        for (double t : kTs)
            for (double Q2 : kQ2s)
                k.push_back( { xi, t, Q2 });
    return k;
}

int g_failures = 0;

void check(const char* label, double err, double tol) {
    std::printf("  %-64s err=%.2e  %s\n", label, err, err <= tol ? "OK" : "FAIL");
    if (!(err <= tol)) ++g_failures;
}

PARTONS::DVCSConvolCoeffFunctionModule* makeModule(unsigned int classId,
        PARTONS::GPDModule* gpd, PARTONS::PerturbativeQCDOrderType::Type order) {
    auto* f = PARTONS::Partons::getInstance()->getModuleObjectFactory();
    auto* ccf = f->newDVCSConvolCoeffFunctionModule(classId);
    ElemUtils::Parameters prm(
            PARTONS::PerturbativeQCDOrderType::PARAMETER_NAME_PERTURBATIVE_QCD_ORDER_TYPE,
            order);
    ccf->configure(prm);
    ccf->setGPDModule(gpd);
    return ccf;
}

}  // namespace

int main(int argc, char** argv) {

    bool dump = false;
    unsigned int level = 5;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--dump") == 0) dump = true;
        else if (std::strcmp(argv[i], "--level") == 0 && i + 1 < argc) level = std::atoi(argv[++i]);
    }

    PARTONS::Partons* partons = PARTONS::Partons::getInstance();
    try {
        partons->init(argc, argv);
        auto* factory = partons->getModuleObjectFactory();
        auto* service =
                partons->getServiceObjectRegistry()->getDVCSConvolCoeffFunctionService();
        auto* gpd = factory->newGPDModule(PARTONS::GPDGK16::classId);
        const std::vector<Kin> kins = kinematics();

        // ---------------------------------------------------------------------
        if (dump) {
            std::printf("# order type xi t Q2 Re Im\n");
            for (int order = 0; order < 2; ++order) {
                auto ord = order == 0 ? PARTONS::PerturbativeQCDOrderType::LO
                                      : PARTONS::PerturbativeQCDOrderType::NLO;
                auto* std_ = makeModule(PARTONS::DVCSCFFStandard::classId, gpd, ord);
                for (const Kin& k : kins) {
                    PARTONS::DVCSConvolCoeffFunctionKinematic kin(k.xi, k.t, k.Q2, k.Q2, k.Q2);
                    auto res = service->computeSingleKinematic(kin, std_);
                    for (int i = 0; i < 4; ++i) {
                        std::complex<double> c = res.getResult(kTypes[i]);
                        std::printf("%s %s %.6g %.6g %.6g %.17g %.17g\n",
                                order == 0 ? "LO" : "NLO", kTypeNames[i], k.xi, k.t,
                                k.Q2, c.real(), c.imag());
                    }
                }
                factory->updateModulePointerReference(std_, 0);
            }
            factory->updateModulePointerReference(gpd, 0);
            partons->close();
            return 0;
        }

        std::printf("DVCSCFFTorch check (GPDGK16, tanh-sinh level %u)\n", level);

        // ---------------------------------------------------------------------
        // 1. Module path: DVCSCFFTorch vs DVCSCFFStandard.
        //
        // The adaptive oracle targets 1e-3 absolute per integral (and warns when
        // it fails to get there), so the comparison is at that level: relative
        // to the size of the CFF, with a 1e-3 floor for values near zero.
        std::printf("-- module path: DVCSCFFTorch vs DVCSCFFStandard --\n");
        std::printf("  %-5s %-3s %-5s %-5s %-4s  %-26s %-26s  %s\n", "order",
                "GPD", "xi", "t", "Q2", "Standard (Re, Im)", "Torch (Re, Im)", "rel.diff");

        // Module results kept for check 2.
        std::vector<std::vector<std::complex<double> > > torchResults(2);
        double tStandard = 0., tTorch = 0.;

        for (int order = 0; order < 2; ++order) {
            auto ord = order == 0 ? PARTONS::PerturbativeQCDOrderType::LO
                                  : PARTONS::PerturbativeQCDOrderType::NLO;
            auto* std_ = makeModule(PARTONS::DVCSCFFStandard::classId, gpd, ord);
            auto* tor_ = makeModule(PARTONS::DVCSCFFTorch::classId, gpd, ord);
            static_cast<PARTONS::DVCSCFFTorch*>(tor_)->setQuadratureLevel(level);

            double worst = 0.;
            for (const Kin& k : kins) {
                PARTONS::DVCSConvolCoeffFunctionKinematic kin(k.xi, k.t, k.Q2, k.Q2, k.Q2);
                auto t0 = std::chrono::steady_clock::now();
                auto rs = service->computeSingleKinematic(kin, std_);
                auto t1 = std::chrono::steady_clock::now();
                auto rt = service->computeSingleKinematic(kin, tor_);
                auto t2 = std::chrono::steady_clock::now();
                tStandard += std::chrono::duration<double>(t1 - t0).count();
                tTorch += std::chrono::duration<double>(t2 - t1).count();

                for (int i = 0; i < 4; ++i) {
                    std::complex<double> a = rs.getResult(kTypes[i]);
                    std::complex<double> b = rt.getResult(kTypes[i]);
                    torchResults[order].push_back(b);
                    double scale = std::max(std::abs(a), 1e-3);
                    double rel = std::abs(a - b) / scale;
                    worst = std::max(worst, rel);
                    std::printf("  %-5s %-3s %-5.3g %-5.2g %-4.2g  %+12.5e %+12.5e  %+12.5e %+12.5e  %.1e\n",
                            order == 0 ? "LO" : "NLO", kTypeNames[i], k.xi, k.t, k.Q2,
                            a.real(), a.imag(), b.real(), b.imag(), rel);
                }
            }
            check(order == 0 ? "LO:  max |Torch - Standard| / max(|Standard|, 1e-3)"
                             : "NLO: max |Torch - Standard| / max(|Standard|, 1e-3)",
                    worst, order == 0 ? 1e-4 : 5e-3);

            factory->updateModulePointerReference(std_, 0);
            factory->updateModulePointerReference(tor_, 0);
        }
        std::printf("  module-path wall time over %zu points x 4 GPDs x 2 orders: "
                    "Standard %.2f s, Torch %.2f s\n", kins.size(), tStandard, tTorch);

        // ---------------------------------------------------------------------
        // 2. Batched API on all points at once vs the per-point module results.
        std::printf("-- batched API (all %zu points in one evaluate()) vs module path --\n",
                kins.size());
        {
            std::vector<double> xi, t, Q2;
            for (const Kin& k : kins) { xi.push_back(k.xi); t.push_back(k.t); Q2.push_back(k.Q2); }
            const auto opts = torch::TensorOptions().dtype(torch::kDouble);
            auto xiT = torch::tensor(xi, opts);
            auto grid = PARTONS::DVCSCFFTorch::makeGrid(xiT, level);

            // alpha_s / 2pi and log(Q2/muF2) per point, as the module computes them
            // (muF2 = muR2 = Q2 here, so the log is 0).
            auto* alphaS = factory->newRunningAlphaStrongModule(
                    PARTONS::RunningAlphaStrongStandard::classId);
            std::vector<double> as(kins.size()), lq(kins.size(), 0.);
            for (size_t p = 0; p < kins.size(); ++p)
                as[p] = alphaS->compute(Q2[p]) / (2. * PARTONS::Constant::PI);
            PARTONS::DVCSCFFTorch::Scalars sc;
            sc.xi = xiT;
            sc.alphaSOver2Pi = torch::tensor(as, opts);
            sc.logQ2OverMu2 = torch::tensor(lq, opts);
            sc.nf = 3;

            auto t0 = std::chrono::steady_clock::now();
            double worst = 0.;
            for (int order = 0; order < 2; ++order) {
                auto ord = order == 0 ? PARTONS::PerturbativeQCDOrderType::LO
                                      : PARTONS::PerturbativeQCDOrderType::NLO;
                for (int i = 0; i < 4; ++i) {
                    bool polarized = (i >= 2);
                    auto samples = PARTONS::DVCSCFFTorch::sampleGPD(gpd, grid, xi, t, Q2, Q2, kTypes[i]);
                    auto cff = PARTONS::DVCSCFFTorch::evaluate(grid, samples, sc, ord, polarized);
                    for (size_t p = 0; p < kins.size(); ++p) {
                        std::complex<double> b = torchResults[order][p * 4 + i];
                        std::complex<double> c(cff.first[p].item<double>(), cff.second[p].item<double>());
                        worst = std::max(worst, std::abs(b - c) / std::max(std::abs(b), 1e-3));
                    }
                }
            }
            double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            check("batched evaluate() == per-point module results", worst, 1e-10);
            std::printf("  batched wall time (incl. GPD sampling): %.2f s\n", secs);

            // Time evaluate() alone (the tensor part) on the LO H samples.
            auto samples = PARTONS::DVCSCFFTorch::sampleGPD(gpd, grid, xi, t, Q2, Q2, PARTONS::GPDType::H);
            t0 = std::chrono::steady_clock::now();
            for (int r = 0; r < 20; ++r)
                (void) PARTONS::DVCSCFFTorch::evaluate(grid, samples, sc, PARTONS::PerturbativeQCDOrderType::NLO, false);
            secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() / 20.;
            std::printf("  evaluate() alone, NLO, %zu points: %.3f ms per call\n", kins.size(), 1e3 * secs);
            factory->updateModulePointerReference(alphaS, 0);

            // -----------------------------------------------------------------
            // 3. Autograd: d Re CFF / d (quark sample) vs finite difference.
            std::printf("-- autograd through evaluate() --\n");
            for (int order = 0; order < 2; ++order) {
                auto ord = order == 0 ? PARTONS::PerturbativeQCDOrderType::LO
                                      : PARTONS::PerturbativeQCDOrderType::NLO;
                PARTONS::DVCSCFFTorch::Samples s = samples;
                s.quark1 = samples.quark1.clone().set_requires_grad(true);
                s.quark2 = samples.quark2.clone().set_requires_grad(true);
                s.quarkDiagonal = samples.quarkDiagonal.clone().set_requires_grad(true);
                auto cff = PARTONS::DVCSCFFTorch::evaluate(grid, s, sc, ord, false);
                auto loss = cff.first.sum() + cff.second.sum();
                loss.backward();

                // Finite difference on one interior node of point 2 and on the
                // diagonal of point 3.
                const int64_t p = 2, n = grid.x2.size(1) / 2;
                const double eps = 1e-6;
                auto pert = samples.quark2.clone();
                pert[p][n] += eps;
                PARTONS::DVCSCFFTorch::Samples sp = samples; sp.quark2 = pert;
                auto cp = PARTONS::DVCSCFFTorch::evaluate(grid, sp, sc, ord, false);
                pert = samples.quark2.clone();
                pert[p][n] -= eps;
                sp.quark2 = pert;
                auto cm = PARTONS::DVCSCFFTorch::evaluate(grid, sp, sc, ord, false);
                double fd = ((cp.first.sum() + cp.second.sum()) - (cm.first.sum() + cm.second.sum())).item<double>() / (2 * eps);
                double ad = s.quark2.grad()[p][n].item<double>();
                char label[96];
                std::snprintf(label, sizeof label, "%s: d(sum CFF)/d quark2[%lld][%lld] autograd vs finite diff",
                        order == 0 ? "LO" : "NLO", (long long) p, (long long) n);
                check(label, std::abs(ad - fd) / std::max(std::abs(fd), 1e-8), 1e-5);

                const int64_t pd = 3;
                auto pertD = samples.quarkDiagonal.clone(); pertD[pd] += eps;
                sp = samples; sp.quarkDiagonal = pertD;
                cp = PARTONS::DVCSCFFTorch::evaluate(grid, sp, sc, ord, false);
                pertD = samples.quarkDiagonal.clone(); pertD[pd] -= eps;
                sp.quarkDiagonal = pertD;
                cm = PARTONS::DVCSCFFTorch::evaluate(grid, sp, sc, ord, false);
                fd = ((cp.first.sum() + cp.second.sum()) - (cm.first.sum() + cm.second.sum())).item<double>() / (2 * eps);
                ad = s.quarkDiagonal.grad()[pd].item<double>();
                std::snprintf(label, sizeof label, "%s: d(sum CFF)/d quarkDiagonal[%lld] autograd vs finite diff",
                        order == 0 ? "LO" : "NLO", (long long) pd);
                check(label, std::abs(ad - fd) / std::max(std::abs(fd), 1e-8), 1e-5);
            }
        }

        factory->updateModulePointerReference(gpd, 0);
        partons->close();
    } catch (const ElemUtils::CustomException& e) {
        std::fprintf(stderr, "PARTONS exception: %s\n", e.what());
        partons->close();
        return 1;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "exception: %s\n", e.what());
        partons->close();
        return 1;
    }

    std::printf(g_failures ? "\n%d FAILURE(S)\n" : "\nALL CHECKS PASSED\n", g_failures);
    return g_failures;
}
