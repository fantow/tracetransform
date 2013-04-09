//
// Configuration
//

// Include guard
#ifndef TRACETRANSFORM_CIRCUS_HPP
#define TRACETRANSFORM_CIRCUS_HPP

// Standard library
#include <cstddef>

// Eigen
#define EIGEN_DEFAULT_DENSE_INDEX_TYPE std::size_t
#include <Eigen/Dense>

// Local
#include "wrapper.hpp"


//
// Module definitions
//

Eigen::MatrixXd nearest_orthonormal_sinogram(
        const Eigen::MatrixXd &input,
        unsigned int& new_center);

Eigen::VectorXd getCircusFunction(
        const Eigen::MatrixXd &input,
        FunctionalWrapper *pfunctional);

#endif
