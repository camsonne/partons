#ifndef DVCS_CFF_KERNELS_TORCH_H
#define DVCS_CFF_KERNELS_TORCH_H

/**
 * @file DVCSCFFKernelsTorch.h
 *
 * The torch::Tensor instantiation of the DVCS coefficient-function templates in
 * DVCSCFFKernels.h: one specialisation of KernelTraits is all the formulas need
 * to run on a batch. Only available with PARTONS_WITH_TORCH (CMake option of
 * the same name); this header is empty otherwise so it can be included freely.
 *
 * Shapes: the kernels are evaluated on x of shape {P, N} (P kinematic points,
 * N quadrature nodes each) with xi, log(Q^2/muF^2) of shape {P, 1}, which
 * broadcast. Everything is double precision.
 */

#ifdef PARTONS_WITH_TORCH

#include <torch/torch.h>

#include "DVCSCFFKernels.h"

namespace PARTONS {
namespace DVCSCFFKernels {

template<>
struct KernelTraits<torch::Tensor> {
    typedef torch::Tensor Mask;
    static torch::Tensor log(const torch::Tensor& x) {
        return torch::log(x);
    }
    static Mask lt(const torch::Tensor& a, const torch::Tensor& b) {
        return a < b;
    }
    static Mask gt(const torch::Tensor& a, const torch::Tensor& b) {
        return a > b;
    }
    static torch::Tensor where(const Mask& c, const torch::Tensor& a,
            const torch::Tensor& b) {
        return torch::where(c, a, b);
    }
    static torch::Tensor constant(double v, const torch::Tensor& like) {
        return torch::full_like(like, v);
    }
};

} /* namespace DVCSCFFKernels */
} /* namespace PARTONS */

#endif /* PARTONS_WITH_TORCH */

#endif /* DVCS_CFF_KERNELS_TORCH_H */
