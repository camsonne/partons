#ifndef DVCS_CFF_KERNELS_H
#define DVCS_CFF_KERNELS_H

/**
 * @file DVCSCFFKernels.h
 *
 * The DVCS coefficient functions (Wilson coefficients) and subtraction
 * constants used by DVCSCFFStandard, written once as templates on the scalar
 * type so a single copy of each formula serves two backends:
 *
 *   - T = double: exactly the scalar evaluation DVCSCFFStandard has always
 *     done, one x at a time inside NumA's adaptive integrator. DVCSCFFStandard's
 *     Kernel*() methods delegate here, so the default build is unchanged.
 *
 *   - T = torch::Tensor (see DVCSCFFKernelsTorch.h, compiled only with
 *     PARTONS_WITH_TORCH): the same formula evaluated on a whole batch of x at
 *     once -- every node of a fixed quadrature grid, for every kinematic point
 *     -- which turns the convolution into batched tensor algebra with native
 *     autograd. That is the LibTorch backend, DVCSCFFTorch.
 *
 * Nothing here includes torch; the tensor backend supplies its own
 * KernelTraits specialisation. Formulas: Moutarde, Pire, Sabatie, Szymanowski,
 * Wagner, Phys. Rev. D 87 (2013) 054029, appendix A (kernels) and appendix B
 * (subtraction constants), in the form implemented by DVCSCFFStandard.
 *
 * Complex values are carried as a (re, im) pair of T (Cplx<T>) because
 * std::complex<torch::Tensor> does not exist; Cplx<double> converts to
 * std::complex<double> with toStdComplex().
 *
 * Arithmetic between T and plain double literals is relied on throughout
 * (valid for double and for torch::Tensor alike); only the few places that need
 * a *value of type T* with a given shape, such as the branches of a where(), go
 * through KernelTraits<T>::constant().
 */

#include <cmath>
#include <complex>

namespace PARTONS {
namespace DVCSCFFKernels {

/**
 * The handful of scalar operations the kernel formulas need, resolved per
 * scalar type. The double specialisation lives here, the torch::Tensor one in
 * DVCSCFFKernelsTorch.h.
 */
template<class T>
struct KernelTraits;

template<>
struct KernelTraits<double> {
    typedef bool Mask;
    static double log(double x) {
        return std::log(x);
    }
    static Mask lt(double a, double b) {
        return a < b;
    }
    static Mask gt(double a, double b) {
        return a > b;
    }
    static double where(Mask c, double a, double b) {
        return c ? a : b;
    }
    /// A value v with the same "shape" as `like` (trivial for a scalar).
    static double constant(double v, double /*like*/) {
        return v;
    }
};

/**
 * Minimal complex pair on T. Complex-complex products are the member
 * operators; anything (double literal or T) can be added to, subtracted
 * from, or multiply/divide a Cplx through the templated operators.
 */
template<class T>
struct Cplx {
    T re;
    T im;

    Cplx(const T& r, const T& i) :
            re(r), im(i) {
    }

    Cplx operator+(const Cplx& o) const {
        return Cplx(re + o.re, im + o.im);
    }
    Cplx operator-(const Cplx& o) const {
        return Cplx(re - o.re, im - o.im);
    }
    Cplx operator-() const {
        return Cplx(-re, -im);
    }
    Cplx operator*(const Cplx& o) const {
        return Cplx(re * o.re - im * o.im, re * o.im + im * o.re);
    }

