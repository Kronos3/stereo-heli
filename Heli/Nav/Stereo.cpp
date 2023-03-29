//
// Created by tumbar on 3/14/23.
//

#include "Stereo.hpp"
#include "Assert.hpp"

#include <Heli/parallel/parallel.hpp>

namespace Vo
{
    enum
    {
        STEREO_JOBS = 4,
        STEREO_MIN_JOB_POINTS = 8,
    };

    struct JobArgs
    {
        const cv::Mat &left;
        const cv::Mat &right;
        const cv::Mat &left_kps;
        cv::Mat &right_kps;

        JobArgs(
                const cv::Mat &left_,
                const cv::Mat &right_,
                const cv::Mat &left_kps_,
                cv::Mat &right_kps_
        ) : left(left_), right(right_), left_kps(left_kps_), right_kps(right_kps_) {}
    };

    struct StereoMatcherImpl
    {
        libparallel::Parallelize<STEREO_JOBS, JobArgs> executor;

        U8 windowSize;
        U16 textureThreshold;
        I16 min_disp;
        I16 max_disp;

        StereoMatcherImpl()
                : executor([this](JobArgs args)
                           { compute(args); }),
                  windowSize(11), textureThreshold(20),
                  min_disp(5), max_disp(800)
        {
        }

        inline void compute(JobArgs args) const
        {
            // TODO(tumbar) Offload buffer initialization
            U16 sad_buffer_data[windowSize][args.left.cols];

            cv::Mat sad_buffer(windowSize, args.left.cols, CV_16U);

            for (I32 i = 0; i < args.left_kps.rows; i++)
            {
                // Clear the buffer by setting everything to highest possible value
                // This is used so
                std::memset(sad_buffer_data, 0xFF, sizeof(sad_buffer_data));

                // Get the point of interest
                int xpoi = args.left_kps.at<I16>(i, 0);
                int ypoi = args.left_kps.at<I16>(i, 1);

                // Generate range that we should perform SAD over
                int x0 = xpoi + min_disp - windowSize;
                int x1 = xpoi + max_disp + windowSize;

                int y0 = ypoi - windowSize;
                int y1 = ypoi + windowSize;

                // Bound to the edges of the image
                // TODO(tumbar) Skip this point if its an edge point?
                x0 = (x0 < 0) ? 0 : x0;
                x1 = (x1 >= args.left.cols) ? args.left.cols - 1 : x1;
                y0 = (y0 < 0) ? 0 : y0;
                y1 = (y0 >= args.left.rows) ? args.right.rows - 1 : y1;

                // Fill the buffer with absolute differences between left and right ranges
                compute_ad(args.left, args.right, sad_buffer,
                           x0, y0, x1, y1);

                // Find the best match coordinate
//                int best_x;

            }
        }

        static inline void
        compute_ad(const cv::Mat &left,
                   const cv::Mat &right,
                   cv::Mat &sad_buffer,
                   int x0, int y0,
                   int x1, int y1)
        {
            for (int i = y0; i < y1; i++)
            {
                U16* d = reinterpret_cast<U16*>(sad_buffer.row(i - y0).data);
                U8* l = left.row(i).data + x0;
                U8* r = right.row(i).data + x0;
                for (int j = x0; j < x1; d++, l++, r++, j++)
                {
                    *d = std::abs(*l - *r);
                }
            }
        }
    };

    StereoMatcher::StereoMatcher()
            : impl(new StereoMatcherImpl)
    {
    }

    StereoMatcher::~StereoMatcher()
    {
        delete impl;
    }

    void StereoMatcher::compute(const cv::Mat &left,
                                const cv::Mat &right,
                                const cv::Mat &left_kps,
                                cv::Mat &right_kps)
    {
        FW_ASSERT(left_kps.cols == 2);

        right_kps = cv::Mat::zeros(left_kps.size(), CV_16U);

        U32 points_per_thread = left_kps.rows / STEREO_JOBS;
        if (points_per_thread >= STEREO_MIN_JOB_POINTS)
        {
            libparallel::Awaitable* awaiters[STEREO_JOBS];
            U32 t = 0;
            for (auto& awaiter : awaiters)
            {
                awaiter = &impl->executor.feed(JobArgs(left, right, left_kps, right_kps), t++);
            }

            // Wait for the jobs to finish up
            for (auto& awaiter : awaiters)
            {
                awaiter->await();
            }
        }
        else
        {
            // Run on the job on the main thread since there
            // are not enough points to justify running on separate threads
            impl->compute(JobArgs(left, right, left_kps, right_kps));
        }
    }

    void StereoMatcher::setWindowSize(U8 windowSize)
    {
        impl->windowSize = windowSize;
    }

    void StereoMatcher::setTextureThreshold(U16 textureThreshold)
    {
        impl->textureThreshold = textureThreshold;
    }
}
