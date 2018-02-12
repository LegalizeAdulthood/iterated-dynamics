# Copyright 2018 Richard Thomson

add_library(helpcom STATIC
    helpcom.cpp
    ../headers/helpcom.h)
target_include_directories(helpcom PUBLIC ../headers)
target_compile_definitions(helpcom PRIVATE ${OS_DEFINITIONS})

add_executable(hc
    hc.cpp
    ${OS_HC_FILES})
target_include_directories(hc PRIVATE ../headers)
if(${HAVE_OS_DEFINITIONS})
    target_compile_definitions(hc PRIVATE ${OS_DEFINITIONS})
endif()
if(WIN32)
    target_compile_definitions(hc PRIVATE _SCL_SECURE_NO_WARNINGS)
endif()
target_link_libraries(hc PRIVATE helpcom)
