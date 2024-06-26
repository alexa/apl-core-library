# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.
include(FetchContent OPTIONAL RESULT_VARIABLE HAS_FETCH_CONTENT)

cmake_minimum_required(VERSION 3.18.1)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
project (apl-android-native VERSION 1.0.0 LANGUAGES C CXX)

set(APL_PROJECT_DIR ${APL_PROJECT_DIR})

if (DEFINED SCENE_GRAPH)
    set(ENABLE_SCENEGRAPH ${SCENE_GRAPH})
endif()

# Tell core to compile alexa extensions.
set(ENABLE_ALEXAEXTENSIONS ON)
set(BUILD_ALEXAEXTENSIONS ON)
set(USE_PROVIDED_YOGA_INLINE ON)
set(ENABLE_PIC ON)

FetchContent_Declare(
        aplcore
        SOURCE_DIR ${APL_PROJECT_DIR}
)
FetchContent_MakeAvailable(aplcore)

set(RAPIDJSON_INCLUDE "${CMAKE_BINARY_DIR}/_deps/rapidjson-src/include")
set(ENUMGEN_BIN "${CMAKE_BINARY_DIR}/tools/enumgen")

add_custom_target(generate-android-enums ALL
        COMMAND cd ${APL_PROJECT_DIR} && ${ENUMGEN_BIN}
        -f  "AnimationQuality"
        -f  "AudioPlayerEventType"
        -f  "BlendMode"
        -f  "ComponentType"
        -f  "ContainerDirection"
        -f  "DimensionType"
        -f  "Display"
        -f  "DisplayState"
        -f  "EventAudioTrack"
        -f  "EventControlMediaCommand"
        -f  "EventDirection"
        -f  "EventHighlightMode"
        -f  "EventProperty"
        -f  "EventReason"
        -f  "EventScrollAlign"
        -f  "EventType"
        -f  "EventMediaType"
        -f  "FilterType"
        -f  "FilterProperty"
        -f  "FlexboxAlign"
        -f  "FlexboxJustifyContent"
        -f  "FocusDirection"
        -f  "FontStyle"
        -f  "GradientProperty"
        -f  "GradientSpreadMethod"
        -f  "GradientType"
        -f  "GradientUnits"
        -f  "GraphicTextAnchor"
        -f  "GraphicElementType"
        -f  "GraphicLayoutDirection"
        -f  "GraphicLineCap"
        -f  "GraphicLineJoin"
        -f  "GraphicPropertyKey"
        -f  "GraphicFilterType"
        -f  "GraphicFilterProperty"
        -f  "GraphicScale"
        -f  "GraphicScale"
        -f  "ImageAlign"
        -f  "ImageCount"
        -f  "ImageScale"
        -f  "KeyHandlerType"
        -f  "LayoutDirection"
        -f  "MediaPlayerEventType"
        -f  "Navigation"
        -f  "NoiseFilterKind"
        -f  "Position"
        -f  "PointerEventType"
        -f  "PointerType"
        -f  "PropertyKey"
        -f  "RootProperty"
        -f  "ScreenShape"
        -f  "ScrollDirection"
        -f  "SpanAttributeName"
        -f  "SpanType"
        -f  "Snap"
        -f  "SpeechMarkType"
        -f  "TextAlign"
        -f  "TextAlignVertical"
        -f  "TextTrackType"
        -f  "TokenType"
        -f  "TrackState"
        -f  "UpdateType"
        -f  "VectorGraphicAlign"
        -f  "VectorGraphicScale"
        -f  "VideoScale"
        -f  "ViewportMode"
        -f  "AudioTrack"
        -f  "KeyboardType"
        -f  "SubmitKeyType"
        -f  "ScreenMode"
        -f  "Role"
        -f  "ExtensionComponentResourceState"
        -l java -p com.amazon.apl.enums -o ${CMAKE_CURRENT_SOURCE_DIR}/src/main/java/com/amazon/apl/enums
        ${APL_PROJECT_DIR}/aplcore/include/apl/action/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/animation/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/audio/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/command/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/component/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/content/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/datagrammar/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/document/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/engine/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/graphic/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/media/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/primitives/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/time/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/utils/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/touch/*.h
        ${APL_PROJECT_DIR}/aplcore/include/apl/focus/*.h
        DEPENDS enumgen
        )

get_target_property(RAPIDJSON_INCLUDE rapidjson-apl INTERFACE_INCLUDE_DIRECTORIES)
add_custom_target(rapidjson ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${RAPIDJSON_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/rapidjson/include
        )

get_target_property(APL_INCLUDE apl INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(ALEXAEXT_INCLUDE alexaext INTERFACE_INCLUDE_DIRECTORIES)

add_custom_target(copy-headers
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${APL_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/apl/include
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${ALEXAEXT_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/alexaext/include
        DEPENDS apl alexaext
        )

if (ENABLE_SCENEGRAPH)
    add_custom_target(copy-config
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/apl/include/apl/apl_config.h ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/aplsgconfig/include/apl/apl_config.h
            DEPENDS copy-headers)
else()
    add_custom_target(copy-config
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/apl/include/apl/apl_config.h ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/aplconfig/include/apl/apl_config.h
            DEPENDS copy-headers)
endif()

if (NOT ANDROID)
    # Ensure jni.h is found
    find_package(JNI REQUIRED)
    include_directories(${JAVA_INCLUDE_PATH})
    include_directories(${JAVA_INCLUDE_PATH2})
endif()
