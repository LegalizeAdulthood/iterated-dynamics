find_package(fmt CONFIG REQUIRED)

add_executable(TestTextScreen test_TextScreen.cpp)
target_link_libraries(TestTextScreen PUBLIC TextScreen fmt::fmt-header-only)
gui_executable(TestTextScreen)
target_folder(TestTextScreen "Tests/wx")

add_executable(TestPlot test_Plot.cpp)
target_link_libraries(TestPlot PUBLIC Plot)
gui_executable(TestPlot)
target_folder(TestPlot "Tests/wx")
