# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.
#
find_package(fmt CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(tgautils CONFIG REQUIRED)

add_subdirectory(image-tool)

add_executable(image-compare image_compare.cpp)
target_link_libraries(image-compare PUBLIC image-tool fmt::fmt-header-only PNG::PNG tgautils::tga)
target_folder(image-compare "Tests/Tools")

add_executable(image-dump image_dump.cpp)
target_link_libraries(image-dump PUBLIC image-tool)
target_folder(image-dump "Tests/Tools")

add_executable(colormap-compare colormap_compare.cpp)
target_link_libraries(colormap-compare PUBLIC image-tool)
target_folder(colormap-compare "Tests/Tools")

add_executable(check-command-diagnostics check_command_diagnostics.cpp)
target_link_libraries(check-command-diagnostics PRIVATE libid fmt::fmt-header-only)
target_folder(check-command-diagnostics "Tests/Tools")

add_executable(check-image-restore check_image_restore.cpp)
target_link_libraries(check-image-restore PRIVATE libid fmt::fmt-header-only)
target_folder(check-image-restore "Tests/Tools")

add_executable(check-standard-resume check_standard_resume.cpp)
target_link_libraries(check-standard-resume PRIVATE image-tool libid)
target_folder(check-standard-resume "Tests/Tools")

add_executable(check-show-dot check_show_dot.cpp)
target_link_libraries(check-show-dot PRIVATE image-tool libid fmt::fmt-header-only)
target_folder(check-show-dot "Tests/Tools")

add_executable(check-standard-validation check_standard_validation.cpp)
target_link_libraries(check-standard-validation PRIVATE image-tool libid fmt::fmt-header-only)
target_folder(check-standard-validation "Tests/Tools")

add_executable(check-ifs-validation check_ifs_validation.cpp)
target_link_libraries(check-ifs-validation PRIVATE image-tool libid fmt::fmt-header-only)
target_folder(check-ifs-validation "Tests/Tools")

add_executable(check-formula-validation check_formula_validation.cpp)
target_link_libraries(check-formula-validation PRIVATE image-tool libid fmt::fmt-header-only)
target_folder(check-formula-validation "Tests/Tools")

add_executable(check-pass-switch check_pass_switch.cpp)
target_link_libraries(check-pass-switch PRIVATE image-tool libid)
target_folder(check-pass-switch "Tests/Tools")

add_executable(check-cellular-preparation check_cellular_preparation.cpp)
target_link_libraries(check-cellular-preparation PRIVATE image-tool libid)
target_folder(check-cellular-preparation "Tests/Tools")

add_executable(check-lsystem-sizing check_lsystem_sizing.cpp)
target_link_libraries(check-lsystem-sizing PRIVATE image-tool libid)
target_folder(check-lsystem-sizing "Tests/Tools")

add_executable(check-lsystem-validation check_lsystem_validation.cpp)
target_link_libraries(check-lsystem-validation PRIVATE image-tool libid fmt::fmt-header-only)
target_folder(check-lsystem-validation "Tests/Tools")

add_executable(check-lyapunov-setup check_lyapunov_setup.cpp)
target_link_libraries(check-lyapunov-setup PRIVATE image-tool libid fmt::fmt-header-only)
target_folder(check-lyapunov-setup "Tests/Tools")

add_executable(check-jiim check_jiim.cpp)
target_link_libraries(check-jiim PRIVATE image-tool libid)
target_folder(check-jiim "Tests/Tools")

if(WIN32)
    add_executable(dump-dpi dump_dpi.cpp)
    target_link_libraries(dump-dpi "Shcore.lib" "User32.lib")
    target_folder(dump-dpi "Tests/Tools")
endif()
