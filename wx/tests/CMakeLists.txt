find_package(fmt CONFIG REQUIRED)

add_executable(TestTextScreen WIN32 test_TextScreen.cpp)
target_link_libraries(TestTextScreen PUBLIC TextScreen fmt::fmt-header-only)
target_folder(TestTextScreen "Tests/wx")

add_executable(TestPlot WIN32 test_Plot.cpp)
target_link_libraries(TestPlot PUBLIC Plot)
target_folder(TestPlot "Tests/wx")
