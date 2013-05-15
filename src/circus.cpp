//
// Configuration
//

// Header include
#include "circus.hpp"

// Standard library
#include <cassert>
#include <cmath>
#include <algorithm>
#include <vector>

// Eigen
#include <Eigen/SVD>

// Local
#include "logger.hpp"
#include "auxiliary.hpp"
extern "C" {
        #include "functionals.h"
}
#include "kernels/functionals.hpp"


//
// Module definitions
//

std::istream& operator>>(std::istream& in, PFunctionalWrapper& wrapper)
{
        in >> wrapper.name;
        if (isdigit(wrapper.name[0]))
            wrapper.name = "P" + wrapper.name;
        if (wrapper.name == "P1") {
                wrapper.functional = PFunctional::P1;
        } else if (wrapper.name == "P2") {
                wrapper.functional = PFunctional::P2;
        } else if (wrapper.name == "P3") {
                wrapper.functional = PFunctional::P3;
        } else if (wrapper.name[0] == 'H') {
                wrapper.functional = PFunctional::Hermite;
                if (wrapper.name.size() < 2)
                        throw boost::program_options::validation_error(
                                boost::program_options::validation_error::invalid_option_value,
                                "Missing order parameter for Hermite P-functional");
                try {
                        wrapper.arguments.order = boost::lexical_cast<unsigned int>(wrapper.name.substr(1));
                }
                catch(boost::bad_lexical_cast &) {
                        throw boost::program_options::validation_error(
                                boost::program_options::validation_error::invalid_option_value,
                                "Unparseable order parameter for Hermite P-functional");
                }
        } else {
                throw boost::program_options::validation_error(
                        boost::program_options::validation_error::invalid_option_value,
                        "Unknown P-functional");
        }
    return in;
}

CUDAHelper::GlobalMemory<float> *nearest_orthonormal_sinogram(
        const CUDAHelper::GlobalMemory<float> *input,
        size_t &new_center)
{
        // TEMPORARY: download input
        Eigen::MatrixXf input_data(input->size(0), input->size(1));
        input->download(input_data.data());

        // Detect the offset of each column to the sinogram center
        assert(input_data.rows() > 0 && input_data.cols() > 0);
        int sinogram_center =  std::floor((input_data.rows() - 1) / 2.0);
        std::vector<int> offset(input_data.cols());  // TODO: Eigen vector
        for (int p = 0; p < input_data.cols(); p++) {
                size_t median = findWeighedMedian(
                                input_data.data() + p*input_data.rows(),
                                input_data.rows());
                offset[p] = median - sinogram_center;
        }

        // Align each column to the sinogram center
        int min = *(std::min_element(offset.begin(), offset.end()));
        int max = *(std::max_element(offset.begin(), offset.end()));
        assert(sgn(min) != sgn(max));
        int padding = (int) (std::abs(max) + std::abs(min));
        new_center = sinogram_center + max;
        // TODO: zeros?
        Eigen::MatrixXf aligned(input_data.rows() + padding, input_data.cols());
        for (int col = 0; col < input_data.cols(); col++) {
                for (int row = 0; row < input_data.rows(); row++) {
                        aligned(max+row-offset[col], col) = input_data(row, col);
                }
        }

        // Compute the nearest orthonormal sinogram
        Eigen::JacobiSVD<Eigen::MatrixXf, Eigen::ColPivHouseholderQRPreconditioner> svd(
                aligned, Eigen::ComputeFullU | Eigen::ComputeFullV);
        Eigen::MatrixXf diagonal = Eigen::MatrixXf::Identity(aligned.rows(), aligned.cols());
        Eigen::MatrixXf nos = svd.matrixU() * diagonal * svd.matrixV().transpose();

        // TEMPORARY: upload input
        CUDAHelper::GlobalMemory<float> *nos_mem = new CUDAHelper::GlobalMemory<float>(CUDAHelper::size_2d(nos.rows(), nos.cols()));
        nos_mem->upload(nos.data());

        return nos_mem;
}

CUDAHelper::GlobalMemory<float> *getCircusFunction(
        const CUDAHelper::GlobalMemory<float> *input,
        const PFunctionalWrapper &pfunctional)
{
        const int rows = input->size(0);
        const int cols = input->size(1);

        // Allocate the output matrix
        CUDAHelper::GlobalMemory<float> *output = new CUDAHelper::GlobalMemory<float>(CUDAHelper::size_1d(cols));

        // Trace all columns
        switch (pfunctional.functional) {
                case PFunctional::P1:
                        PFunctional1(input, output);
                        break;
                case PFunctional::P2:
                        PFunctional2(input, output);
                        break;
                case PFunctional::P3:
                        PFunctional3(input, output);
                        break;
                case PFunctional::Hermite:
                        PFunctionalHermite(input, output, *pfunctional.arguments.order, *pfunctional.arguments.center);
                        break;
        }

        return output;
}