    template<class S>
    Cplx operator*(const S& s) const {
        return Cplx(re * s, im * s);
    }
    template<class S>
    Cplx operator/(const S& s) const {
        return Cplx(re / s, im / s);
    }
    template<class S>
    Cplx operator+(const S& s) const {
        return Cplx(re + s, im);
    }
    template<class S>
    Cplx operator-(const S& s) const {
        return Cplx(re - s, im);
    }
};

inline std::complex<double> toStdComplex(const Cplx<double>& c) {
    return std::complex<double>(c.re, c.im);
}

/// The colour factor ( Nc^2 - 1 ) / ( 2 Nc ) for Nc = 3.
const double CF = 4. / 3.;

/**
 * log( ( 1 - z ) / 2 ) continued below the cut: real for x < xi, and
 * log( ( z - 1 ) / 2 ) - i pi for x > xi, with z = x / xi. Exactly zero at
 * x = xi, as in DVCSCFFStandard. Appears in every NLO kernel.
 *
 * @param x  Momentum fraction(s): a scalar, or a batch (tensor).
 * @param xi Skewness; for a batch, broadcastable against x (e.g. {P, 1}).
 */
template<class T>
inline Cplx<T> logOneMinusZ(const T& x, const T& xi) {
    typedef KernelTraits<T> K;
    const T z = x / xi;
    const T zero = K::constant(0., x);
    const T half = K::constant(0.5, x);
    // Both branches are *selected* by where() but, for a batch, both are also
    // *evaluated*: guard their arguments so the branch not taken never feeds a
    // non-positive number to the log.
    const T below = K::log(K::where(K::lt(x, xi), (1. - z) * 0.5, half));
    const T above = K::log(K::where(K::gt(x, xi), (z - 1.) * 0.5, half));
    const T re = K::where(K::lt(x, xi), below,
            K::where(K::gt(x, xi), above, zero));
    const T im = K::where(K::gt(x, xi), K::constant(-M_PI, x), zero);
    return Cplx<T>(re, im);
}

/**
 * Leading-order quark kernel, appendix A eq. (A1): 1 / ( 1 - z ), z = x / xi.
 * Identical for the vector (H, E) and axial (Ht, Et) cases. Real.
 */
template<class T>
inline T quarkLO(const T& x, const T& xi) {
    return 1. / (1. - x / xi);
}

/**
 * NLO quark kernel, axial case T^{q, NLO, A}, appendix A eq. (A2), without
 * the alpha_s / ( 2 pi ) factor.
 *
 * @param logQ2OverMu2 log( Q^2 / muF^2 ); for a batch, broadcastable against x.
 */
template<class T>
inline Cplx<T> quarkNLOA(const T& x, const T& xi, const T& logQ2OverMu2) {
    typedef KernelTraits<T> K;
    const T z = x / xi;
    const Cplx<T> L = logOneMinusZ(x, xi);

    Cplx<T> r(logQ2OverMu2, K::constant(0., x));
    r = r + (L / 2. - 3. / 4.);
    r = r * (L * 2. + 3.);
    r = r - 27. / 4. - L * ((1. - z) / (1. + z));
    r = r * (CF / (2. * (1. - z)));
    return r;
}

/**
 * NLO quark kernel, vector case T^{q, NLO, V}, appendix A eq. (A2), without
 * the alpha_s / ( 2 pi ) factor.
 */
template<class T>
inline Cplx<T> quarkNLOV(const T& x, const T& xi, const T& logQ2OverMu2) {
    const T z = x / xi;
    const Cplx<T> L = logOneMinusZ(x, xi);
    return quarkNLOA(x, xi, logQ2OverMu2) - L * (CF / (1. + z));
}

/**
 * NLO gluon kernel, axial case T^{g, NLO, A}, appendix A eq. (A2), without
 * the alpha_s / ( 2 pi ) factor.
 *
 * @param nf Number of active flavours.
 */
template<class T>
inline Cplx<T> gluonNLOA(const T& x, const T& xi, const T& logQ2OverMu2,
        double nf) {
    typedef KernelTraits<T> K;
    const T z = x / xi;
    const T onePlusZ2 = (1. + z) * (1. + z);
    const Cplx<T> L = logOneMinusZ(x, xi);

    Cplx<T> r = L + (logQ2OverMu2 - 2.);
    r = r * (Cplx<T>(1. / (1. - z * z), K::constant(0., x)) + L / onePlusZ2);
    r = r - (L * L) / (onePlusZ2 * 2.);
    r = r * (nf / 2.);
    return r;
}

/**
 * NLO gluon kernel, vector case T^{g, NLO, V}, appendix A eq. (A2), without
 * the alpha_s / ( 2 pi ) factor.
 */
template<class T>
inline Cplx<T> gluonNLOV(const T& x, const T& xi, const T& logQ2OverMu2,
        double nf) {
    const T z = x / xi;
    const Cplx<T> L = logOneMinusZ(x, xi);

    Cplx<T> r = L + (logQ2OverMu2 - 2.);
    r = r / (1. - z);
    r = r + L / (1. + z);
    r = r * (nf / 2.);
    r = r - gluonNLOA(x, xi, logQ2OverMu2, nf);
    return r;
}

/**
 * Subtraction constants of eqs. (8), (9) and appendix B: the coefficients of
 * the diagonal GPD value ( GPD at x = xi ) in the real and imaginary parts of
 * the CFF, for the quark and the gluon sector. They depend on the kinematics
 * only, not on x, so they are plain doubles for both backends (the tensor
 * backend evaluates them once per kinematic point). Shared by
 * DVCSCFFStandard::computeSubtractionFunctionsV/A and DVCSCFFTorch.
 */
struct SubtractionConstants {
    double realQuark;
    double imaginaryQuark;
    double realGluon;
    double imaginaryGluon;
};

/**
 * @param zeta          2 xi / ( 1 + xi ).
 * @param xi            Skewness.
 * @param logQ2OverMu2  log( Q^2 / muF^2 ).
 * @param alphaSOver2Pi alpha_s( muR^2 ) / ( 2 pi ); unused at LO.
 * @param diLogInvZeta  Li2( 1 - 1 / zeta ). Supplied by the caller (from
 *                      NumA::MathUtils::DiLog) to keep this header free of
 *                      dependencies.
 * @param nlo           Add the NLO terms.
 * @param polarized     Axial (Ht, Et) instead of vector (H, E).
 */
inline SubtractionConstants subtractionConstants(double zeta, double xi,
        double logQ2OverMu2, double alphaSOver2Pi, double diLogInvZeta,
        bool nlo, bool polarized) {

    const double LogZeta = std::log(zeta);
    const double LogInvZeta = std::log((1. - zeta) / zeta);
    const double LogInvZeta2 = LogInvZeta * LogInvZeta;
    const double Pi2 = M_PI * M_PI;

    SubtractionConstants s;

    // LO, eq. (B2): 1 / ( 1 - z ) integrated against a constant.
    s.realQuark = -LogInvZeta;
    s.imaginaryQuark = M_PI;
    s.realGluon = 0.;
    s.imaginaryGluon = 0.;

    if (!nlo) {
        return s;
    }

    double realQuarkNLO, imaginaryQuarkNLO, realGluonNLO, imaginaryGluonNLO;

    if (!polarized) {
        // NLO, quark, vector, eq. (B4)
        realQuarkNLO = Pi2 / 2. - 3. * diLogInvZeta
                + LogInvZeta * (Pi2 + 9. + 3. * LogZeta - LogInvZeta2 / 3.);
        realQuarkNLO += logQ2OverMu2 * (Pi2 - 3. * LogInvZeta - LogInvZeta2);
        realQuarkNLO *= CF / 2.;

        imaginaryQuarkNLO = Pi2 / 3. + 9. + 3. * LogZeta - LogInvZeta2
                - logQ2OverMu2 * (2. * LogInvZeta + 3);
        imaginaryQuarkNLO *= -M_PI * CF / 2.;

        // NLO, gluon, vector, eq. (B6)
        realGluonNLO = -1. + Pi2 / 3. * (1. - 3. / 4. * zeta) + diLogInvZeta
                - LogZeta * LogInvZeta;
        realGluonNLO += (2. - zeta) * LogInvZeta * (1. - LogInvZeta / 4.);
        realGluonNLO += logQ2OverMu2 / 2. * (1. - (2. - zeta) * LogInvZeta);
        realGluonNLO *= 1 / (2. * xi);

        imaginaryGluonNLO = (2. - zeta) * (2. - logQ2OverMu2 - LogInvZeta)
                - 2. * LogZeta;
        imaginaryGluonNLO *= -M_PI / (4. * xi);
    } else {
        // NLO, quark, axial, eq. (B3)
        realQuarkNLO = Pi2 / 6. - diLogInvZeta
                + LogInvZeta * (Pi2 + 9. + LogZeta - LogInvZeta2 / 3.);
        realQuarkNLO += logQ2OverMu2 * (Pi2 - 3. * LogInvZeta - LogInvZeta2);
        realQuarkNLO *= CF / 2.;

        imaginaryQuarkNLO = Pi2 / 3. + 9. + LogZeta - LogInvZeta2
                - logQ2OverMu2 * (2. * LogInvZeta + 3);
        imaginaryQuarkNLO *= -M_PI * CF / 2.;

        // NLO, gluon, axial, eq. (B5)
        realGluonNLO = 1. + Pi2 / 4. * zeta
                + zeta * LogInvZeta * (1. - LogInvZeta / 4.);
        realGluonNLO += -logQ2OverMu2 / 2. * (1. + zeta * LogInvZeta);
        realGluonNLO *= 1 / (2. * xi);

        imaginaryGluonNLO = 2. - LogInvZeta - logQ2OverMu2;
        imaginaryGluonNLO *= -M_PI * zeta / (4. * xi);
    }

    s.realQuark += alphaSOver2Pi * realQuarkNLO;
    s.realGluon += alphaSOver2Pi * realGluonNLO;
    s.imaginaryQuark += alphaSOver2Pi * imaginaryQuarkNLO;
    s.imaginaryGluon += alphaSOver2Pi * imaginaryGluonNLO;

    return s;
}

/**
 * Sum of the squared electric charges of the active quark flavours, as used
 * for the gluon sector in eq. (9). Returns a negative number for an
 * unsupported nf; callers validate nf in [3, 6].
 */
inline double sumSquaredCharges(unsigned int nf) {
    switch (nf) {
    case 3:
        return 2. / 3.;
    case 4:
        return 10. / 9.;
    case 5:
        return 11. / 9.;
    case 6:
        return 15. / 9.;
    default:
        return -1.;
    }
}

} /* namespace DVCSCFFKernels */
} /* namespace PARTONS */

#endif /* DVCS_CFF_KERNELS_H */
