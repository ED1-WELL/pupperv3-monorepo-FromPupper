#pragma message("#include \"real2sim_controller_parameters.hpp\" is deprecated. Use #include <real2sim_controller/real2sim_controller_parameters.hpp> instead.")
// auto-generated DO NOT EDIT

#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <mutex>
#include <rclcpp/node.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <rclcpp/logger.hpp>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <parameter_traits/parameter_traits.hpp>

#include <rsl/static_string.hpp>
#include <rsl/static_vector.hpp>
#include <rsl/parameter_validators.hpp>



namespace real2sim_controller {

// Use validators from RSL
using rsl::unique;
using rsl::subset_of;
using rsl::fixed_size;
using rsl::size_gt;
using rsl::size_lt;
using rsl::not_empty;
using rsl::element_bounds;
using rsl::lower_element_bounds;
using rsl::upper_element_bounds;
using rsl::bounds;
using rsl::lt;
using rsl::gt;
using rsl::lt_eq;
using rsl::gt_eq;
using rsl::one_of;
using rsl::to_parameter_result_msg;

// temporarily needed for backwards compatibility for custom validators
using namespace parameter_traits;

template <typename T>
[[nodiscard]] auto to_parameter_value(T value) {
    return rclcpp::ParameterValue(value);
}

template <size_t capacity>
[[nodiscard]] auto to_parameter_value(rsl::StaticString<capacity> const& value) {
    return rclcpp::ParameterValue(rsl::to_string(value));
}

template <typename T, size_t capacity>
[[nodiscard]] auto to_parameter_value(rsl::StaticVector<T, capacity> const& value) {
    return rclcpp::ParameterValue(rsl::to_vector(value));
}
    struct Params {
        std::vector<std::string> joint_names = {};
        std::vector<double> default_joint_pos = {};
        std::vector<double> kps = {};
        std::vector<double> kds = {};
        std::vector<double> init_kps = {};
        std::vector<double> init_kds = {};
        double init_duration = 2.0;
        double fade_in_duration = 2.0;
        std::vector<double> action_scales = {};
        std::vector<std::string> action_types = {};
        double action_limit = 1.0;
        double test_frequency = 1.0;
        double test_amplitude = 0.0;
        int64_t test_motor_index = 2;
        bool do_sweep = false;
        double sweep_min_frequency = 1.0;
        double sweep_max_frequency = 20.0;
        int64_t sweep_frequency_step = 1;
        double sweep_secs_per_frequency = 1.0;
        bool square_wave = false;
        std::string save_filename = "real2sim_controller_data.csv";
        // for detecting if the parameter struct has been updated
        rclcpp::Time __stamp;
    };
    struct StackParams {
        double init_duration = 2.0;
        double fade_in_duration = 2.0;
        double action_limit = 1.0;
        double test_frequency = 1.0;
        double test_amplitude = 0.0;
        int64_t test_motor_index = 2;
        bool do_sweep = false;
        double sweep_min_frequency = 1.0;
        double sweep_max_frequency = 20.0;
        int64_t sweep_frequency_step = 1;
        double sweep_secs_per_frequency = 1.0;
        bool square_wave = false;
    };

  class ParamListener{
  public:
    // throws rclcpp::exceptions::InvalidParameterValueException on initialization if invalid parameter are loaded
    ParamListener(rclcpp::Node::SharedPtr node, std::string const& prefix = "")
    : ParamListener(node->get_node_parameters_interface(), node->get_logger(), prefix) {}

    ParamListener(rclcpp_lifecycle::LifecycleNode::SharedPtr node, std::string const& prefix = "")
    : ParamListener(node->get_node_parameters_interface(), node->get_logger(), prefix) {}

    ParamListener(const std::shared_ptr<rclcpp::node_interfaces::NodeParametersInterface>& parameters_interface,
                  std::string const& prefix = "")
    : ParamListener(parameters_interface, rclcpp::get_logger("real2sim_controller"), prefix) {
      RCLCPP_DEBUG(logger_, "ParameterListener: Not using node logger, recommend using other constructors to use a node logger");
    }

    ParamListener(const std::shared_ptr<rclcpp::node_interfaces::NodeParametersInterface>& parameters_interface,
                  rclcpp::Logger logger, std::string const& prefix = "") {
      logger_ = std::move(logger);
      prefix_ = prefix;
      if (!prefix_.empty() && prefix_.back() != '.') {
        prefix_ += ".";
      }

      parameters_interface_ = parameters_interface;
      declare_params();
      auto update_param_cb = [this](const std::vector<rclcpp::Parameter> &parameters){return this->update(parameters);};
      handle_ = parameters_interface_->add_on_set_parameters_callback(update_param_cb);
      clock_ = rclcpp::Clock();
    }

    Params get_params() const{
      std::lock_guard<std::mutex> lock(mutex_);
      return params_;
    }

    bool try_get_params(Params & params_in) const {
      if (mutex_.try_lock()) {
        if (const bool is_old = params_in.__stamp != params_.__stamp; is_old) {
          params_in = params_;
        }
        mutex_.unlock();
        return true;
      }
      return false;
    }

