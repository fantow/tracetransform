//
// Configuration
//

// Standard library
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>

// Boost
#include <boost/program_options.hpp>
#include <boost/format.hpp>

// Eigen
#include <Eigen/Dense>

// CUDA
#include <cuda_runtime.h>

// Local
#include "logger.hpp"
#include "auxiliary.hpp"
#include "transform.hpp"
#include "cudahelper/errorhandling.hpp"


//
// Program option parsers
//

std::istream& operator>>(std::istream& in, TFunctionalWrapper& wrapper)
{
        in >> wrapper.name;
        wrapper.name = "T" + wrapper.name;
        if (wrapper.name == "T0") {
                wrapper.functional = TFunctional::Radon;
        } else if (wrapper.name == "T1") {
                wrapper.functional = TFunctional::T1;
        } else if (wrapper.name == "T2") {
                wrapper.functional = TFunctional::T2;
        } else if (wrapper.name == "T3") {
                wrapper.functional = TFunctional::T3;
        } else if (wrapper.name == "T4") {
                wrapper.functional = TFunctional::T4;
        } else if (wrapper.name == "T5") {
                wrapper.functional = TFunctional::T5;
        } else {
                throw boost::program_options::validation_error(
                        boost::program_options::validation_error::invalid_option_value,
                        "Unknown T-functional");
        }
        return in;
}

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


//
// Main application
//

int main(int argc, char **argv)
{
        //
        // Initialization
        //
        
        // List of functionals
        std::vector<TFunctionalWrapper> tfunctionals;
        std::vector<PFunctionalWrapper> pfunctionals;

        // Declare named options
        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
                ("help,h",
                        "produce help message")
                ("quiet,q",
                        "only display errors and warnings")
                ("verbose,v",
                        "display some more details")
                ("debug,d",
                        "display even more details")
                ("input,i",
                        boost::program_options::value<std::string>()
                        ->required(),
                        "image to process")
                ("t-functional,T",
                        boost::program_options::value<std::vector<TFunctionalWrapper>>(&tfunctionals)
                        ->required(),
                        "T-functionals")
                ("p-functional,P",
                        boost::program_options::value<std::vector<PFunctionalWrapper>>(&pfunctionals),
                        "P-functionals")
        ;

        // Declare positional options
        boost::program_options::positional_options_description pod;
        pod.add("input", -1);

        // Parse the options
        boost::program_options::variables_map vm;
        try {
                store(boost::program_options::command_line_parser(argc, argv)
                        .options(desc).positional(pod).run(), vm);
        }
        catch (const std::exception &e) {
                std::cerr << "Error " << e.what() << std::endl;

                std::cout << desc << std::endl;
                return 1;
        }

        // Display help
        if (vm.count("help")) {
                std::cout << desc << std::endl;
                return 0;
        }

        // Notify the user of errors
        try {
                notify(vm);
        }
        catch (const std::exception &e) {
                std::cerr << "Invalid usage: " << e.what() << std::endl;

                std::cout << desc << std::endl;
                return 1;
        }

        // Check for orthonormal P-functionals
        unsigned int orthonormal_count = 0;
        bool orthonormal;
        for (size_t p = 0; p < pfunctionals.size(); p++) {
                if (pfunctionals[p].functional == PFunctional::Hermite)
                        orthonormal_count++;
        }
        if (orthonormal_count == 0)
                orthonormal = false;
        else if (orthonormal_count == pfunctionals.size())
                orthonormal = true;
        else
                throw boost::program_options::validation_error(
                        boost::program_options::validation_error::invalid_option_value,
                        "Cannot mix regular and orthonormal P-functionals");

        // Configure logging
        if (vm.count("debug")) {
                logger.settings.threshold = trace;
                logger.settings.prefix_timestamp = true;
                logger.settings.prefix_level = true;
        } else if (vm.count("verbose"))
                logger.settings.threshold = debug;
        else if (vm.count("quiet"))
                logger.settings.threshold = warning;


        //
        // Image processing
        //
        
        // Check for CUDA devices
        int count;
        cudaGetDeviceCount(&count);
        clog(debug) << "Found " << count << " CUDA device(s)." << std::endl;
        if (count < 1) {
                std::cerr << "No CUDA-capable devices found." << std::endl;
                return 1;
        } else {
                for (int i = 0; i < count; i++) {
                        cudaDeviceProp prop;
                        CUDAHelper::checkError(cudaGetDeviceProperties(&prop, i));

                        clog(trace) << " --- General Information for device " << i << " --- " << std::endl;
                        clog(trace) << "     Name: " << prop.name << std::endl;
                        clog(trace) << "     Compute capability: " << prop.major << "." << prop.minor << std::endl;
                        clog(trace) << "     Clock rate: " << readable_frequency(prop.clockRate*1024) << std::endl;
                        clog(trace) << "     Device copy overlap: " << (prop.deviceOverlap ? "enabled" : "disabled") << std::endl;
                        clog(trace) << "     Kernel execution timeout: " << (prop.kernelExecTimeoutEnabled ? "enabled" : "disabled") << std::endl;

                        clog(trace) << " --- Memory Information for device " << i << " --- " << std::endl;
                        clog(trace) << "     Total global memory: " << readable_size(prop.totalGlobalMem) << std::endl;
                        clog(trace) << "     Total constant memory: " << readable_size(prop.totalConstMem) << std::endl;
                        clog(trace) << "     Total memory pitch: " << readable_size(prop.memPitch) << std::endl;
                        clog(trace) << "     Texture alignment: " << prop.textureAlignment << std::endl;

                        clog(trace) << " --- Multiprocessing Information for device " << i << " --- " << std::endl;
                        clog(trace) << "     Multiprocessor count: " << prop.multiProcessorCount << std::endl;
                        clog(trace) << "     Shared memory per processor: " << readable_size(prop.sharedMemPerBlock) << std::endl;
                        clog(trace) << "     Registers per processor: " << prop.regsPerBlock << std::endl;
                        clog(trace) << "     Threads in warp: " << prop.warpSize << std::endl;
                        clog(trace) << "     Maximum threads per block: " << prop.maxThreadsPerBlock << std::endl;
                        clog(trace) << "     Maximum thread dimensions: (" << prop.maxThreadsDim[0] << ", " << prop.maxThreadsDim[1] << ", " << prop.maxThreadsDim[2] << ")" << std::endl;
                        clog(trace) << "     Maximum grid dimensions: (" << prop.maxGridSize[0] << ", " << prop.maxGridSize[1] << ", " << prop.maxGridSize[2] << ")" << std::endl;
                }
        }

        // Read the image
        Eigen::MatrixXf input = gray2mat(readpgm(vm["input"].as<std::string>()));

        // Transform the image
        Transformer transformer(input, orthonormal);
        transformer.getTransform(tfunctionals, pfunctionals);

        clog(debug) << "Exiting" << std::endl;
        return 0;
}
