# Introduction
The rendering of screen layouts at runtime for the Alexa Presentation Language
is controlled by the APL Core Library, which is an abstract engine that manages
not only APL document parsing and layout inflation, but also event handling,
commands, and the rendering workflow.  The APL Core Library interfaces
with platform and language-specific APL "view hosts", which are responsible for
performing the rendering in the platform or framework for which the view host
was designed.

Consumers of the APL Core Library can create their own APL view host in order to
create Alexa experiences with visual renderings on their device or platform in
the programming language of their choice.

# Architecture
```
                              APL Core Library

         +----------------------------------------------------------+
         | +---------------+  +----------------+ +----------------+ |
         | |               |  |                | |                | |
         | |     APL       |  |    Inflation   | |   Command      | |
         | |   Document    |  |    + Layout    | |   Processing   | |
         | |    Parsing    |  |    Management  | |                | |
         | |               |  |                | |                | |
         | +---------------+  +----------------+ +----------------+ |
         | +---------------+  +----------------+ +----------------+ |
         | |               |  |                | |                | |
         | |    Event      |  |     Data       | |    Viewport    | |
         | |   Handling    |  |    Binding     | |                | |
         | |               |  |                | |                | |
         | |               |  |                | |                | |
         | +---------------+  +----------------+ +----------------+ |
         | ........................................................ |
         | .:                                                    .: |
         | .:               RootContext & Content                .: |
         | .:                                                    .: |
         | .:''''''/''''''''''''''''''/''''\''''''''''''''''\''''.: |
         +--------+------------------+------+----------------+------+
                 /                  /        \                \
 +--------+     /   +--------+     /          \     +----+     \     +------+
 | OS 1   |    /    | OS 1   |    /            \    |OS 3|      \    |OS 4  |
 | App 1  |   /     | App 2  |   /              \   |App |       \   |App   |
 |        |  /      |        |  /                \  |    |        \  |      |
 +--------+-+----+  +--------+-+----+    +--------+-+----+  +------+-+------+
 |     OS1 Native|  | Cross Platform|    | Cross Platform|  |    OS4 Native |
 |     View Host |  | View Host     |    | View Host     |  |    View Host  |
 |               |  |               |    |               |  |               |
 +---------------+  +---------------+    +---------------+  +---------------+
 ```

Above is shown the high-level architecture of the APL Core Library, showing 
its interaction with a few possible view host implementations.

### APL Document Parsing
APL Core is responsible for parsing and validating APL documents.

### Inflation + Layout Management
APL Core is responsible for layout inflation into a **Virtual DOM** (with all
 its resources, styles and data-bound expressions evaluated)

### Command Processing / Event Handling
After the APL Document is parsed, any commands specified in the document are
inserted into a Command Sequence.  Commands are exposed to the app via the
**Content** object.  APL Core is also responsible for handling and reporting
contextual changes, UI, and other events.

### Data Binding
APL Documents allow for the separation of presentation and data by providing
a data binding mechanism that links document values to a data model.

### Viewport
APL Core provides device viewport information such as theme, shape, height,
width, and resolution.

### RootContext and Content
The view hosts interface with APL Core through the **RootContext** and
**Content** objects.  The **RootContext** provides an interface for
view host driven advancement of lifecycle events such as view hierarchy
creation, text layout requests, change events, and command events.
The **Content** represents the APL Document content.

# View Host Guide
## Separation of Concerns

In order to get a grasp on how to create a view host, it is good to be aware
of the separation of concerns between the APL Core Library
and the View Host.  APL Core is a library that provides the document parsing, 
bookkeeping and logic associated with displaying and controlling a
visual, auditory and tactile experience on a device.  APL Core is
platform-independent and operates on view *_abstractions_*.  The view host is
responsible for interfacing with APL Core to create and manage *_concrete_* views
representing the visual experience.

## View Host Responsibilities
### Execution and Control Flow
The view host is in charge.  The view host drives the execution and control flow
and interfaces with the APL Core Library.  Apart from a text measurement callback,
there are no other callbacks from the Core to the view host.  In all other cases,
it is the view host that calls into the APL Core Library.
	
### APL Document and Content
The view host obtains the APL document to be rendered and passes it to core as
a parameter to Content::create().  The Content is passed as a parameter to
RootContext::create.

### APL RootContext
The view host creates a new APL RootContext, providing a viewport definition and
the Content.
	see RootContext::create