    bool is_old(Params const& other) const {
      std::lock_guard<std::mutex> lock(mutex_);
      return params_.__stamp != other.__stamp;
    }

    StackParams get_stack_params() {
      Params params = get_params();
      StackParams output;
      output.init_duration = params.init_duration;
      output.fade_in_duration = params.fade_in_duration;
      output.action_limit = params.action_limit;
      output.test_frequency = params.test_frequency;
      output.test_amplitude = params.test_amplitude;
      output.test_motor_index = params.test_motor_index;
      output.do_sweep = params.do_sweep;
      output.sweep_min_frequency = params.sweep_min_frequency;
      output.sweep_max_frequency = params.sweep_max_frequency;
      output.sweep_frequency_step = params.sweep_frequency_step;
      output.sweep_secs_per_frequency = params.sweep_secs_per_frequency;
      output.square_wave = params.square_wave;

      return output;
    }

    void refresh_dynamic_parameters() {
      auto updated_params = get_params();
      // TODO remove any destroyed dynamic parameters

      // declare any new dynamic parameters
      rclcpp::Parameter param;

    }

    rcl_interfaces::msg::SetParametersResult update(const std::vector<rclcpp::Parameter> &parameters) {
      auto updated_params = get_params();

      for (const auto &param: parameters) {
        if (param.get_name() == (prefix_ + "joint_names")) {
            updated_params.joint_names = param.as_string_array();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "default_joint_pos")) {
            updated_params.default_joint_pos = param.as_double_array();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "kps")) {
            updated_params.kps = param.as_double_array();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "kds")) {
            updated_params.kds = param.as_double_array();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "init_kps")) {
            updated_params.init_kps = param.as_double_array();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "init_kds")) {
            updated_params.init_kds = param.as_double_array();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "init_duration")) {
            updated_params.init_duration = param.as_double();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "fade_in_duration")) {
            updated_params.fade_in_duration = param.as_double();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "action_scales")) {
            updated_params.action_scales = param.as_double_array();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "action_types")) {
            updated_params.action_types = param.as_string_array();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "action_limit")) {
            updated_params.action_limit = param.as_double();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "test_frequency")) {
            updated_params.test_frequency = param.as_double();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "test_amplitude")) {
            updated_params.test_amplitude = param.as_double();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "test_motor_index")) {
            updated_params.test_motor_index = param.as_int();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "do_sweep")) {
            updated_params.do_sweep = param.as_bool();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "sweep_min_frequency")) {
            updated_params.sweep_min_frequency = param.as_double();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "sweep_max_frequency")) {
            updated_params.sweep_max_frequency = param.as_double();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "sweep_frequency_step")) {
            updated_params.sweep_frequency_step = param.as_int();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "sweep_secs_per_frequency")) {
            updated_params.sweep_secs_per_frequency = param.as_double();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "square_wave")) {
            updated_params.square_wave = param.as_bool();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "save_filename")) {
            updated_params.save_filename = param.as_string();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
      }

      updated_params.__stamp = clock_.now();
      update_internal_params(updated_params);
      return rsl::to_parameter_result_msg({});
    }

    void declare_params(){
      auto updated_params = get_params();
      // declare all parameters and give default values to non-required ones
      if (!parameters_interface_->has_parameter(prefix_ + "joint_names")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Names of the joints to control, in the order expected by the policy observation/action spaces";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.joint_names);
          parameters_interface_->declare_parameter(prefix_ + "joint_names", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "default_joint_pos")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Default joint positions to use when the controller is reset";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.default_joint_pos);
          parameters_interface_->declare_parameter(prefix_ + "default_joint_pos", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "kps")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Position gains for the joints";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.kps);
          parameters_interface_->declare_parameter(prefix_ + "kps", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "kds")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Velocity gains for the joints";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.kds);
          parameters_interface_->declare_parameter(prefix_ + "kds", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "init_kps")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Position gains for the joints during initialization (return to default position)";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.init_kps);
          parameters_interface_->declare_parameter(prefix_ + "init_kps", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "init_kds")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Velocity gains for the joints during initialization (return to default position)";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.init_kds);
          parameters_interface_->declare_parameter(prefix_ + "init_kds", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "init_duration")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Time to return to the default position on startup";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.init_duration);
          parameters_interface_->declare_parameter(prefix_ + "init_duration", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "fade_in_duration")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Time to gradually fade in the policy actions on startup";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.fade_in_duration);
          parameters_interface_->declare_parameter(prefix_ + "fade_in_duration", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "action_scales")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Scaling factor for the actions";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.action_scales);
          parameters_interface_->declare_parameter(prefix_ + "action_scales", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "action_types")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Type of action for each joint: position or velocity";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.action_types);
          parameters_interface_->declare_parameter(prefix_ + "action_types", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "action_limit")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Maximum action value before scaling";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.action_limit);
          parameters_interface_->declare_parameter(prefix_ + "action_limit", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "test_frequency")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Frequency of test motion in Hz";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.test_frequency);
          parameters_interface_->declare_parameter(prefix_ + "test_frequency", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "test_amplitude")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Amplitude of test motion in radians";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.test_amplitude);
          parameters_interface_->declare_parameter(prefix_ + "test_amplitude", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "test_motor_index")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Index of motor to use for test";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.test_motor_index);
          parameters_interface_->declare_parameter(prefix_ + "test_motor_index", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "do_sweep")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "whether to do frequency sweep";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.do_sweep);
          parameters_interface_->declare_parameter(prefix_ + "do_sweep", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "sweep_min_frequency")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Min frequency in Hz";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.sweep_min_frequency);
          parameters_interface_->declare_parameter(prefix_ + "sweep_min_frequency", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "sweep_max_frequency")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Max frequency in Hz";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.sweep_max_frequency);
          parameters_interface_->declare_parameter(prefix_ + "sweep_max_frequency", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "sweep_frequency_step")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Frequency step in Hz";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.sweep_frequency_step);
          parameters_interface_->declare_parameter(prefix_ + "sweep_frequency_step", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "sweep_secs_per_frequency")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Seconds at each frequency";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.sweep_secs_per_frequency);
          parameters_interface_->declare_parameter(prefix_ + "sweep_secs_per_frequency", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "square_wave")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Whether to use square wave rather than sinusoid";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.square_wave);
          parameters_interface_->declare_parameter(prefix_ + "square_wave", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "save_filename")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Name of file to which to write data";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.save_filename);
          parameters_interface_->declare_parameter(prefix_ + "save_filename", parameter, descriptor);
      }
      // get parameters and fill struct fields
      rclcpp::Parameter param;
      param = parameters_interface_->get_parameter(prefix_ + "joint_names");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.joint_names = param.as_string_array();
      param = parameters_interface_->get_parameter(prefix_ + "default_joint_pos");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.default_joint_pos = param.as_double_array();
      param = parameters_interface_->get_parameter(prefix_ + "kps");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.kps = param.as_double_array();
      param = parameters_interface_->get_parameter(prefix_ + "kds");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.kds = param.as_double_array();
      param = parameters_interface_->get_parameter(prefix_ + "init_kps");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.init_kps = param.as_double_array();
      param = parameters_interface_->get_parameter(prefix_ + "init_kds");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.init_kds = param.as_double_array();
      param = parameters_interface_->get_parameter(prefix_ + "init_duration");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.init_duration = param.as_double();
      param = parameters_interface_->get_parameter(prefix_ + "fade_in_duration");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.fade_in_duration = param.as_double();
      param = parameters_interface_->get_parameter(prefix_ + "action_scales");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.action_scales = param.as_double_array();
      param = parameters_interface_->get_parameter(prefix_ + "action_types");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.action_types = param.as_string_array();
      param = parameters_interface_->get_parameter(prefix_ + "action_limit");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.action_limit = param.as_double();
      param = parameters_interface_->get_parameter(prefix_ + "test_frequency");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.test_frequency = param.as_double();
      param = parameters_interface_->get_parameter(prefix_ + "test_amplitude");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.test_amplitude = param.as_double();
      param = parameters_interface_->get_parameter(prefix_ + "test_motor_index");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.test_motor_index = param.as_int();
      param = parameters_interface_->get_parameter(prefix_ + "do_sweep");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.do_sweep = param.as_bool();
      param = parameters_interface_->get_parameter(prefix_ + "sweep_min_frequency");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.sweep_min_frequency = param.as_double();
      param = parameters_interface_->get_parameter(prefix_ + "sweep_max_frequency");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.sweep_max_frequency = param.as_double();
      param = parameters_interface_->get_parameter(prefix_ + "sweep_frequency_step");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.sweep_frequency_step = param.as_int();
      param = parameters_interface_->get_parameter(prefix_ + "sweep_secs_per_frequency");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.sweep_secs_per_frequency = param.as_double();
      param = parameters_interface_->get_parameter(prefix_ + "square_wave");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.square_wave = param.as_bool();
      param = parameters_interface_->get_parameter(prefix_ + "save_filename");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.save_filename = param.as_string();


      updated_params.__stamp = clock_.now();
      update_internal_params(updated_params);
    }

    private:
      void update_internal_params(Params updated_params) {
        std::lock_guard<std::mutex> lock(mutex_);
        params_ = std::move(updated_params);
      }

      std::string prefix_;
      Params params_;
      rclcpp::Clock clock_;
      std::shared_ptr<rclcpp::node_interfaces::OnSetParametersCallbackHandle> handle_;
      std::shared_ptr<rclcpp::node_interfaces::NodeParametersInterface> parameters_interface_;

      // rclcpp::Logger cannot be default-constructed
      // so we must provide a initialization here even though
      // every one of our constructors initializes logger_
      rclcpp::Logger logger_ = rclcpp::get_logger("real2sim_controller");
      std::mutex mutable mutex_;
  };

} // namespace real2sim_controller
