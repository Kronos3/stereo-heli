####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding files
# MOD_DEPS: (optional) module dependencies
#
####

set(SOURCE_FILES
        "${CMAKE_CURRENT_LIST_DIR}/IntervalTimer.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/IntervalTimer.cpp"
        )


register_fprime_module()
