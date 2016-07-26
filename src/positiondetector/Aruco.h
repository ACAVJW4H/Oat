//****************************************************************************** * File:   Aruco.h
//* Author: Jon Newman <jpnewman snail mit dot edu>
//
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
//****************************************************************************

#ifndef OAT_ARUCO_H
#define	OAT_ARUCO_H

#include <string>
#include <opencv2/aruco.hpp>
#include <opencv2/core/mat.hpp>

#include "PositionDetector.h"

namespace oat {

// Forward decl.
class Position2D;

enum class HeadingDirection {
    NW = 0,
    NE,
    SE,
    SW
};

/**
 * Aruco marker tracking.
 */
class Aruco : public PositionDetector {
public:

    /**
     * Aruco marker tracking.
     * @param frame_source_address Frame SOURCE node address
     * @param position_sink_address Position SINK node address
     */
    Aruco(const std::string &frame_source_address,
          const std::string &position_sink_address);

    /**
     * Perform aruco marker code detection
     * @param frame frame to look for object in.
     * @return detected marker position (upper left hand corner)
     */
    void detectPosition(cv::Mat &frame, oat::Position2D &position) override;

    void configure(const std::string &config_file,
                   const std::string &config_key) override;

private:

    /// Marker ID to look for
    int marker_id_ {1};
    cv::Ptr<cv::aruco::Dictionary> marker_dict_;
    cv::Ptr<cv::aruco::DetectorParameters> detection_params_;

    /// TODO: Option for Marker dictionary to use
    cv::aruco::PREDEFINED_DICTIONARY_NAME 
        marker_dict_id_ {cv::aruco::DICT_6X6_250};

    /// TODO: Option for Heading direction
    oat::HeadingDirection 
        heading_dir_ {oat::HeadingDirection::NW};

    // TODO: Add dectection parameters
    
    // TODO: helper mode to print markers
};

}       /* namespace oat */
#endif	/* OAT_ARUCO_H */