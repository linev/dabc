DABC_LINK_LIBRARY(DabcAqua
                    LIBRARIES DabcBase DabcMbs
                    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(aquatest aquatest.c)
