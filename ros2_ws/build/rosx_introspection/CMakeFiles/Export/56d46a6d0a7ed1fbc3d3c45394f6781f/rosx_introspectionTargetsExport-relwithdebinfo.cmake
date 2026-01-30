#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "rosx_introspection::rosx_introspection" for configuration "RelWithDebInfo"
set_property(TARGET rosx_introspection::rosx_introspection APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(rosx_introspection::rosx_introspection PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/librosx_introspection.a"
  )

list(APPEND _cmake_import_check_targets rosx_introspection::rosx_introspection )
list(APPEND _cmake_import_check_files_for_rosx_introspection::rosx_introspection "${_IMPORT_PREFIX}/lib/librosx_introspection.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
