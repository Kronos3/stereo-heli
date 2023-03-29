//
// Created by tumbar on 3/12/23.
//

#ifndef STEREO_HELI_VO_HPP
#define STEREO_HELI_VO_HPP

#include "Heli/Fm/Transform.hpp"

namespace Vo
{
    struct SystemImpl;
    struct System
    {
        SystemImpl* impl;

        explicit System(const Heli::Transform &pose);
        ~System();

        /**
         * Process the given stereo pair frame.
         * Images must be synchronized and rectified.
         * @param imLeft RGB (CV_8UC3) or grayscale (CV_8U). RGB is converted to grayscale
         * @param imRight RGB (CV_8UC3) or grayscale (CV_8U). RGB is converted to grayscale
         */
        void trackStereo(
                const cv::Mat &left_r,
                const cv::Mat &right_r
        );

        /**
         * Get the current pose of the left camera
         * This pose is tracked from the initial pose passed into the system
         * Usually this will be the current site frame
         * @return Pose with respect to the site frame pose
         */
        const Heli::Transform& getPose() const;
    };

}

#endif //STEREO_HELI_VO_HPP