### Inflation
The APL Core parses the document and generates a virtual DOM hierarchy of 
APL Components. The view host constructs and renders concrete native views
that represent the virtual DOM.  

### Run Loop
Execute a "run loop" that in a typical scenario, executes a single iteration
of the loop per display frame.
On every frame, the run loop does the following:

- Calls RootContext::updateTime with a new timestamp for the frame.

- Run Loop calls RootContext::hasEvent() - if true, it pops all events off of
the queue and dispatches them appropriately.

- Checks the RootContext for dirty properties and applies them to components.

**Run Loop Pseudocode:**

```
	BEGIN ON_NEW_FRAME
		Set current timestamp (RootContext::updateTime)
		WHILE (RootContext::hasEvent())
	 		RootContext::popEvent()
	 		Handle the Event
		END WHILE
		IF RootContext::isDirty THEN
	 		handleDirtyProps(RootContext::getDirty)
		END IF
	END ON_NEW_FRAME
```

### Command Execution
An external source may invoke commands on the APL document.  The view host
passes calls RootContext::executeCommands to execute them and is informed
of virtual DOM changes by the dirty properties set on components.  The
view host can cancel the command execution with RootContext::cancelCommands.

### Keystroke Events
The view host is responsible for notifying APL Core of keystroke and
input focus events.

#  Build Prerequisites
- Supported Compilers:
  - GNU GCC and G++ version 5.3.1 or higher
  - LLVM and clang version 6.0.0 or higher
  - MSVC version 16 2019 or higher
- CMake, version 3.5 or higher
- For Windows, Ninja build system, 1.9.0 or higher, see:
  https://github.com/ninja-build/ninja
- For Windows, the patch executable should be accessible on the system's PATH.
  The Windows build has been tested using the patch executable that ships with Git.
  The GnuWin32 patch executable is NOT recommended.

# Building APL Core + Tests (Linux and Mac OS)
To build `libaplcore.a` and tests in your favorite C++ environment, do the
following:

```
 source apl-dev-env.sh

 # Build the library
 apl-build-core

 # Run unit tests
 apl-test-core

 # Generate code coverage
 apl-coverage-core

 # Run memcheck
 apl-check-core
```

# Building APL Core + Tests (Windows)
To build `apl.lib` and tests in in a Windows or UWP environment, do the
following:

```
 Start a VS Command Prompt (tested with "x64 Native Tools Command Prompt for VS 2019")
 C:\APLCoreEngine> mkdir build
 C:\APLCoreEngine> cd build
 C:\APLCoreEngine\build> cmake -G"Ninja" -DBUILD_TESTS=ON -DCOVERAGE=OFF -DCMAKE_BUILD_TYPE=Release ..
 C:\APLCoreEngine\build> ninja -j1
```
(If using the GnuWin32 patch.exe instead Git's patch.exe, the command prompt may need to run in
 Administrator Mode)

# Running Tests
After the build succeeds, there are a number of test programs included in the `build/test`
directory.  For example, to validate the color parser, try:

```
 $ build/test/parseColor green
```

The most useful test program is `parseLayout`, which takes two data files as input: an
APL document and an APL data file.  The document will be processed and expanded into a
component hierarchy by applying the data file.

# Build Verification Using Docker
In order to verify that the build will work on various platforms, we have
included Docker images for some of the more popular platforms.  For instance,
to verify that the build works on an Ubuntu 18.04 system, install Docker and
run the following from the root directory of the APL Core Library project:

```
 $ docker build -f ubuntu18.04.Dockerfile  .
```
The Docker files for verifying the build on various platforms have the
extension ".Dockerfile".  They can be found by running the following
from the root directory of the project:

```
 $ ls *.Dockerfile
```
For more info on installing and configuring Docker, see:
    https://www.docker.com/get-started

# Global configuration
## Tracing
In order to compile core tracing support use TRACING cmake parameter:
```
$ cmake -DTRACING=ON
```

## Memory debugging
To build lib with memory debugging support use:
```
$ cmake -DDEBUG_MEMORY_USE=ON
```

## Tools
In order to build the tools use:
```
$ cmake -DTOOLS=ON
```

## Paranoid build
In order to build library with -Werror use:
```
$ cmake -DWERROR=ON
```

# Alpha features

An Alpha feature may become part of the APL specification in a future release. These features are made available
before the specification has finalized to allow for early experimentation with the features. Early adopters should
expect that for alpha features that are released, APIs and data structures are likely to change when the
specification is finalized.
