# README: APL Docker

This Dockerfile provides an image for building and testing the `APL Core Engine` library.
This image can be used to build and test the core library and to build the library
documentation.

## Docker

* Install Docker: https://docs.docker.com/get-started/
* Start in the APLCoreEngine directory
```bash
$ cd <workspace>
$ ls
APLCoreEngine   APLViewhostAndroid
$ cd APLCoreEngine
```

### 1) Create the Docker image

Build an image from the Dockerfile:
```bash
$ docker build -t apl:apl-core .
$ docker images
```

### 2) Running an interactive shell

Run an interactive bash shell to directly test the container.
```bash
$ docker run -it -v "$PWD":/apl/ apl:apl-core
```

### 3) Build and run the unit tests

Test the core engine by running the unit tests.  This command
expects your current directory to be `APLCoreEngine`:

```bash
$ docker run --rm -v "$PWD":/apl/ -w /apl apl:apl-core -c "source apl-dev-env.sh && apl-test-core"
```

If you had previously built and tested the core engine from outside of Docker, you may need to
remove your old build directory.  Append a `-f` (force) flag to the `apl-test-core` command.

### 4) Build the documentation

Build the core engine documentation.  This command expects
your current directory to be `APLCoreEngine`:

```bash
$ docker run --rm -v "$PWD":/apl/ -w /apl apl:apl-core -c "source apl-dev-env.sh && apl-build-doc"
```
