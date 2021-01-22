MACRO(find_conan_package package_name)
    find_package(${package_name} QUIET)
    if (NOT ${${package_name}_FOUND})
    conan_cmake_install(BUILD missing ${ARGN}) # pass any extra arguments on to function
    
    find_package(${package_name} REQUIRED)
    endif()
ENDMACRO()