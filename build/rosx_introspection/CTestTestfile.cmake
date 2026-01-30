# CMake generated Testfile for 
# Source directory: /home/pi/pupperv3-monorepo/ros2_ws/src/common/rosx_introspection
# Build directory: /home/pi/pupperv3-monorepo/ros2_ws/build/rosx_introspection
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(rosx_introspection_test "/usr/bin/python3" "-u" "/opt/ros/jazzy/share/ament_cmake_test/cmake/run_test.py" "/home/pi/pupperv3-monorepo/ros2_ws/build/rosx_introspection/test_results/rosx_introspection/rosx_introspection_test.gtest.xml" "--package-name" "rosx_introspection" "--output-file" "/home/pi/pupperv3-monorepo/ros2_ws/build/rosx_introspection/ament_cmake_gtest/rosx_introspection_test.txt" "--command" "/home/pi/pupperv3-monorepo/ros2_ws/build/rosx_introspection/rosx_introspection_test" "--gtest_output=xml:/home/pi/pupperv3-monorepo/ros2_ws/build/rosx_introspection/test_results/rosx_introspection/rosx_introspection_test.gtest.xml")
set_tests_properties(rosx_introspection_test PROPERTIES  LABELS "gtest" REQUIRED_FILES "/home/pi/pupperv3-monorepo/ros2_ws/build/rosx_introspection/rosx_introspection_test" TIMEOUT "60" WORKING_DIRECTORY "/home/pi/pupperv3-monorepo/ros2_ws/build/rosx_introspection" _BACKTRACE_TRIPLES "/opt/ros/jazzy/share/ament_cmake_test/cmake/ament_add_test.cmake;125;add_test;/opt/ros/jazzy/share/ament_cmake_gtest/cmake/ament_add_gtest_test.cmake;95;ament_add_test;/opt/ros/jazzy/share/ament_cmake_gtest/cmake/ament_add_gtest.cmake;93;ament_add_gtest_test;/home/pi/pupperv3-monorepo/ros2_ws/src/common/rosx_introspection/CMakeLists.txt;87;ament_add_gtest;/home/pi/pupperv3-monorepo/ros2_ws/src/common/rosx_introspection/CMakeLists.txt;0;")
subdirs("gtest")
