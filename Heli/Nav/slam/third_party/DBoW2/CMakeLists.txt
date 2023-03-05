set(HDRS_DBOW2
        DBoW2/BowVector.h
        DBoW2/FORB.h
        DBoW2/FClass.h
        DBoW2/FeatureVector.h
        DBoW2/ScoringObject.h
        DBoW2/TemplatedVocabulary.h)
set(SRCS_DBOW2
        DBoW2/BowVector.cpp
        DBoW2/FORB.cpp
        DBoW2/FeatureVector.cpp
        DBoW2/ScoringObject.cpp)

set(HDRS_DUTILS
        DUtils/Random.h
        DUtils/Timestamp.h)
set(SRCS_DUTILS
        DUtils/Random.cpp
        DUtils/Timestamp.cpp)

add_library(DBoW2 ${SRCS_DBOW2} ${SRCS_DUTILS})
