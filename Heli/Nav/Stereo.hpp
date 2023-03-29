//
// Created by tumbar on 3/14/23.
//

#ifndef STEREO_HELI_STEREO_HPP
#define STEREO_HELI_STEREO_HPP

#include <Fw/Types/BasicTypes.hpp>
#include "opencv2/core/mat.hpp"

namespace Vo
{
    struct StereoMatcherImpl;
    class StereoMatcher
    {
    public:

        StereoMatcher();
        ~StereoMatcher();

        void compute(const cv::Mat& left,
                     const cv::Mat& right,
                     const cv::Mat& left_kps,
                     cv::Mat& right_kps);

        void setWindowSize(U8 windowSize);
        void setTextureThreshold(U16 textureThreshold);

    PRIVATE:

        StereoMatcherImpl* impl;
    };
}

#endif //STEREO_HELI_STEREO_HPP
