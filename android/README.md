# Introduction
This folder contains an Android gradle project that will build *APLCoreEngine* with the Android SDK/NDK and publish the static library binaries as a prefab with a coordinate of `com.amazon.apl.android:coreengine`.

# Publishing
By default, the gradle project contains the `publishToMavenLocal` task which will publish `com.amazon.apl.android:coreengine` to a local maven repository. Additional publish targets must be added by the user.

# Unit Tests
In order to run unit tests that include coverage of calls made to native libraries, this gradle project is capable of building *APLCoreEngine* using the default tool chain located on the host computer. Invoking the gradle task `buildHostBinary` will build `apl` and `alexaext` static libraries capable of being used in unit tests.

# Shortcuts
Running the `localBuild` task will call both `publishToMavenLocal` and `buildHostBinary` automatically.