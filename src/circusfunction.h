//
// Configuration
//

// Include guard
#ifndef CIRCUSFUNCTION_H
#define CIRCUSFUNCTION_H

// System includes
#include <limits>

// OpenCV includes
#include <cv.h>

// Local includes
#include "auxiliary.h"


//
// Routines
//

cv::Mat nearest_orthonormal_sinogram(
	const cv::Mat &input,
	unsigned int& new_center)
{
	// Detect the offset of each column to the sinogram center
	assert(input.rows > 0);
	assert(input.cols >= 0);
	unsigned int sinogram_center = (unsigned int) std::floor((input.rows - 1) / 2.0);
	std::vector<int> offset(input.cols);
	for (int p = 0; p < input.cols; p++) {
		size_t median = findWeighedMedian(
			input.ptr<double>(p),
			input.cols);
		offset[p] = median- sinogram_center;
	}

	// Align each column to the sinogram center
	int min = *(std::min_element(offset.begin(), offset.end()));
	int max = *(std::max_element(offset.begin(), offset.end()));
	unsigned int padding = max + std::abs(min);
	new_center = sinogram_center + max;
	cv::Mat aligned = cv::Mat::zeros(
		input.rows + padding,
		input.cols,
		input.type()
	);
	for (int i = 0; i < input.rows; i++) {
		for (int j = 0; j < input.cols; j++) {
			aligned.at<double>(max+i-offset[j], j) = 
				input.at<double>(i, j);
		}
	}

	// Compute the nearest orthonormal sinogram
	cv::SVD svd(aligned, cv::SVD::FULL_UV);
	cv::Mat diagonal = cv::Mat::eye(
		aligned.size(),	// (often) rectangular!
		aligned.type()
	);
	cv::Mat nos = svd.u * diagonal * svd.vt;

	return nos;
}

cv::Mat getCircusFunction(
	const cv::Mat &input,
	Functional pfunctional,
	void* pfunctional_arguments)
{
	assert(input.type() == CV_64FC1);

	// TODO: rotate (all rows were cols)
	// (and debug)

	// Allocate the output matrix
	cv::Mat output(
		cv::Size(input.rows, 1),
		input.type()
	);

	// Trace all rows
	for (int p = 0; p < input.rows; p++) {
		output.at<double>(
			0,	// row
			p	// column
		) = pfunctional(
			input.ptr<double>(p),
			input.cols,
			pfunctional_arguments);
	}

	return output;
}

#endif
