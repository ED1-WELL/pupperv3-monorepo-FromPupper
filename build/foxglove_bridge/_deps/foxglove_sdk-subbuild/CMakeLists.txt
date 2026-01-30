# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.25.1)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(foxglove_sdk-populate NONE)



include(ExternalProject)
ExternalProject_Add(foxglove_sdk-populate
                     "UPDATE_DISCONNECTED" "False" "DOWNLOAD_EXTRACT_TIMESTAMP" "TRUE" "URL" "https://github.com/foxglove/foxglove-sdk/releases/download/sdk%2Fv0.14.2/foxglove-v0.14.2-cpp-aarch64-unknown-linux-gnu.zip" "URL_HASH" "SHA256=f54e3e08926a8882a9bc8031cfb24a040be2638f1ef4e87236fa3aded1ab2a23"
                    SOURCE_DIR          "/home/pi/pupperv3-monorepo/ros2_ws/build/foxglove_bridge/_deps/foxglove_sdk-src"
                    BINARY_DIR          "/home/pi/pupperv3-monorepo/ros2_ws/build/foxglove_bridge/_deps/foxglove_sdk-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
                    USES_TERMINAL_PATCH     YES
)


