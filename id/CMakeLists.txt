add_executable(id ${ID_EXECUTABLE_TYPE}
    main.cpp
)
target_link_libraries(id PUBLIC libid)
install(TARGETS id DESTINATION bin)
