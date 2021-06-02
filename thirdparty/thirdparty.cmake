include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

if(NOT APL_PROJECT_DIR)
    set(APL_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

list(APPEND CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

get_cmake_property(CACHE_VARS CACHE_VARIABLES)
foreach(CACHE_VAR ${CACHE_VARS})
    get_property(CACHE_VAR_HELPSTRING CACHE ${CACHE_VAR} PROPERTY HELPSTRING)
    if(CACHE_VAR_HELPSTRING STREQUAL "No help, variable specified on the command line.")
        get_property(CACHE_VAR_TYPE CACHE ${CACHE_VAR} PROPERTY TYPE)
        if(CACHE_VAR_TYPE STREQUAL "UNINITIALIZED")
            set(CACHE_VAR_TYPE)
        else()
            set(CACHE_VAR_TYPE :${CACHE_VAR_TYPE})
        endif()
        list(APPEND CMAKE_ARGS -D${CACHE_VAR}${CACHE_VAR_TYPE}=${${CACHE_VAR}})
    endif()
endforeach()

set(EXT_CXX_ARGS "-std=c++11 ${WASM_FLAGS} ${CMAKE_CXX_FLAGS}")
if (BUILD_SHARED OR ENABLE_PIC)
    set(EXT_CXX_ARGS "${EXT_CXX_ARGS} -fPIC")
endif()
list(APPEND CMAKE_ARGS -DCMAKE_CXX_FLAGS=${EXT_CXX_ARGS})
list(APPEND CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/lib)
list(APPEND CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})

if (YOGA_EXTERNAL_INSTALL_DIR)
    # Use an externally provided Yoga library
    find_path(YOGA_INCLUDE
        NAMES yoga/Yoga.h
        PATHS ${YOGA_EXTERNAL_INSTALL_DIR}/include
        REQUIRED)
    find_library(YOGA_LIB
        NAMES yoga
        PATHS ${YOGA_EXTERNAL_INSTALL_DIR}/lib
        REQUIRED)
    set(YOGA_EXTERNAL_LIB ${YOGA_LIB}) # used by aplcoreConfig.cmake.in
elseif (USE_SYSTEM_YOGA)
    # Use yoga provided by the system
    find_path(YOGA_INCLUDE
        NAMES yoga/Yoga.h
        PATHS ${CMAKE_SYSROOT}/usr/include
        REQUIRED)
    find_library(YOGA_LIB
        NAMES yoga
        PATHS ${CMAKE_SYSROOT}/usr/lib
        REQUIRED)
    set(YOGA_EXTERNAL_LIB ${YOGA_LIB}) # used by aplcoreConfig.cmake.in
else()
    # Build the bundled Yoga library
    ExternalProject_Add(yoga
        URL ${APL_PROJECT_DIR}/thirdparty/yoga-1.16.0.tar.gz
        URL_MD5 c9e88076ec371513fb23a0a5370ec2fd
        EXCLUDE_FROM_ALL TRUE
        INSTALL_DIR ${CMAKE_BINARY_DIR}/lib
        PATCH_COMMAND patch ${PATCH_FLAGS} -p1 < ${APL_PATCH_DIR}/yoga.patch
        BUILD_BYPRODUCTS <INSTALL_DIR>/${CMAKE_STATIC_LIBRARY_PREFIX}yogacore${CMAKE_STATIC_LIBRARY_SUFFIX}
        CMAKE_ARGS ${CMAKE_ARGS}
        )

    ExternalProject_Get_Property(yoga install_dir)
    ExternalProject_Get_Property(yoga source_dir)
    set(YOGA_INCLUDE ${source_dir})
    set(YOGA_LIB ${install_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}yogacore${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_library(libyoga STATIC IMPORTED)
set_target_properties(libyoga
    PROPERTIES
        IMPORTED_LOCATION
            "${YOGA_LIB}"
    )
message(VERBOSE Using yoga include directory = ${YOGA_INCLUDE})
message(VERBOSE Using yoga lib = ${YOGA_LIB})

ExternalProject_Add(pegtl
        URL ${APL_PROJECT_DIR}/thirdparty/pegtl-2.8.3.tar.gz
        URL_MD5 28b3c455d9ec392dd4230402383a8c6f
        PATCH_COMMAND patch ${PATCH_FLAGS} -p1 < ${APL_PATCH_DIR}/pegtl.patch
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        CMAKE_ARGS ${CMAKE_ARGS}
        )
ExternalProject_Get_Property(pegtl install_dir)
set(PEGTL_INCLUDE ${install_dir}/src/pegtl/include)

if (USE_SYSTEM_RAPIDJSON)
    find_path(RAPIDJSON_INCLUDE
        NAMES rapidjson/document.h
        REQUIRED)
else()
    ExternalProject_Add(rapidjson
            URL ${APL_PROJECT_DIR}/thirdparty/rapidjson-v1.1.0.tar.gz
            URL_MD5 badd12c511e081fec6c89c43a7027bce
            PATCH_COMMAND patch ${PATCH_FLAGS} -p1 < ${APL_PATCH_DIR}/rapidjson.patch
            STEP_TARGETS build
            EXCLUDE_FROM_ALL TRUE
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            CMAKE_ARGS ${CMAKE_ARGS}
            )
    ExternalProject_Get_Property(rapidjson install_dir)
    set(RAPIDJSON_INCLUDE ${install_dir}/src/rapidjson/include)
endif()

# Unpack googletest at configure time.  This is copied from the googletest README.md file
configure_file(${APL_PROJECT_DIR}/thirdparty/googletest-CMakeLists.txt.in
               ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt )

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
