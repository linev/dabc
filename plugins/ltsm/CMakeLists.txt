find_package(LTSM QUIET)

if(LTSM_FOUND)
   DABC_LINK_LIBRARY(DabcLtsm
                     LIBRARIES DabcBase ${LTSM_LIBRARY}
                     INCLUDES ${CMAKE_CURRENT_SOURCE_DIR} ${LTSM_INCLUDE_DIR})
endif()
