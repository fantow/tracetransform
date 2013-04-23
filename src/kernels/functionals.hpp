//
// Configuration
//

// Include guard
#ifndef _TRACETRANSFORM_KERNELS_FUNCTIONALS_
#define _TRACETRANSFORM_KERNELS_FUNCTIONALS_

// Local
#include "../cudahelper/memory.hpp"


//
// Routines
//

// T functionals
extern CUDAHelper::GlobalMemory<float> *TFunctionalRadon(
                const CUDAHelper::GlobalMemory<float> *input, int rows,
                int cols, CUDAHelper::GlobalMemory<float> *output, size_t a);
extern CUDAHelper::GlobalMemory<float> *TFunctional1(
                const CUDAHelper::GlobalMemory<float> *input, int rows,
                int cols, CUDAHelper::GlobalMemory<float> *output, size_t a);
extern CUDAHelper::GlobalMemory<float> *TFunctional2(
                const CUDAHelper::GlobalMemory<float> *input, int rows,
                int cols, CUDAHelper::GlobalMemory<float> *output, size_t a);
extern CUDAHelper::GlobalMemory<float> *TFunctional3(
                const CUDAHelper::GlobalMemory<float> *input, int rows,
                int cols, CUDAHelper::GlobalMemory<float> *output, size_t a);
extern CUDAHelper::GlobalMemory<float> *TFunctional4(
                const CUDAHelper::GlobalMemory<float> *input, int rows,
                int cols, CUDAHelper::GlobalMemory<float> *output, size_t a);
extern CUDAHelper::GlobalMemory<float> *TFunctional5(
                const CUDAHelper::GlobalMemory<float> *input, int rows,
                int cols, CUDAHelper::GlobalMemory<float> *output, size_t a);

// P-functionals
extern CUDAHelper::GlobalMemory<float> *PFunctional1(
                const CUDAHelper::GlobalMemory<float> *input, int rows,
                int cols, int a);
extern CUDAHelper::GlobalMemory<float> *PFunctional2(
                const CUDAHelper::GlobalMemory<float> *input, int rows,
                int cols, int a);
extern CUDAHelper::GlobalMemory<float> *PFunctional3(
                const CUDAHelper::GlobalMemory<float> *input, int rows,
                int cols, int a);
extern CUDAHelper::GlobalMemory<float> *PFunctionalHermite(
                const CUDAHelper::GlobalMemory<float> *input, int rows,
                int cols, int a, unsigned int order, size_t center);

#endif
