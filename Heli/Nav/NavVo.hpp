//
// Created by tumbar on 3/12/23.
//

#ifndef STEREO_HELI_NAVVO_HPP
#define STEREO_HELI_NAVVO_HPP

#include "opencv2/core/mat.hpp"

namespace Heli
{
    struct VoSystemImpl;
    struct VoSystem
    {
        VoSystemImpl* impl;

        VoSystem();
        ~VoSystem();

        /**
         * Process the given stereo pair frame.
         * Images must be synchronized and rectified.
         * @param imLeft RGB (CV_8UC3) or grayscale (CV_8U). RGB is converted to grayscale
         * @param imRight RGB (CV_8UC3) or grayscale (CV_8U). RGB is converted to grayscale
         * @return
         */
        void TrackStereo(
                const cv::Mat &left_r,
                const cv::Mat &right_r
        );

        /**
         * Get the current camera position
         * @return Camera position (3x1 matrix)
         */
        const cv::Mat& getPosition() const;

        /**
         * Get the current orientation of the camera
         * (Quaternion)
         * @return Camera quaternion (4x1 matrix)
         */
        const cv::Mat& getOrientation() const;

        void setPose();
    };

}

#endif //STEREO_HELI_NAVVO_HPP
