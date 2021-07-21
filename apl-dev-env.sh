# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
#
#     http://aws.amazon.com/apache2.0/
#
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.

# This file to be sourced in the terminal for development
# Save the top-level APL directory

export APL_DIR=$(pwd)

# Amazon Linux uses cmake3
export CMAKE=$(command -v cmake3 || command -v cmake)

if [ -e /proc/cpuinfo ]; then # Linux
    APL_BUILD_PROCS=$(grep -c ^processor /proc/cpuinfo)
elif [ $(sysctl -n hw.ncpu) ]; then # Mac
    APL_BUILD_PROCS=$(sysctl -n hw.ncpu)
else # Other/fail
    APL_BUILD_PROCS=4
fi
export APL_BUILD_PROCS


function apl-parse-args {
    FORCE=false
    while test $# -gt 0 ; do
        case "$1" in
            -f|--force)
                echo "Forcing"
                FORCE=true
                ;;
            *)
                echo "Unexpected argument $1"
                ;;
        esac
        shift
    done
}

function apl-cd { # cd to the scl directory
    cd $APL_DIR
}

function apl-env-general {  # Set up non-Android environmental variables
    [[ -z "$CMAKE" ]] || return 0

    # Locate the version of CMake to use
    if [[ -z "$CMAKE" ]] ; then
        echo "Selecting a version of cmake"
        if [ -x "$(command -v cmake3)" ]; then
            CMAKE=cmake3
        elif [ -x "$(command -v cmake)" ]; then
            CMAKE=cmake
        else
            echo "Need to install a valid version of cmake"
            return 1
        fi
    fi

    echo "CMAKE=$CMAKE"
}

# Pass the name of the build directory you wish to create followed by all command line arguments
# This method parses the arguments and sets up the build directory.

function apl-switch-to-build-directory {
    SUB_DIR="$1"
    shift
    apl-parse-args $@ || return 1
    apl-env-general || return 1
    (
        cd $APL_DIR
        if [ $FORCE = true ] ; then
            echo "Removing $SUB_DIR directory"
            rm -rf $SUB_DIR
        fi
        mkdir -p $SUB_DIR
    )
    BUILD_DIR=$APL_DIR/$SUB_DIR
    echo "Running in $BUILD_DIR"
    cd $BUILD_DIR
}

# --------------------------------------
# Build and test the central C++ library
# --------------------------------------

function apl-clean-core {  # Clean the current build
    (
        apl-switch-to-build-directory build $@ && \
        [ -e "Makefile" ] && \
        make clean
    )
}

function apl-build-core {  # Run make for the core build
    (
        apl-switch-to-build-directory build $@ && \
        $CMAKE -DBUILD_TESTS=ON -DCOVERAGE=OFF .. && \
        make -j$APL_BUILD_PROCS
    )
}

function apl-check-core {  # Run make for the core build with -Werror
    (
        apl-switch-to-build-directory build $@ && \
        $CMAKE -DBUILD_TESTS=ON -DWERROR=ON -DCOVERAGE=OFF .. && \
        make -j$APL_BUILD_PROCS
    )
}

function apl-test-core {  # Run unit tests in the core build
    (
        apl-switch-to-build-directory build $@ && \
        $CMAKE -DBUILD_TESTS=ON  -DCOVERAGE=OFF .. && \
        make -j$APL_BUILD_PROCS && \
        unit/unittest
    )
}

function apl-memcheck-core {  # Run unit tests in the core build
    (
        apl-switch-to-build-directory build $@ && \
        $CMAKE -DBUILD_TESTS=ON  -DCOVERAGE=OFF .. && \
        make -j$APL_BUILD_PROCS && \
        valgrind --tool=memcheck --gen-suppressions=all --track-origins=yes --leak-check=full --num-callers=50 ./unit/unittest
    )
}

function apl-coverage-core {  # Generate and print coverage report
    (
        apl-switch-to-build-directory build $@ && \
        $CMAKE -DCOVERAGE=ON -DBUILD_TESTS=ON .. && \
        make -j$APL_BUILD_PROCS local-coverage
    )
}


# --------------------------------------
# Build the documentation
# --------------------------------------

function apl-clean-doc {  # Clean the documentation build
    (
        apl-switch-to-build-directory doc-build $@ && \
        [ -e "Makefile" ] && \
        make clean
    )
}

function apl-build-doc {  # Run make for the documentation
    (
        apl-switch-to-build-directory doc-build $@ && \
        $CMAKE .. && \
        make -j$APL_BUILD_PROCS doc
    )
}

function apl-open-doc {   # Open the documentation (works on MacOS)
    (
        apl-switch-to-build-directory doc-build $@ && \
        $CMAKE .. && \
        make -j$APL_BUILD_PROCS doc && \
        open html/index.html
    )
}

# --------------------------------------
# Build and run the Android code
# --------------------------------------

