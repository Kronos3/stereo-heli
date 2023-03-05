/**
* This file is part of ORB-SLAM2.
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

#include "KeyFrameDatabase.h"

#include "KeyFrame.h"
#include "DBoW2/DBoW2/BowVector.h"

#include <mutex>

namespace ORB_SLAM2
{
    KeyFrameDatabase::KeyFrameDatabase(const ORBVocabulary &voc) :
            mpVoc(&voc)
    {
        mvInvertedFile.resize(voc.size());
    }


    void KeyFrameDatabase::add(KeyFrame* pKF)
    {
        std::unique_lock<std::mutex> lock(mMutex);

        for (const auto &vit: pKF->mBowVec)
            mvInvertedFile[vit.first].push_back(pKF);
    }

    void KeyFrameDatabase::erase(KeyFrame* pKF)
    {
        std::unique_lock<std::mutex> lock(mMutex);

        // Erase elements in the Inverse File for the entry
        for (const auto &vit: pKF->mBowVec)
        {
            // List of keyframes that share the word
            std::list<KeyFrame*> &lKFs = mvInvertedFile[vit.first];

            for (auto lit = lKFs.begin(), lend = lKFs.end(); lit != lend; lit++)
            {
                if (pKF == *lit)
                {
                    lKFs.erase(lit);
                    break;
                }
            }
        }
    }

    void KeyFrameDatabase::clear()
    {
        mvInvertedFile.clear();
        mvInvertedFile.resize(mpVoc->size());
    }

    std::vector<KeyFrame*> KeyFrameDatabase::DetectLoopCandidates(KeyFrame* pKF, float minScore)
    {
        std::set<KeyFrame*> spConnectedKeyFrames = pKF->GetConnectedKeyFrames();
        std::list<KeyFrame*> lKFsSharingWords;

        // Search all keyframes that share a word with current keyframes
        // Discard keyframes connected to the query keyframe
        {
            std::unique_lock<std::mutex> lock(mMutex);

            for (auto vit = pKF->mBowVec.begin(), vend = pKF->mBowVec.end();
                 vit != vend; vit++)
            {
                std::list<KeyFrame*> &lKFs = mvInvertedFile[vit->first];

                for (auto pKFi: lKFs)
                {
                    if (pKFi->mnLoopQuery != pKF->mnId)
                    {
                        pKFi->mnLoopWords = 0;
                        if (!spConnectedKeyFrames.count(pKFi))
                        {
                            pKFi->mnLoopQuery = pKF->mnId;
                            lKFsSharingWords.push_back(pKFi);
                        }
                    }
                    pKFi->mnLoopWords++;
                }
            }
        }

        if (lKFsSharingWords.empty())
            return {};

        std::list<std::pair<float, KeyFrame*> > lScoreAndMatch;

        // Only compare against those keyframes that share enough words
        int maxCommonWords = 0;
        for (auto &lKFsSharingWord: lKFsSharingWords)
        {
            if (lKFsSharingWord->mnLoopWords > maxCommonWords)
                maxCommonWords = lKFsSharingWord->mnLoopWords;
        }

        int minCommonWords = maxCommonWords * 0.8f;

        int nscores = 0;

        // Compute similarity score. Retain the matches whose score is higher than minScore
        for (auto pKFi: lKFsSharingWords)
        {
            if (pKFi->mnLoopWords > minCommonWords)
            {
                nscores++;

                float si = mpVoc->score(pKF->mBowVec, pKFi->mBowVec);

                pKFi->mLoopScore = si;
                if (si >= minScore)
                    lScoreAndMatch.emplace_back(si, pKFi);
            }
        }

        if (lScoreAndMatch.empty())
            return {};

        std::list<std::pair<float, KeyFrame*> > lAccScoreAndMatch;
        float bestAccScore = minScore;

        // Lets now accumulate score by covisibility
        for (auto &it: lScoreAndMatch)
        {
            KeyFrame* pKFi = it.second;
            std::vector<KeyFrame*> vpNeighs = pKFi->GetBestCovisibilityKeyFrames(10);

            float bestScore = it.first;
            float accScore = it.first;
            KeyFrame* pBestKF = pKFi;
            for (auto pKF2: vpNeighs)
            {
                if (pKF2->mnLoopQuery == pKF->mnId && pKF2->mnLoopWords > minCommonWords)
                {
                    accScore += pKF2->mLoopScore;
                    if (pKF2->mLoopScore > bestScore)
                    {
                        pBestKF = pKF2;
                        bestScore = pKF2->mLoopScore;
                    }
                }
            }

            lAccScoreAndMatch.emplace_back(accScore, pBestKF);
            if (accScore > bestAccScore)
                bestAccScore = accScore;
        }

        // Return all those keyframes with a score higher than 0.75*bestScore
        float minScoreToRetain = 0.75f * bestAccScore;

        std::set<KeyFrame*> spAlreadyAddedKF;
        std::vector<KeyFrame*> vpLoopCandidates;
        vpLoopCandidates.reserve(lAccScoreAndMatch.size());

        for (auto &it: lAccScoreAndMatch)
        {
            if (it.first > minScoreToRetain)
            {
                KeyFrame* pKFi = it.second;
                if (!spAlreadyAddedKF.count(pKFi))
                {
                    vpLoopCandidates.push_back(pKFi);
                    spAlreadyAddedKF.insert(pKFi);
                }
            }
        }


        return vpLoopCandidates;
    }

    std::vector<KeyFrame*> KeyFrameDatabase::DetectRelocalizationCandidates(Frame* F)
    {
        std::list<KeyFrame*> lKFsSharingWords;

        // Search all keyframes that share a word with current frame
        {
            std::unique_lock<std::mutex> lock(mMutex);

            for (auto vit = F->mBowVec.begin(), vend = F->mBowVec.end(); vit != vend; vit++)
            {
                std::list<KeyFrame*> &lKFs = mvInvertedFile[vit->first];

                for (auto pKFi: lKFs)
                {
                    if (pKFi->mnRelocQuery != F->mnId)
                    {
                        pKFi->mnRelocWords = 0;
                        pKFi->mnRelocQuery = F->mnId;
                        lKFsSharingWords.push_back(pKFi);
                    }
                    pKFi->mnRelocWords++;
                }
            }
        }
        if (lKFsSharingWords.empty())
            return {};

        // Only compare against those keyframes that share enough words
        int maxCommonWords = 0;
        for (auto &lKFsSharingWord: lKFsSharingWords)
        {
            if (lKFsSharingWord->mnRelocWords > maxCommonWords)
                maxCommonWords = lKFsSharingWord->mnRelocWords;
        }

        int minCommonWords = maxCommonWords * 0.8f;

        std::list<std::pair<float, KeyFrame*> > lScoreAndMatch;

        int nscores = 0;

        // Compute similarity score.
        for (auto pKFi: lKFsSharingWords)
        {
            if (pKFi->mnRelocWords > minCommonWords)
            {
                nscores++;
                float si = mpVoc->score(F->mBowVec, pKFi->mBowVec);
                pKFi->mRelocScore = si;
                lScoreAndMatch.emplace_back(si, pKFi);
            }
        }

        if (lScoreAndMatch.empty())
            return {};

        std::list<std::pair<float, KeyFrame*>> lAccScoreAndMatch;
        float bestAccScore = 0;

        // Lets now accumulate score by covisibility
        for (auto &it: lScoreAndMatch)
        {
            KeyFrame* pKFi = it.second;
            std::vector<KeyFrame*> vpNeighs = pKFi->GetBestCovisibilityKeyFrames(10);

            float bestScore = it.first;
            float accScore = bestScore;
            KeyFrame* pBestKF = pKFi;
            for (auto pKF2: vpNeighs)
            {
                if (pKF2->mnRelocQuery != F->mnId)
                    continue;

                accScore += pKF2->mRelocScore;
                if (pKF2->mRelocScore > bestScore)
                {
                    pBestKF = pKF2;
                    bestScore = pKF2->mRelocScore;
                }
            }

            lAccScoreAndMatch.emplace_back(accScore, pBestKF);
            if (accScore > bestAccScore)
                bestAccScore = accScore;
        }

        // Return all those keyframes with a score higher than 0.75*bestScore
        float minScoreToRetain = 0.75f * bestAccScore;
        std::set<KeyFrame*> spAlreadyAddedKF;
        std::vector<KeyFrame*> vpRelocCandidates;
        vpRelocCandidates.reserve(lAccScoreAndMatch.size());
        for (auto &it: lAccScoreAndMatch)
        {
            const float &si = it.first;
            if (si > minScoreToRetain)
            {
                KeyFrame* pKFi = it.second;
                if (!spAlreadyAddedKF.count(pKFi))
                {
                    vpRelocCandidates.push_back(pKFi);
                    spAlreadyAddedKF.insert(pKFi);
                }
            }
        }

        return vpRelocCandidates;
    }

} //namespace ORB_SLAM
