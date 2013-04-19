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
extern "C" {
        #include "functionals.h"
}
#include "wrapper.hpp"
#include "transform.hpp"
#include "cudahelper/errorhandling.hpp"


//
// Program option parsers
//

std::istream& operator>>(std::istream& in, TFunctional& tfunctional)
{
        std::string token;
        in >> token;
        if (token == "0") {
                tfunctional = TFunctional("Radon",
                                new SimpleFunctionalWrapper(TFunctionalRadon));
        } else if (token == "1") {
                tfunctional = TFunctional("T1",
                                new SimpleFunctionalWrapper(TFunctional1));
        } else if (token == "2") {
                tfunctional = TFunctional("T2",
                                new SimpleFunctionalWrapper(TFunctional2));
        } else if (token == "3") {
                tfunctional = TFunctional("T3",
                                new SimpleFunctionalWrapper(TFunctional3));
        } else if (token == "4") {
                tfunctional = TFunctional("T4",
                                new SimpleFunctionalWrapper(TFunctional4));
        } else if (token == "5") {
                tfunctional = TFunctional("T5",
                                new SimpleFunctionalWrapper(TFunctional5));
        } else {
                throw boost::program_options::validation_error(
                        boost::program_options::validation_error::invalid_option_value,
                        "Unknown T-functional");
        }
        return in;
}

std::istream& operator>>(std::istream& in, PFunctional& pfunctional)
{
        std::string token;
        in >> token;
        if (token == "1") {
                pfunctional = PFunctional("P1",
                                new SimpleFunctionalWrapper(PFunctional1));
        } else if (token == "2") {
                pfunctional = PFunctional("P2",
                                new SimpleFunctionalWrapper(PFunctional2));
        } else if (token == "3") {
                pfunctional = PFunctional("P3",
                                new SimpleFunctionalWrapper(PFunctional3));
        } else if (token[0] == 'H') {
                unsigned int order;
                if (token.size() < 2)
                        throw boost::program_options::validation_error(
                                boost::program_options::validation_error::invalid_option_value,
                                "Missing order parameter for Hermite P-functional");
                try {
                        order = boost::lexical_cast<unsigned int>(token.substr(1));
                }
                catch(boost::bad_lexical_cast &) {
                        throw boost::program_options::validation_error(
                                boost::program_options::validation_error::invalid_option_value,
                                "Unparseable order parameter for Hermite P-functional");
                }
                pfunctional = PFunctional(boost::str(boost::format("H%d") % order),
                                new GenericFunctionalWrapper<unsigned int, size_t>(PFunctionalHermite),
                                PFunctional::Type::HERMITE,
                                order);
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
        std::vector<TFunctional> tfunctionals;
        std::vector<PFunctional> pfunctionals;

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
                ("output,o",
                        boost::program_options::value<std::string>()
                        ->default_value("circus.dat"),
                        "where to write the output circus data")
                ("t-functional,T",
                        boost::program_options::value<std::vector<TFunctional>>(&tfunctionals)
                        ->required(),
                        "T-functionals")
                ("p-functional,P",
                        boost::program_options::value<std::vector<PFunctional>>(&pfunctionals),
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
        Eigen::MatrixXf input = gray2mat(pgmRead(vm["input"].as<std::string>()));

        // Transform the image
        Transformer transformer(input);
        Eigen::MatrixXf output = transformer.getTransform(tfunctionals, pfunctionals);

        // Save the output data
        if (pfunctionals.size() > 0) {          
                std::vector<std::string> headers;
                for (size_t tp = 0; tp < output.cols(); tp++) {
                        size_t t = tp / pfunctionals.size();
                        size_t p = tp % pfunctionals.size();
                        std::stringstream header;
                        header << tfunctionals[t].name << "-"
                                << pfunctionals[p].name;
                        headers.push_back(header.str());

                        if (clog(debug)) {
                                // Save individual traces as well
                                std::stringstream fn_trace_data;
                                fn_trace_data << "trace_" << header.str() << ".dat";
                                dataWrite(fn_trace_data.str(), (Eigen::MatrixXf) output.col(tp));
                        }
                }
                dataWrite(vm["output"].as<std::string>(), output, headers);
        }

        clog(debug) << "Exiting" << std::endl;
        return 0;
}
