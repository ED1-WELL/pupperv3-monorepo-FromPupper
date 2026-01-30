# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/pi/pupperv3-monorepo/ros2_ws/build/foxglove_bridge/_deps/foxglove_sdk-src"
  "/home/pi/pupperv3-monorepo/ros2_ws/build/foxglove_bridge/_deps/foxglove_sdk-build"
  "/home/pi/pupperv3-monorepo/ros2_ws/build/foxglove_bridge/_deps/foxglove_sdk-subbuild/foxglove_sdk-populate-prefix"
  "/home/pi/pupperv3-monorepo/ros2_ws/build/foxglove_bridge/_deps/foxglove_sdk-subbuild/foxglove_sdk-populate-prefix/tmp"
  "/home/pi/pupperv3-monorepo/ros2_ws/build/foxglove_bridge/_deps/foxglove_sdk-subbuild/foxglove_sdk-populate-prefix/src/foxglove_sdk-populate-stamp"
  "/home/pi/pupperv3-monorepo/ros2_ws/build/foxglove_bridge/_deps/foxglove_sdk-subbuild/foxglove_sdk-populate-prefix/src"
  "/home/pi/pupperv3-monorepo/ros2_ws/build/foxglove_bridge/_deps/foxglove_sdk-subbuild/foxglove_sdk-populate-prefix/src/foxglove_sdk-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/pi/pupperv3-monorepo/ros2_ws/build/foxglove_bridge/_deps/foxglove_sdk-subbuild/foxglove_sdk-populate-prefix/src/foxglove_sdk-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/pi/pupperv3-monorepo/ros2_ws/build/foxglove_bridge/_deps/foxglove_sdk-subbuild/foxglove_sdk-populate-prefix/src/foxglove_sdk-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
