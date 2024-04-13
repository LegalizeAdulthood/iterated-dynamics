find_package(GIF REQUIRED)

add_executable(image-compare image_compare.cpp)
target_link_libraries(image-compare PUBLIC GIF::GIF)
