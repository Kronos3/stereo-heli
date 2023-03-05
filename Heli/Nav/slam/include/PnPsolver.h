/**
* This file is part of ORB-SLAM2.
* This file is a modified version of EPnP <http://cvlab.epfl.ch/EPnP/index.php>, see FreeBSD license below.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/

/**
* Copyright (c) 2009, V. Lepetit, EPFL
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* The views and conclusions contained in the software and documentation are those
* of the authors and should not be interpreted as representing official policies,
*   either expressed or implied, of the FreeBSD Project
*/

#ifndef PNPSOLVER_H
#define PNPSOLVER_H

#include <opencv2/core/core.hpp>
#include "MapPoint.h"
#include "Frame.h"

namespace ORB_SLAM2
{

    class PnPsolver
    {
    public:
        PnPsolver(const Frame &F, const std::vector<MapPoint*> &vpMapPointMatches);

        ~PnPsolver();

        void SetRansacParameters(double probability = 0.99, int minInliers = 8, int maxIterations = 300, int minSet = 4,
                                 float epsilon = 0.4,
                                 float th2 = 5.991);

        cv::Mat find(std::vector<bool> &vbInliers, int &nInliers);

        cv::Mat iterate(int nIterations, bool &bNoMore, std::vector<bool> &vbInliers, int &nInliers);

    private:

        void CheckInliers();

        bool Refine();

        // Functions from the original EPnP code
        void set_maximum_number_of_correspondences(int n);

        void reset_correspondences();

        void add_correspondence(double X, double Y, double Z,
                                double u, double v);

        double compute_pose(double R[3][3], double T[3]);

        static void relative_error(double &rot_err, double &transl_err,
                                   const double Rtrue[3][3], const double ttrue[3],
                                   const double Rest[3][3], const double test[3]);

        void print_pose(const double R[3][3], const double t[3]);

        double reprojection_error(const double R[3][3], const double t[3]);

        void choose_control_points();

        void compute_barycentric_coordinates();

        void fill_M(cv::Mat &M, int row, const double* alphas, double u, double v) const;

        void compute_ccs(const double* betas, const double* ut);

        void compute_pcs();

        void solve_for_sign();

        static void find_betas_approx_1(const cv::Mat &L_6x10, const cv::Mat &Rho, double* betas);

        static void find_betas_approx_2(const cv::Mat &L_6x10, const cv::Mat &Rho, double* betas);

        static void find_betas_approx_3(const cv::Mat &L_6x10, const cv::Mat &Rho, double* betas);

        static void qr_solve(cv::Mat &A, cv::Mat &b, cv::Mat &X);

        static double dot(const double* v1, const double* v2);

        static double dist2(const double* p1, const double* p2);

        void compute_rho(double* rho);

        static void compute_L_6x10(const double* ut, double* l_6x10);

        static void gauss_newton(const cv::Mat &L_6x10, const cv::Mat &Rho, double current_betas[4]);

        static void compute_A_and_b_gauss_newton(const cv::Mat &l_6x10, const cv::Mat &rho,
                                                 const double cb[4], cv::Mat &A, cv::Mat &b);

        double compute_R_and_t(const double* ut, const double* betas,
                               double R[3][3], double t[3]);

        void estimate_R_and_t(double R[3][3], double t[3]);

        static void copy_R_and_t(const double R_dst[3][3], const double t_dst[3],
                                 double R_src[3][3], double t_src[3]);

        static void mat_to_quat(const double R[3][3], double q[4]);


        double uc, vc, fu, fv;

        double* pws, * us, * alphas, * pcs;
        int maximum_number_of_correspondences;
        int number_of_correspondences;

        double cws[4][3], ccs[4][3];
        double cws_determinant;

        std::vector<MapPoint*> mvpMapPointMatches;

        // 2D Points
        std::vector<cv::Point2f> mvP2D;
        std::vector<float> mvSigma2;

        // 3D Points
        std::vector<cv::Point3f> mvP3Dw;

        // Index in Frame
        std::vector<size_t> mvKeyPointIndices;

        // Current Estimation
        double mRi[3][3];
        double mti[3];
        cv::Mat mTcwi;
        std::vector<bool> mvbInliersi;
        int mnInliersi;

        // Current Ransac State
        int mnIterations;
        std::vector<bool> mvbBestInliers;
        int mnBestInliers;
        cv::Mat mBestTcw;

        // Refined
        cv::Mat mRefinedTcw;
        std::vector<bool> mvbRefinedInliers;
        int mnRefinedInliers;

        // Number of Correspondences
        int N;

        // Indices for random selection [0 .. N-1]
        std::vector<size_t> mvAllIndices;

        // RANSAC probability
        double mRansacProb;

        // RANSAC min inliers
        int mRansacMinInliers;

        // RANSAC max iterations
        int mRansacMaxIts;

        // RANSAC expected inliers/total ratio
        float mRansacEpsilon;

        // RANSAC Threshold inlier/outlier. Max error e = dist(P1,T_12*P2)^2
        float mRansacTh;

        // RANSAC Minimun Set used at each iteration
        int mRansacMinSet;

        // Max square error associated with scale level. Max error = th*th*sigma(level)*sigma(level)
        std::vector<float> mvMaxError;

    };

} //namespace ORB_SLAM

#endif //PNPSOLVER_H
