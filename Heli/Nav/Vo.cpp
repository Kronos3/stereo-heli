//
// Created by tumbar on 3/12/23.
//

#include "Vo.hpp"
#include <opencv2/features2d.hpp>
#include <opencv2/video/tracking.hpp>

namespace Vo
{
    struct SystemImpl
    {
        cv::Ptr<cv::Feature2D> features;
        Heli::Transform pose;

        cv::Size opticalFlowWindowSize;
        cv::TermCriteria term_criteria;

        cv::Mat last_left;
        cv::Mat last_right;

        explicit SystemImpl(const Heli::Transform &pose_)
                : features(cv::FastFeatureDetector::create()), pose(pose_),
                  opticalFlowWindowSize(31, 31),
                  term_criteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 20, 0.03)
        {
        }

        /**
         * Divide the image up into equally sized tiles.
         * Extract the top 10 features from each tile and
         * concatenate them into the keypoints vector
         * @param image full left image to extract keypoints for
         * @param keypoints output keypoints
         * @param tile_h tile height in px
         * @param tile_w tile width in px
         */
        void extractKeypoints(const cv::Mat &image,
                              std::vector<cv::KeyPoint> &keypoints,
                              I32 tile_h, I32 tile_w) const
        {
            for (I32 y = 0; y < image.rows; y += tile_h)
            {
                for (I32 x = 0; x < image.cols; x += tile_w)
                {
                    auto sub_image = image(cv::Rect(x, y, tile_w, tile_h));

                    std::vector<cv::KeyPoint> current_kps;
                    features->detect(sub_image, current_kps);

                    // Sort the keypoints in descending order
                    std::sort(current_kps.begin(), current_kps.end(),
                              [](const cv::KeyPoint &c1, const cv::KeyPoint &c2)
                              { return c1.response > c2.response; });

                    // Take the 10 best keypoints (if there exists 10)
                    for (U32 i = 0; i < 10 && i < current_kps.size(); i++)
                    {
                        auto &kp = current_kps.at(i);

                        // Adjust the sub image coordinates to adjust to the entire image
                        kp.pt += cv::Point2f(static_cast<F32>(x), static_cast<F32>(y));

                        keypoints.push_back(kp);
                    }
                }
            }
        }

        /**
         * Track keypoints from the previous to current image
         * @param prev i-1th image
         * @param curr i-th image
         * @param kp_prev_initial keypoints in the previous image
         * @param tracked_prev Points tracked from previous image
         * @param tracked_curr Points tracked in current image
         * @param maxError The maximum acceptable error
         */
        void trackKeypoints(const cv::Mat &prev, const cv::Mat &curr,
                            const std::vector<cv::Point2f> &kp_prev_initial,
                            std::vector<cv::Point2f> &tracked_prev,
                            std::vector<cv::Point2f> &tracked_curr,
                            float maxError = 4.0) const
        {
            std::vector<uchar> status;
            std::vector<F32> err;
            std::vector<cv::Point2f> kp_next_initial;
            cv::calcOpticalFlowPyrLK(prev, curr, kp_prev_initial, kp_next_initial, status, err,
                                     opticalFlowWindowSize, 3, term_criteria, 0, 0.001);

            // Grab only the points that were matched
            // Filter out bad points
            // Filter out points outside the image bounds
            for (U32 i = 0; i < kp_prev_initial.size(); i++)
            {
                if (status[i])
                {
                    // Check if this point is any good
                    if (err[i] < maxError)
                    {
                        // Place the tracking point directly on a pixel coordinate
                        auto realigned = cv::Point2f (std::round(kp_next_initial[i].x), std::round(kp_next_initial[i].y));

                        // Check if the point is in bound
                        if (realigned.x >= 0 && realigned.x < prev.cols
                            && realigned.y >= 0 && realigned.y < prev.rows)
                        {
                            tracked_prev.push_back(kp_prev_initial[i]);
                            tracked_curr.push_back(realigned);
                        }
                    }
                }
            }
        }

        /**
         * Feed in the next image frame and track since the last image
         * Uses optical flow model to compute camera transform from last
         * pose to current pose.
         * @param left_r left rectified image frame
         * @param right_r right rectified image frame
         */
        void trackStereo(const cv::Mat &left_r,
                         const cv::Mat &right_r)
        {
            // We need a sequence of images to perform tracking
            // Wait for the next frame
            if (last_left.size != left_r.size)
            {
                left_r.copyTo(last_left);
                right_r.copyTo(last_right);
                return;
            }

            // Find keypoints from the last frame
            // We can't use the tracked from the last image
            // because there will be new points in view as the camera moves
            std::vector<cv::KeyPoint> keypoints;
            extractKeypoints(last_left, keypoints, 10, 20);

            // Convert the keypoints to points
            std::vector<cv::Point2f> keypoints_points;
            for (const auto& kp : keypoints)
            {
                keypoints_points.push_back(kp.pt);
            }

            // Track the keypoints from the last image to the current frame
            std::vector<cv::Point2f> kp_1, kp_2;
            trackKeypoints(last_left, left_r, keypoints_points, kp_1, kp_2);

            // Compute the depth only on the keypoints

        }
    };

    System::System(const Heli::Transform &pose)
            : impl(new SystemImpl(pose))
    {
    }

    System::~System()
    {
        delete impl;
    }

    void System::trackStereo(const cv::Mat &left_r,
                             const cv::Mat &right_r)
    {
        impl->trackStereo(left_r, right_r);
    }

    const Heli::Transform &System::getPose() const
    {
        return impl->pose;
    }
}
