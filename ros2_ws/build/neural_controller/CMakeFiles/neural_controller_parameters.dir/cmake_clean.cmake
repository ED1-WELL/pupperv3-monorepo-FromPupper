file(REMOVE_RECURSE
  "include/neural_controller/neural_controller_parameters.hpp"
  "include/neural_controller_parameters.hpp"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/neural_controller_parameters.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
