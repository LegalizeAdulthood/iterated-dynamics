# Copyright 2018 Richard Thomson

add_library(helpcom STATIC
    helpcom.cpp
    include/helpcom.h)
target_compile_definitions(helpcom PUBLIC ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_include_directories(helpcom PUBLIC include)
target_link_libraries(helpcom PUBLIC config)
set_target_properties(helpcom PROPERTIES FOLDER Libraries)
