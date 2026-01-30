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



namespace neural_controller {

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
        int64_t repeat_action = 10;
        std::string model_path = "";
        bool use_imu = true;
        std::string imu_sensor_name = "imu_sensor";
        std::vector<std::string> joint_names = {};
        std::vector<double> default_joint_pos = {};
        std::vector<double> kps = {};
        std::vector<double> kds = {};
        double gain_multiplier = 1.0;
        std::vector<double> init_kps = {};
        std::vector<double> init_kds = {};
        double init_duration = 2.0;
        double fade_in_duration = 2.0;
        std::vector<double> action_scales = {};
        std::vector<std::string> action_types = {};
        std::vector<double> joint_lower_limits = {};
        std::vector<double> joint_upper_limits = {};
        int64_t observation_history = 1;
        double observation_limit = 100.0;
        double max_body_angle = 0.52;
        double estop_kd = 0.1;
        // for detecting if the parameter struct has been updated
        rclcpp::Time __stamp;
    };
    struct StackParams {
        int64_t repeat_action = 10;
        bool use_imu = true;
        double gain_multiplier = 1.0;
        double init_duration = 2.0;
        double fade_in_duration = 2.0;
        int64_t observation_history = 1;
        double observation_limit = 100.0;
        double max_body_angle = 0.52;
        double estop_kd = 0.1;
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
    : ParamListener(parameters_interface, rclcpp::get_logger("neural_controller"), prefix) {
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
      output.repeat_action = params.repeat_action;
      output.use_imu = params.use_imu;
      output.gain_multiplier = params.gain_multiplier;
      output.init_duration = params.init_duration;
      output.fade_in_duration = params.fade_in_duration;
      output.observation_history = params.observation_history;
      output.observation_limit = params.observation_limit;
      output.max_body_angle = params.max_body_angle;
      output.estop_kd = params.estop_kd;

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
        if (param.get_name() == (prefix_ + "repeat_action")) {
            updated_params.repeat_action = param.as_int();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "model_path")) {
            updated_params.model_path = param.as_string();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "use_imu")) {
            updated_params.use_imu = param.as_bool();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "imu_sensor_name")) {
            updated_params.imu_sensor_name = param.as_string();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
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
        if (param.get_name() == (prefix_ + "gain_multiplier")) {
            updated_params.gain_multiplier = param.as_double();
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
        if (param.get_name() == (prefix_ + "joint_lower_limits")) {
            updated_params.joint_lower_limits = param.as_double_array();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "joint_upper_limits")) {
            updated_params.joint_upper_limits = param.as_double_array();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "observation_history")) {
            updated_params.observation_history = param.as_int();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "observation_limit")) {
            updated_params.observation_limit = param.as_double();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "max_body_angle")) {
            updated_params.max_body_angle = param.as_double();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "estop_kd")) {
            updated_params.estop_kd = param.as_double();
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
      if (!parameters_interface_->has_parameter(prefix_ + "repeat_action")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Number of times to repeat the same action before querying the policy again";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.repeat_action);
          parameters_interface_->declare_parameter(prefix_ + "repeat_action", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "model_path")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Path to the model weights";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.model_path);
          parameters_interface_->declare_parameter(prefix_ + "model_path", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "use_imu")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Whether to use the IMU sensor";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.use_imu);
          parameters_interface_->declare_parameter(prefix_ + "use_imu", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "imu_sensor_name")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Name of the IMU sensor";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.imu_sensor_name);
          parameters_interface_->declare_parameter(prefix_ + "imu_sensor_name", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "joint_names")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Names of the joints to control, in the order expected by the policy observation/action spaces";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.joint_names);
          parameters_interface_->declare_parameter(prefix_ + "joint_names", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "default_joint_pos")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Default joint positions. Added to policy output to get command. Subtracted from read motor angles to get observation, e.g. commanded_motor_angles = scaled_action + default_joint_pos";
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
      if (!parameters_interface_->has_parameter(prefix_ + "gain_multiplier")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Multiplier for the position control gains";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.gain_multiplier);
          parameters_interface_->declare_parameter(prefix_ + "gain_multiplier", parameter, descriptor);
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
      if (!parameters_interface_->has_parameter(prefix_ + "joint_lower_limits")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Lower limits for the joint positions";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.joint_lower_limits);
          parameters_interface_->declare_parameter(prefix_ + "joint_lower_limits", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "joint_upper_limits")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Upper limits for the joint positions";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.joint_upper_limits);
          parameters_interface_->declare_parameter(prefix_ + "joint_upper_limits", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "observation_history")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Number of previous observations to include in the observation space";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.observation_history);
          parameters_interface_->declare_parameter(prefix_ + "observation_history", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "observation_limit")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Maximum observation value before scaling";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.observation_limit);
          parameters_interface_->declare_parameter(prefix_ + "observation_limit", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "max_body_angle")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Maximum body angle (rad) allowed before the controller is reset";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.max_body_angle);
          parameters_interface_->declare_parameter(prefix_ + "max_body_angle", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "estop_kd")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "Velocity gain for the joints during emergency stop";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.estop_kd);
          parameters_interface_->declare_parameter(prefix_ + "estop_kd", parameter, descriptor);
      }
      // get parameters and fill struct fields
      rclcpp::Parameter param;
      param = parameters_interface_->get_parameter(prefix_ + "repeat_action");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.repeat_action = param.as_int();
      param = parameters_interface_->get_parameter(prefix_ + "model_path");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.model_path = param.as_string();
      param = parameters_interface_->get_parameter(prefix_ + "use_imu");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.use_imu = param.as_bool();
      param = parameters_interface_->get_parameter(prefix_ + "imu_sensor_name");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.imu_sensor_name = param.as_string();
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
      param = parameters_interface_->get_parameter(prefix_ + "gain_multiplier");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.gain_multiplier = param.as_double();
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
      param = parameters_interface_->get_parameter(prefix_ + "joint_lower_limits");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.joint_lower_limits = param.as_double_array();
      param = parameters_interface_->get_parameter(prefix_ + "joint_upper_limits");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.joint_upper_limits = param.as_double_array();
      param = parameters_interface_->get_parameter(prefix_ + "observation_history");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.observation_history = param.as_int();
      param = parameters_interface_->get_parameter(prefix_ + "observation_limit");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.observation_limit = param.as_double();
      param = parameters_interface_->get_parameter(prefix_ + "max_body_angle");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.max_body_angle = param.as_double();
      param = parameters_interface_->get_parameter(prefix_ + "estop_kd");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.estop_kd = param.as_double();


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
      rclcpp::Logger logger_ = rclcpp::get_logger("neural_controller");
      std::mutex mutable mutex_;
  };

} // namespace neural_controller
