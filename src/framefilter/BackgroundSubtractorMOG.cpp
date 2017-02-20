//******************************************************************************
//* File:   BackgroundSubtractorMOG.cpp
//* Author: Jon Newman <jpnewman snail mit dot edu>
//*
//* Copyright (c) Jon Newman (jpnewman snail mit dot edu)
//* All right reserved.
//* This file is part of the Oat project.
//* This is free software: you can redistribute it and/or modify
//* it under the terms of the GNU General Public License as published by
//* the Free Software Foundation, either version 3 of the License, or
//* (at your option) any later version.
//* This software is distributed in the hope that it will be useful,
//* but WITHOUT ANY WARRANTY; without even the implied warranty of
//* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//* GNU General Public License for more details.
//* You should have received a copy of the GNU General Public License
//* along with this source code.  If not, see <http://www.gnu.org/licenses/>.
//******************************************************************************

#include "BackgroundSubtractorMOG.h"

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/cvconfig.h>
#include <opencv2/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <stdexcept>
#include <string>
#ifdef HAVE_CUDA
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudabgsegm.hpp>
#endif
#include <cpptoml.h>

#include "../../lib/utility/IOFormat.h"
#include "../../lib/utility/TOMLSanitize.h"

namespace oat {

BackgroundSubtractorMOG::BackgroundSubtractorMOG(
        const std::string &frame_source_address,
        const std::string &frame_sink_address)
: FrameFilter(frame_source_address, frame_sink_address)
{
    // Nothing
}

po::options_description BackgroundSubtractorMOG::options() const
{
    // Update CLI options
    po::options_description local_opts;
    local_opts.add_options()
        ("adaptation-coeff,a", po::value<double>(),
         "Value, 0 to 1.0, specifying how quickly the statistical model "
         "of the background image should be updated. "
         "Default is 0, specifying no adaptation.")
#ifdef HAVE_CUDA
        ("gpu-index", po::value<size_t>(),
         "Index of GPU card to use for performing MOG segmentation.")
#endif
        ;

    return local_opts;
}

void BackgroundSubtractorMOG::applyConfiguration(const po::variables_map &vm,
                                              const config::OptionTable &config_table)
{
#ifdef HAVE_CUDA
    // GPU index
    size_t index = 0;
    oat::config::getNumericValue<size_t>(
        vm, config_table, "gpu-index", index, 0);
    cv::cuda::GpuMat gm; // Create context. This can take an extremely long time
    configureGPU(index);
    background_subtractor_
        = cv::cuda::createBackgroundSubtractorMOG(/*TODO: defaults OK?*/);
#else
    background_subtractor_
        = cv::createBackgroundSubtractorMOG2(/*TODO:defaults OK?*/);
#endif

    // Learning coefficient
    oat::config::getNumericValue(
        vm, config_table, "adaptation-coeff", learning_coeff_, 0.0, 1.0);
}

#ifdef HAVE_CUDA
void BackgroundSubtractorMOG::configureGPU(size_t index)
{
    // Determine if a compatible device is available
    size_t num_devices = cv::cuda::getCudaEnabledDeviceCount();
    if (num_devices < 1)
        throw (std::runtime_error("No GPU found or OpenCV was compiled without CUDA support."));

    if (index > num_devices)
        throw (std::runtime_error("Selected GPU index is invalid."));

    cv::cuda::DeviceInfo gpu_info(index);
    if (!gpu_info.isCompatible())
        throw (std::runtime_error("Selected GPU is not compatible with OpenCV."));

    cv::cuda::setDevice(index);

#ifndef NDEBUG
    cv::cuda::printShortCudaDeviceInfo(index);
#endif
}
#endif

void BackgroundSubtractorMOG::filter(cv::Mat &frame)
{
#ifdef HAVE_CUDA
    current_frame_.upload(frame);
    background_subtractor_->apply(current_frame_, background_mask_, learning_coeff_);
    //TODO: Add hard mask operation here to increase performance
    cv::cuda::bitwise_not(background_mask_, background_mask_);
    current_frame_.setTo(0, background_mask_);
    current_frame_.download(frame);
#else
    background_subtractor_->apply(frame, background_mask_, learning_coeff_);
    frame.setTo(0, background_mask_ == 0);
#endif
}

} /* namespace oat */
