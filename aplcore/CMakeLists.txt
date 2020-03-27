set(CMAKE_CXX_STANDARD 11)

# Set template depth to support pegtl on older versions of clang.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=1024")
endif()

add_library(apl STATIC
        src/action/action.cpp
        src/action/animateitemaction.cpp
        src/action/autopageaction.cpp
        src/action/arrayaction.cpp
        src/action/extensioneventaction.cpp
        src/action/documentaction.cpp
        src/action/openurlaction.cpp
        src/action/scrolltoaction.cpp
        src/action/sequentialaction.cpp
        src/action/setpageaction.cpp
        src/action/speakitemaction.cpp
        src/action/speaklistaction.cpp
        src/animation/animatedproperty.cpp
        src/animation/easing.cpp
        src/command/arraycommand.cpp
        src/command/commandfactory.cpp
        src/command/commandproperties.cpp
        src/command/corecommand.cpp
        src/command/extensioneventcommand.cpp
        src/command/documentcommand.cpp
        src/component/actionablecomponent.cpp
        src/component/component.cpp
        src/component/componentproperties.cpp
        src/component/containercomponent.cpp
        src/component/corecomponent.cpp
        src/component/framecomponent.cpp
        src/component/imagecomponent.cpp
        src/component/pagercomponent.cpp
        src/component/scrollablecomponent.cpp
        src/component/scrollviewcomponent.cpp
        src/component/sequencecomponent.cpp
        src/component/textcomponent.cpp
        src/component/textmeasurement.cpp
        src/component/touchwrappercomponent.cpp
        src/component/vectorgraphiccomponent.cpp
        src/component/videocomponent.cpp
        src/component/yogaproperties.cpp
        src/content/aplversion.cpp
        src/content/content.cpp
        src/content/directive.cpp
        src/content/importrequest.cpp
        src/content/jsondata.cpp
        src/content/metrics.cpp
        src/content/package.cpp
        src/content/rootconfig.cpp
        src/content/viewport.cpp
        src/datagrammar/boundsymbol.cpp
        src/datagrammar/functions.cpp
        src/datagrammar/node.cpp
        src/datasource/datasource.cpp
        src/datasource/dynamicindexlistdatasourceprovider.cpp
        src/datasource/offsetindexdatasourceconnection.cpp
        src/engine/arrayify.cpp
        src/engine/binding.cpp
        src/engine/builder.cpp
        src/engine/context.cpp
        src/engine/componentdependant.cpp
        src/engine/contextdependant.cpp
        src/engine/contextobject.cpp
        src/engine/dependant.cpp
        src/engine/evaluate.cpp
        src/engine/event.cpp
        src/engine/extensionmanager.cpp
        src/engine/focusmanager.cpp
        src/engine/hovermanager.cpp
        src/engine/info.cpp
        src/engine/keyboardmanager.cpp
        src/engine/parameterarray.cpp
        src/engine/propdef.cpp
        src/engine/properties.cpp
        src/engine/resources.cpp
        src/engine/rootcontext.cpp
        src/engine/rootcontextdata.cpp
        src/engine/state.cpp
        src/engine/styleinstance.cpp
        src/engine/styledefinition.cpp
        src/engine/styles.cpp
        src/livedata/livearrayobject.cpp
        src/livedata/livedataobject.cpp
        src/livedata/livedatamanager.cpp
        src/livedata/layoutrebuilder.cpp
        src/livedata/livearray.cpp
        src/livedata/livemap.cpp
        src/livedata/livemapobject.cpp
        src/graphic/graphic.cpp
        src/graphic/graphiccontent.cpp
        src/graphic/graphicdependant.cpp
        src/graphic/graphicelement.cpp
        src/graphic/graphicproperties.cpp
        src/primitives/color.cpp
        src/primitives/dimension.cpp
        src/primitives/filter.cpp
        src/primitives/functions.cpp
        src/primitives/gradient.cpp
        src/primitives/keyboard.cpp
        src/primitives/mediasource.cpp
        src/primitives/radii.cpp
        src/primitives/rect.cpp
        src/primitives/symbolreferencemap.cpp
        src/primitives/styledtext.cpp
        src/primitives/timefunctions.cpp
        src/primitives/timegrammar.cpp
        src/primitives/transform.cpp
        src/primitives/transform2d.cpp
        src/primitives/object.cpp
        src/scaling/metricstransform.cpp
        src/scaling/scalingcalculator.cpp
        src/time/sequencer.cpp
        src/utils/log.cpp
        src/utils/path.cpp
        src/utils/session.cpp
        src/utils/telemetry.cpp)

set_target_properties(apl PROPERTIES
                      VERSION "${PROJECT_VERSION}"
                      SOVERSION 1
                      PUBLIC_HEADER include/apl/apl.h)

configure_file(apl.pc.in apl.pc @ONLY)
target_include_directories(apl PRIVATE include)

add_library(libyoga STATIC IMPORTED)
set_target_properties(libyoga PROPERTIES
    IMPORTED_LINK_INTERFACE_LIBRARIES ""
    IMPORTED_LOCATION "${YOGA_LIB}"
)
target_link_libraries(apl libyoga)


# The core library
add_dependencies(apl yoga)
add_dependencies(apl pegtl-build)
add_dependencies(apl rapidjson-build)

include_directories(include)
include_directories(${RAPIDJSON_INCLUDE})
include_directories(${PEGTL_INCLUDE})
include_directories(${YOGA_INCLUDE})

# Log dependencies for Android

if (ANDROID)

# Searches for the prebuilt Android NDK log library and stores the path as a
# variable. CMake verifies that the library exists before
# completing its build.
find_library(
        # Sets the name of the path variable.
        log-lib
        # Specifies the name of the NDK library that you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.
target_link_libraries(
        # Specifies the target library.
        apl
        # Links the target library to the log library included in the NDK.
        ${log-lib})

endif(ANDROID)