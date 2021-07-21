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

# Base image is Fedora 30
FROM fedora:30

#Update
RUN dnf distro-sync -y

# Install compiler and build tools
#   Versions in Fedora 30:
#      make               4.2.1
#      automake           1.16.1
#      cmake              3.14.5
#      gcc                9.0.1
#      gcc-c++            9.0.1
#      patch              2.7.6
RUN dnf install make automake cmake gcc gcc-c++ patch kernel-devel clang -y

# Make APL Core
ADD . /apl-core
RUN cd apl-core \
	&& rm -rf build \
	&& mkdir build \
	&& cd build \
	&& cmake -DBUILD_TESTS=ON -DCOVERAGE=OFF .. \
	&& make -j4

# RUN APL Core Tests
RUN cd apl-core/build \
	&& unit/unittest

# Make APL Core
ADD . /apl-core
RUN cd apl-core \
	&& rm -rf build-clang \
	&& mkdir build-clang \
	&& cd build-clang \
	&& cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DBUILD_TESTS=ON -DCOVERAGE=OFF .. \
	&& make -j4

# RUN APL Core Tests
RUN cd apl-core/build \
	&& unit/unittest
