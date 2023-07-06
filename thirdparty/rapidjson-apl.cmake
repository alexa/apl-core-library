############################################################
# RapidJSON is a header-only library
############################################################

set(RAPIDJSON_SOURCE_URL "${APL_PROJECT_DIR}/thirdparty/rapidjson-v1.1.0.tar.gz")
set(RAPIDJSON_SOURCE_MD5 "badd12c511e081fec6c89c43a7027bce")
set(USE_RAPIDJSON_PACKAGE FALSE)

if (USE_SYSTEM_RAPIDJSON)
    find_package(RapidJSON QUIET)
    if (RapidJSON_FOUND)
        set(RAPIDJSON_INCLUDE ${RAPIDJSON_INCLUDE_DIRS})
        set(USE_RAPIDJSON_PACKAGE TRUE)
    else()
        find_path(RAPIDJSON_INCLUDE
            NAMES rapidjson/document.h
            REQUIRED)
    endif()
elseif (HAS_FETCH_CONTENT)
    FetchContent_Declare(rapidjson
        URL ${RAPIDJSON_SOURCE_URL}
        URL_MD5 ${RAPIDJSON_SOURCE_MD5}
        PATCH_COMMAND patch ${PATCH_FLAGS} -p1 < ${APL_PATCH_DIR}/rapidjson.patch
        )
    FetchContent_Populate(rapidjson)
    set(RAPIDJSON_INCLUDE ${rapidjson_SOURCE_DIR}/include)
else()
    ExternalProject_Add(rapidjson
        URL ${RAPIDJSON_SOURCE_URL}
        URL_MD5 ${RAPIDJSON_SOURCE_MD5}
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

message(VERBOSE "Rapidjson include directory ${RAPIDJSON_INCLUDE}")

add_library(rapidjson-apl INTERFACE)
target_include_directories(rapidjson-apl INTERFACE
    # When we're building against RapidJSON, just use the include directory we discovered above
    $<BUILD_INTERFACE:${RAPIDJSON_INCLUDE}>
)

if (USE_SYSTEM_RAPIDJSON)
    # If we're using the system rapidjson, then use the full include path in our generated config
    target_include_directories(rapidjson-apl INTERFACE
        $<INSTALL_INTERFACE:${RAPIDJSON_INCLUDE}>
    )
else()
    # If we're using the bundled rapidjson, use the relative path to "include" in our generated config
    target_include_directories(rapidjson-apl INTERFACE
        $<INSTALL_INTERFACE:include>
    )
endif()

if (NOT USE_SYSTEM_RAPIDJSON AND NOT HAS_FETCH_CONTENT)
    add_dependencies(rapidjson-apl rapidjson-build)
endif()

if (NOT USE_SYSTEM_RAPIDJSON)
    # If we're using the bundled RapidJSON, make sure to install it along with the rest of the APL Core files
    install(DIRECTORY ${RAPIDJSON_INCLUDE}/rapidjson
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/aplrapidjsonConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/aplrapidjsonConfig.cmake"
    INSTALL_DESTINATION
        lib/cmake/aplrapidjson
    NO_CHECK_REQUIRED_COMPONENTS_MACRO)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/aplrapidjsonConfig.cmake
    DESTINATION
        lib/cmake/aplrapidjson
)

# Create an export for rapidjson-apl so that other exported modules can depend on it
install(
    TARGETS rapidjson-apl
    EXPORT aplrapidjson-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

export(
    EXPORT
        aplrapidjson-targets
)

install(
    EXPORT
        aplrapidjson-targets
    DESTINATION
        lib/cmake/aplrapidjson
    FILE
        aplrapidjsonTargets.cmake
)