if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    return()
endif()

add_executable(id ${ID_EXECUTABLE_TYPE}
    main.cpp
)
target_link_libraries(id PUBLIC libid)
install(TARGETS id DESTINATION bin)