function apl-env-android {  # Set up Android environmental variables
    [[ -z "$NDK_HOME" || -z "$CMAKE" ]] || return 0

    # Locate the NDK
    if [[ -z "$NDK_HOME" ]] ; then
        echo "Searching for valid NDK_HOME"
        if [[ -d "$ANDROID_HOME/ndk-bundle" ]] ; then
            echo "Setting NDK_HOME relative to ANDROID_HOME"
            NDK_HOME=$ANDROID_HOME/ndk-bundle
        elif [[ -d "$HOME/Library/Android/sdk/ndk-bundle" ]] ; then  # MacOS
            echo "Setting NDK_HOME to MacOS user path"
            NDK_HOME="$HOME/Library/Android/sdk/ndk-bundle"
        elif [[ -d "$HOME/Android/Sdk/ndk-bundle" ]] ; then # Linux
            echo "Setting NDK_HOME to Linux user path"
            NDK_HOME="$HOME/Android/Sdk/ndk-bundle"
        else
            echo "Need to set ANDROID_HOME or NDK_HOME"
            return 1
        fi
    fi

    # Locate the version of CMake to use
    if [[ -z "$CMAKE" ]] ; then
        echo "Selecting a version of CMAKE"
        PS3="Use a number to select the version: "
        select cmakeversion in $(ls $NDK_HOME/../cmake)
        do
            if [[ "cmakeversion" == "" ]]
            then
                echo "'$cmakeversion' is not a valid choice"
                continue
            fi

            echo "$cmakeversion selected"
            break
        done

        echo "Selected version $cmakeversion"
        CMAKE="$NDK_HOME/../cmake/$cmakeversion/bin/cmake"
    fi

    echo "ANDROID_HOME=$ANDROID_HOME"
    echo "NDK_HOME=$NDK_HOME"
    echo "CMAKE=$CMAKE"
}

function apl-config-android { # Run cmake for Android
    (
        apl-parse-args $@ || return 1
        apl-env-android || return 1

        cd $APL_DIR

        TOOLCHAIN_FILE=$NDK_HOME/build/cmake/android.toolchain.cmake
        if [[ -z "$TOOLCHAIN_FILE" ]] ; then
            echo "Unable to find android.toolchain.cmake"
            return 1
        fi

        if [ $FORCE = true ] ; then
            echo "Removing android-build"
            rm -rf android-build
        fi

        mkdir -p android-build
        cd android-build
        echo "Running $CMAKE"
        $CMAKE -DCMAKE_BUILD_TYPE=Release \
               -DANDROID_ABI="armeabi-v7a" \
               -DANDROID_PLATFORM=android-24 \
               -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE \
               -DAPL_JNI=ON \
               -DBUILD_TESTS=ON ..
    )
}

function apl-clean-android {
    (
        apl-parse-args $@
        cd $APL_DIR

        if [ $FORCE = true ] ; then
            echo "Removing android-build"
            rm -rf android-build
        fi

        [[ -e build ]] || apl-config-android
        cd build && make clean
    )
}

function apl-build-android { # Run make for the Android build
    (
        cd $APL_DIR
        apl-config-android
        cd android-build && make -j$APL_BUILD_PROCS
    )
}

function apl-verify-android { # verify android - assemble, test, build without lint check
    (
    cd $APL_DIR/android
    ./gradlew build -x lint
    )
}

# --------------------------------------
# Build and run the WASM code
# --------------------------------------

function apl-config-wasm {  # Run cmake in the wasm build
    (
        cd $APL_DIR
        mkdir -p wasm-build
        cd wasm-build
        emcmake cmake -DEMSCRIPTEN=ON -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release -DEMSCRIPTEN_SOURCEMAPS=ON ..
    )
}

function apl-clean-wasm {
    (
        cd $APL_DIR
        apl-config-wasm

        cd wasm-build && make clean
    )
}

function apl-build-wasm { # Build the wasm source
    (
        cd $APL_DIR
        apl-config-wasm

        cd wasm-build && make -j$APL_BUILD_PROCS && make wasm-build
    )
}

function apl-install-wasm {  # Install the wasm source for the sandbox
    (
        cd $APL_DIR
        apl-build-wasm

        cd wasm-build && make install
    )
}

function apl-run-wasm-sandbox {  # Run the wasm sandbox.  Does not return
    (
        cd $APL_DIR
        apl-install-wasm

        cd wasm/js/apl-wasm-sandbox && node server.js public
    )
}

function apl-test-wasm {  # Run wasm tests.  Does not return
    (
        cd $APL_DIR
        apl-config-wasm

        cd wasm-build && make install && make wasm-test
    )
}

# ----------------------------------------------
# Build and test all of the different categories
# ----------------------------------------------

function apl-config-all { # Configure all of the build directories
    apl-config-core
    apl-config-wasm
}

function apl-clean-all {  # Clean all builds
    apl-clean-core
}

function apl-build-all { # Build all sources
    apl-build-core
    apl-build-wasm
}

function apl-test-all { # Test all sources
    apl-test-core
    apl-test-wasm
}

function apl-distclean { # Distclean removes all artifacts from building and configuring
    apl-clean-all
    rm -rf build # remove core artifacts
    rm -rf ./*-build* # remove viewhost artifacts
}
