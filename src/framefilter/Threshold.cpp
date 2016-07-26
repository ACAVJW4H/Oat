//******************************************************************************
//* File:   Threshold.cpp
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

#include "OatConfig.h" // Generated by CMake

#include <string>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <cpptoml.h>
#include "../../lib/utility/TOMLSanitize.h"
#include "../../lib/utility/IOFormat.h"

#include "Threshold.h"

namespace oat {

Threshold::Threshold(const std::string &frame_source_address,
                     const std::string &frame_sink_address) :
  FrameFilter(frame_source_address, frame_sink_address)
{
    // Nothing
}

// TODO: Tuning
void Threshold::configure(const std::string& config_file,
                             const std::string& config_key) {

    // Available options
    std::vector<std::string> options {"min-intensity",
                                      "max-intensity"};

    // This will throw cpptoml::parse_exception if a file
    // with invalid TOML is provided
   auto config = cpptoml::parse_file(config_file);

    // See if a camera configuration was provided
    if (config->contains(config_key)) {

        // Get this components configuration table
        auto this_config = config->get_table(config_key);

        // Check for unknown options in the table and throw if you find them
        oat::config::checkKeys(options, this_config);

        // Intensity min, max
        {
            int64_t min, max;
            oat::config::getValue(this_config, "min-intensity", min, 256);
            min_intensity_bound_ = min;

            oat::config::getValue(this_config, "max-intensity", max, 256);
            max_intensity_bound_ = max;
        }

    } else {
        throw (std::runtime_error(oat::configNoTableError(config_key, config_file)));
    }

}

void Threshold::filter(cv::Mat& frame) {

    cv::Mat frame_copy = frame.clone();

    if (frame.type() != CV_8UC1)
        cv::cvtColor(frame, frame_copy, cv::COLOR_BGR2GRAY);

    cv::inRange(frame_copy, 
                min_intensity_bound_, 
                max_intensity_bound_, 
                threshold_frame_);

    std::vector<std::vector <cv::Point> > contours;

    cv::findContours(threshold_frame_, 
                     contours, 
                     cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);

    cv::drawContours(frame, contours, -1, cv::Scalar(255, 255, 255));
    //frame.setTo(cv::Scalar(0, 0, 0), threshold_frame_ == 0);
}

} /* namespace oat */

