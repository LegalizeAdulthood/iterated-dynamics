find_package(fmt CONFIG REQUIRED)

add_executable(TestTextScreen WIN32 main.cpp)
target_link_libraries(TestTextScreen PUBLIC TextScreen fmt::fmt-header-only)
target_folder(TestTextScreen "Tests")
