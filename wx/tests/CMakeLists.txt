add_executable(TestTextScreen WIN32 main.cpp)
target_link_libraries(TestTextScreen PUBLIC TextScreen)
target_folder(TestTextScreen "Tests")
