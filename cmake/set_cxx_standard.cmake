function(set_cxx_standard version)
    set(CMAKE_CXX_STANDARD ${version} PARENT_SCOPE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)
endfunction()
