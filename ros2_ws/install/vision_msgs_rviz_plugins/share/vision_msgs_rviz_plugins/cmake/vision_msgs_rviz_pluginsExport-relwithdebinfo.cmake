#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "vision_msgs_rviz_plugins::vision_msgs_rviz_plugins" for configuration "RelWithDebInfo"
set_property(TARGET vision_msgs_rviz_plugins::vision_msgs_rviz_plugins APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(vision_msgs_rviz_plugins::vision_msgs_rviz_plugins PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libvision_msgs_rviz_plugins.so"
  IMPORTED_SONAME_RELWITHDEBINFO "libvision_msgs_rviz_plugins.so"
  )

list(APPEND _cmake_import_check_targets vision_msgs_rviz_plugins::vision_msgs_rviz_plugins )
list(APPEND _cmake_import_check_files_for_vision_msgs_rviz_plugins::vision_msgs_rviz_plugins "${_IMPORT_PREFIX}/lib/libvision_msgs_rviz_plugins.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
