//******************************************************************************
//* File:   FileReader.cpp
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

#include <thread>

#include <cpptoml.h>
#include "../../lib/utility/TOMLSanitize.h"
#include "../../lib/utility/IOFormat.h"

#include "FileReader.h"

namespace oat {

FileReader::FileReader(const std::string &sink_address) :
  FrameServer(sink_address)
{
    config_keys_ = {"video-file",
                    "fps",
                    "roi"};

    // Initialize time
    tick_ = clock_.now();
}

void FileReader::appendOptions(po::options_description &opts) const {

    // Accepts default options
    FrameServer::appendOptions(opts);

    // Update CLI options
    opts.add_options()
        ("video-file,f", po::value<std::string>(),
         "Path to video file to serve frames from.")
        ("fps,r", po::value<double>(),
         "Frames to serve per second.")
        ("roi {CF}", po::value<std::string>(),
         "Four element array of ints, [x0 y0 width height],"
         "defining a rectangular region of interest. Origin"
         "is upper left corner. ROI must fit within acquired"
         "frame size.")
        ;
}

void FileReader::configure(const po::variables_map &vm) {

    // Check for config file and entry correctness
    auto config_table = oat::config::getConfigTable(vm);
    oat::config::checkKeys(config_keys_, config_table);

    // Video file
    std::string file_name;
    oat::config::getValue(vm, config_table, "video-file", file_name, true);
    file_reader_.open(file_name);

    // Frame rate
    if (oat::config::getValue(vm, config_table, "fps", frames_per_second_, 0.0)) 
        calculateFramePeriod();

    // ROI
    oat::config::Array roi;
    if (oat::config::getArray(config_table, "roi", roi, 4, false)) {

        use_roi_ = true;
        auto roi_vec = roi->array_of<double>();

        region_of_interest_.x = roi_vec[0]->get();
        region_of_interest_.y = roi_vec[1]->get();
        region_of_interest_.width = roi_vec[2]->get();
        region_of_interest_.height = roi_vec[3]->get();
    }
}

void FileReader::connectToNode() {

    cv::Mat example_frame;
    file_reader_ >> example_frame;

    if (use_roi_)
        example_frame = example_frame(region_of_interest_);

    frame_sink_.bind(frame_sink_address_,
            example_frame.total() * example_frame.elemSize());

    shared_frame_ = frame_sink_.retrieve(
            example_frame.rows, example_frame.cols, example_frame.type());

    // Reset the video to the start
    file_reader_.set(cv::CAP_PROP_POS_AVI_RATIO, 0);

    // Put the sample rate in the shared frame
    shared_frame_.sample().set_rate_hz(1.0 / frame_period_in_sec_.count());
}

bool FileReader::process() {

    // START CRITICAL SECTION //
    ////////////////////////////

    // Wait for sources to read
    frame_sink_.wait();

    // Crop if necessary
    if (!use_roi_) {
            
        file_reader_ >> shared_frame_;
        frame_empty_ = shared_frame_.empty();

    } else {

        oat::Frame to_crop;
        file_reader_ >> to_crop;
        if (!(frame_empty_ = to_crop.empty()))
            to_crop = to_crop(region_of_interest_);
        to_crop.copyTo(shared_frame_);
    }

    // Increment sample count
    shared_frame_.sample().incrementCount();

    // Tell sources there is new data
    frame_sink_.post();

    ////////////////////////////
    //  END CRITICAL SECTION  //

    std::this_thread::sleep_for(frame_period_in_sec_ - (clock_.now() - tick_));
    tick_ = clock_.now();

    return frame_empty_;
}

void FileReader::calculateFramePeriod() {

    std::chrono::duration<double> frame_period {1.0 / frames_per_second_};

    // Automatic conversion
    frame_period_in_sec_ = frame_period;
}

} /* namespace oat */
