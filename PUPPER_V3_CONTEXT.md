# Pupper V3 Wheeled Robot — Full Project Context

> **Purpose**: This document provides all the context needed to understand and work on the Pupper V3 Wheeled Robot project. Upload this to any new AI chat to give the agent the full picture.
> 
> **Key repos (relative to workspace root `Tund_wheel_V3_Fork/`):**
> - `pupperv3-monorepo-FromPupper/` — Main ROS2 monorepo for deployment on real robot
> - `description_Wheels_pupperv3/` — MuJoCo robot description with wheel geometry
> - `wheeled_pupper_collab/` — Google Colab notebooks for RL policy training

---

## 1. Project Overview

**Hardware**: Stanford Pupper V3 quadruped robot  
**Compute**: Raspberry Pi running Ubuntu 22.04 + ROS2 Humble  
**Control board**: Custom Pupper V3 control/power board communicating via CAN bus  
**Input**: PS5 DualSense controller connected via Bluetooth (`/dev/input/js0`)  
**Repo root**: `pupperv3-monorepo-FromPupper/`

### Current Goal
Transform the Pupper V3 from a walking quadruped into a **hybrid walking/driving robot**:
- **Walking mode**: Uses a trained RL walking policy (`policy_chocolate-shape-4_max_reward_24.37.json`)
- **Driving mode**: Uses `wheeled_pd_controller.py` — holds legs in a fixed pose and drives wheels via differential drive
- **Transition**: The lower legs use **heat memory polymers** that transform between leg shape and wheel shape
- **Next step**: Create scripts/policies for **lifting one leg at a time** to allow transition between walking and driving

### Transition Requirements
- **One leg at a time**: Lift one leg while the other 3 maintain static balance
- **~30 seconds per leg**: Each leg must be held in the lifted position for approximately 30 seconds while the heat memory polymer reshapes
- **Static balance**: The robot should statically balance on 3 legs (no active walking/shifting required)
- **No specific ordering**: Legs can be transformed in any order
- **Bi-directional**: Must support both walking→driving and driving→walking transitions

---

## 2. Repository Structure

```
pupperv3-monorepo-FromPupper/
├── ros2_ws/src/                          # Main ROS2 workspace
│   ├── neural_controller_incomplete/     # C++ ros2_control RL policy controller
│   ├── animation_controller_py/          # Python animation + wheeled PD controller
│   ├── control_board_hardware_interface/ # CAN bus hardware interface (ros2_control)
│   ├── pupper_v3_description/            # URDF/xacro robot description
│   ├── joy_utils/                        # E-stop & controller switching (C++)
│   ├── cmd_vel_mux/                      # Velocity command multiplexer (C++)
│   ├── bag_recorder/                     # ROS bag recording
│   ├── imu_to_tf/                        # IMU → TF transform
│   ├── hailo/                            # Hailo AI accelerator detection
│   ├── person_follower/                  # Person following behavior
│   ├── llm_websocket_server/             # LLM integration
│   ├── openai_bridge/                    # OpenAI API bridge
│   ├── pupper_feelings/                  # Emotion system
│   ├── pupperv3_mujoco_sim/              # MuJoCo simulation
│   └── real2sim_controller/              # Real-to-sim controller
├── robot/                                # System services & utilities
├── scripts/                              # Analysis & animation editor tools
├── ai/                                   # AI-related resources
├── docs/                                 # Documentation
└── bags/                                 # Recorded ROS bags
```

---

## 3. Joint Naming & Indexing

The robot has **12 joints** — 4 legs × 3 joints each. Joint 3 of each leg (`_3`) is the **lowest/distal joint** that serves as either a regular leg joint or a wheel.

| Index | Joint Name       | Leg      | Role       | CAN Ch | CAN ID |
|-------|------------------|----------|------------|--------|--------|
| 0     | `leg_front_r_1`  | Front-R  | Hip (ab/ad)| 2      | 1      |
| 1     | `leg_front_r_2`  | Front-R  | Upper leg  | 2      | 2      |
| 2     | `leg_front_r_3`  | Front-R  | Lower/Wheel| 2      | 3      |
| 3     | `leg_front_l_1`  | Front-L  | Hip (ab/ad)| 1      | 1      |
| 4     | `leg_front_l_2`  | Front-L  | Upper leg  | 1      | 2      |
| 5     | `leg_front_l_3`  | Front-L  | Lower/Wheel| 1      | 3      |
| 6     | `leg_back_r_1`   | Back-R   | Hip (ab/ad)| 4      | 1      |
| 7     | `leg_back_r_2`   | Back-R   | Upper leg  | 4      | 2      |
| 8     | `leg_back_r_3`   | Back-R   | Lower/Wheel| 4      | 3      |
| 9     | `leg_back_l_1`   | Back-L   | Hip (ab/ad)| 3      | 1      |
| 10    | `leg_back_l_2`   | Back-L   | Upper leg  | 3      | 2      |
| 11    | `leg_back_l_3`   | Back-L   | Lower/Wheel| 3      | 3      |

**Convenience groupings used in code:**
```python
LEG_INDICES = [0, 1, 3, 4, 6, 7, 9, 10]   # 8 position-controlled leg joints
WHEEL_INDICES = [2, 5, 8, 11]              # 4 velocity-controlled wheel joints
```

**Joint position limits (from `components.xacro`):**
- `_1` joints (hip): roughly [-1.12, 2.41] for right, [-2.41, 1.12] for left
- `_2` joints (upper): roughly [-0.32, 3.04] for right, [-3.04, 0.32] for left
- `_3` joints (lower/wheel): [-1000.0, 1000.0] — effectively unlimited for continuous rotation

Each joint has command interfaces: `position`, `velocity`, `effort`, `kp`, `kd`  
Each joint has state interfaces: `position`, `velocity`

---

## 4. Hardware Interface

**Package**: `control_board_hardware_interface`  
**Plugin**: `control_board_hardware_interface/ControlBoardHardwareInterface`

Communicates with actuators via CAN bus using the "cheetah" protocol. Each joint supports:
- **Command interfaces**: position, velocity, effort, kp, kd  
- **State interfaces**: position, velocity  

**IMU sensor** (`imu_sensor`): orientation (quaternion), angular velocity, linear acceleration  
- IMU mounting: yaw=0, pitch=-2.35619 rad, roll=0

---

## 5. Controller Architecture

### 5.1 Controller Manager (`ros2_control`)

Defined in `config.yaml`, runs at **520 Hz**. Available controllers:

| Controller | Type | Purpose |
|---|---|---|
| `neural_controller` | `neural_controller/NeuralController` | Walking RL policy (4-leg) |
| `neural_controller_three_legged` | `neural_controller/NeuralController` | Three-legged RL policy (wheels as velocity) |
| `forward_position_controller` | `ForwardCommandController` | Position commands (for animation/wheeled PD) |
| `forward_kp_controller` | `ForwardCommandController` | Kp gain commands |
| `forward_kd_controller` | `ForwardCommandController` | Kd gain commands |
| `forward_velocity_controller` | `ForwardCommandController` | Velocity commands (for wheels) |
| `joint_state_broadcaster` | `JointStateBroadcaster` | Publishes `/joint_states` at 260 Hz |
| `imu_sensor_broadcaster` | `IMUSensorBroadcaster` | Publishes IMU data at 260 Hz |

**Key constraint**: Only ONE set of controllers can own the command interfaces at a time. The `neural_controller` and the `forward_*_controllers` are **mutually exclusive**. Switching between them is done via the `controller_manager/switch_controller` service.

### 5.2 Neural Controller (C++ — `ros2_control` plugin)

**File**: `ros2_ws/src/neural_controller_incomplete/src/neural_controller.cpp`

- Loads a JSON policy file containing RTNeural weights  
- Runs RL policy inference in the ros2_control real-time loop  
- Action repeat: 10 (runs policy every 10th control loop iteration → ~52 Hz policy)  

**Observation vector** (per timestep, `kSingleObservationSize = 36`):
| Indices | Content |
|---|---|
| 0-2 | Base angular velocity (x, y, z) |
| 3-5 | Projected gravity vector (x, y, z) |
| 6-8 | Velocity commands (x, y, yaw) |
| 9-11 | Desired world z in body frame (orientation cmd) |
| 12-23 | Joint positions (relative to default) |
| 24-35 | Previous actions |

- Supports observation history (multiple timesteps stacked)  
- E-stop triggers on body angle exceeding `max_body_angle` (1.5 rad)  
- JSON policy files can embed: `kp`, `kd`, `action_scale`, `default_joint_pos`, `joint_lower_limits`, `joint_upper_limits`, `use_imu`, `observation_history`

**Walking policy config** (`neural_controller` in `config.yaml`):
```yaml
default_joint_pos: [0.26, 0.0, -0.52, -0.26, 0.0, 0.52, 0.26, 0.0, -0.52, -0.26, 0.0, 0.52]
init_kps: [7.5, 7.5, 7.5, 7.5, 7.5, 7.5, 7.5, 7.5, 7.5, 7.5, 7.5, 7.5]
init_kds: [0.25, 0.25, 0.25, 0.25, 0.25, 0.25, 0.25, 0.25, 0.25, 0.25, 0.25, 0.25]
action_types: all "position"
model_path: policy_chocolate-shape-4_max_reward_24.37.json
```

**Three-legged policy config** (`neural_controller_three_legged`):
```yaml
default_joint_pos: [0.26, 0.0, -0.52, -0.26, 0.0, 0.52, 0.26, 0.0, -0.52, -0.26, 0.0, 0.52]
init_kps: [7.5, 7.5, 0.0, 7.5, 7.5, 0.0, 7.5, 7.5, 0.0, 7.5, 7.5, 0.0]  # kp=0 for wheels
action_types: [position, position, velocity, position, position, velocity, ...]  # _3 joints are velocity
model_path: policy_no_normalization_max_reward_7.27.json
```

### 5.3 Wheeled PD Controller (Python)

**File**: `ros2_ws/src/animation_controller_py/animation_controller_py/wheeled_pd_controller.py`  
**Executable**: `wheeled_pd_controller` (registered in `setup.py`)

A standalone ROS2 node (NOT a ros2_control plugin) that:
1. **Holds legs** at a fixed standing pose using PD position control
2. **Controls wheels** using differential drive from `/cmd_vel` commands

**How it works:**
- Subscribes to `/cmd_vel` (Twist), `/joint_states`, `/emergency_stop`
- Publishes to forward command controllers: `/forward_position_controller/commands`, `/forward_kp_controller/commands`, `/forward_kd_controller/commands`, `/forward_velocity_controller/commands`
- **Requires** the `forward_*_controllers` to be active (not the `neural_controller`)

**Current tuned parameters:**
```yaml
default_joint_pos: [-0.75, 0.0, 0.0, 0.75, 0.0, 0.0, -0.75, 0.0, 0.0, 0.75, 0.0, 0.0]
leg_kp: 3.25
leg_kd: 0.5
wheel_kd: 0.5
wheel_separation: 0.167  # meters
wheel_radius: 0.04445    # meters
max_wheel_velocity: 10.0  # rad/s
control_rate: 50.0        # Hz
```

**Differential drive equations:**
```
v_left  = (linear_x - angular_z * wheel_separation / 2) / wheel_radius
v_right = (linear_x + angular_z * wheel_separation / 2) / wheel_radius
```

Wheel mapping: Right wheels = indices 2 (FR), 8 (BR); Left wheels = indices 5 (FL), 11 (BL)

### 5.4 Animation Controller (Python)

**File**: `ros2_ws/src/animation_controller_py/animation_controller_py/animation_controller.py`

Plays pre-recorded CSV animation keyframes through the forward command controllers:
- Loads CSV files from `launch/animations/` containing joint position keyframes
- Subscribes to `~/animation_select` (String topic) to trigger animations
- Smoothly interpolates from current pose → first keyframe → plays animation at configurable frame rate
- Handles controller switching: deactivates `neural_controller`/`neural_controller_three_legged`, activates `forward_*_controllers`

**Available animations** (CSV files):
- `lie_downward_dog`, `lie_sit_lie`, `push_up`, `sneeze`, `spider`
- `stand_downward_dog`, `stand_sit_shake_sit_stand`, `stand_sit_stand`, `swim`, `twerk`

Each CSV has columns matching joint names. Frame rate default: 40 Hz (1.33x speed).

---

## 6. MuJoCo Simulation Model

**File**: `description_Wheels_pupperv3/description/mujoco_xml/Wheel_pupper.xml`  
**Training repo**: `https://github.com/ED1-WELL/mjx_Wheels_pupperv3.git` (branch: `Ethan_test`)  
**Description repo**: `https://github.com/ED1-WELL/description_Wheels_pupperv3.git` (branch: `Ethan_test`)

### Key Differences from Real Robot Naming
In the MuJoCo model, the 3rd joint of each leg is named as a wheel (`Wheel_FR`, `Wheel_FL`, `Wheel_BR`, `Wheel_BL`) instead of `leg_*_3`. The mapping:

| MuJoCo Joint | Real Robot Joint | Index |
|---|---|---|
| `Wheel_FR` | `leg_front_r_3` | 2 |
| `Wheel_FL` | `leg_front_l_3` | 5 |
| `Wheel_BR` | `leg_back_r_3` | 8 |
| `Wheel_BL` | `leg_back_l_3` | 11 |

### Actuator Setup
```xml
<!-- Default: position control (PD) -->
<general forcerange="-3 3" gainprm="5.0 0 0" biasprm="0 -5.0 -0.1" ctrlrange="-3 3" />

<!-- Wheel class: velocity control -->
<default class="wheel">
    <joint limited="false" />
    <general ctrllimited="false" biasprm="0 0 -1.0"/>  <!-- F = kd * (ctrl - dq) -->
</default>

<!-- Wheel actuators have flipped gain for left vs right -->
<general joint="Wheel_FR" class="wheel" gainprm="1.0 0 0" />
<general joint="Wheel_FL" class="wheel" gainprm="-1.0 0 0" />  <!-- negated -->
<general joint="Wheel_BR" class="wheel" gainprm="1.0 0 0" />
<general joint="Wheel_BL" class="wheel" gainprm="-1.0 0 0" />  <!-- negated -->
```

### Physical Properties
- **Base link**: mass=1.506 kg, height=0.13m above ground
- **Leg segment 1** (`_1`): mass=0.18 kg each
- **Leg segment 2** (`_2`): mass=0.186 kg each
- **Wheel bodies**: mass=0.05 kg, cylinder r=0.04445m, height=0.013m (contact geom)
- **Simulation timestep**: 0.004s (250 Hz)
- **Friction**: 1.5 (collision geoms)
- **IMU site**: `body_imu_site` at pos=(0.09, 0, 0.032) relative to base_link
- **Home keyframe**: body at z=0.28, all joints at 0.0

### Leg Mounting Geometry
| Leg | Position (relative to base_link) | Euler Angles |
|---|---|---|
| Front-R | (0.075, -0.0835, 0) | (1.57, 0, 1.0) |
| Front-L | (0.075, 0.0835, 0) | (-1.57, 0, -1.0) |
| Back-R | (-0.075, -0.0725, 0) | (1.57, 0, 0.6) |
| Back-L | (-0.075, 0.0725, 0) | (-1.57, 0, -0.6) |

---

## 7. RL Training Pipeline

**Notebook**: `wheeled_pupper_collab/Updated_wheeled_pupper_Jan_2026.ipynb`  
**Colab link**: `https://colab.research.google.com/github/ED1-WELL/wheeled_pupper_collab/blob/main/Updated_wheeled_pupper_Jan_2026.ipynb`  
**Framework**: Brax 0.12.1 + MuJoCo MJX 3.2.7 + JAX 0.4.30 + Flax 0.10.2

### Training Config (PPO)
```python
num_timesteps = 300_000_000   # Set to 1B for better policy
episode_length = 500
num_envs = 8192
batch_size = 256
num_minibatches = 32
num_updates_per_batch = 4
learning_rate = 3.0e-4        # Use 3e-5 for >300M timesteps
discounting = 0.97
entropy_cost = 1e-2
unroll_length = 20
normalize_observations = True
environment_dt = 0.02         # 50 Hz control
```

### Policy Architecture
```python
hidden_layer_sizes = (256, 128, 128, 128)
activation = "elu"            # RTNeural supports: relu, tanh, sigmoid, elu, prelu
observation_history = 4       # Stacked observations
action_scale = 0.75           # Default for 4-leg walking
use_imu = True
```

### Default Pose (Training)
```python
default_pose = [0.26, 0.0, -0.52, -0.26, 0.0, 0.52, 0.26, 0.0, -0.52, -0.26, 0.0, 0.52]
```

### Command Ranges (configurable per training run)
```python
lin_vel_x_range = [-0.0, 0.0]    # Set to [-0.75, 0.75] for locomotion
lin_vel_y_range = [-0.0, 0.0]    # Set to [-0.5, 0.5] for locomotion
ang_vel_yaw_range = [-0.0, 0.0]  # Set to [-2.0, 2.0] for locomotion
zero_command_probability = 0.02
```
*Note: The example notebook had velocity ranges set to 0 for a "stand still" training run.*

### Reward Function (Active Scales)
| Reward Term | Scale | Description |
|---|---|---|
| `tracking_lin_vel` | +1.5 | Track commanded linear velocity |
| `tracking_ang_vel` | +0.8 | Track commanded yaw rate |
| `tracking_orientation` | +0.5 | Track desired body orientation |
| `lin_vel_z` | -0.1 | Penalize vertical body velocity |
| `ang_vel_xy` | -0.002 | Penalize roll/pitch rates |
| `orientation` | -0.0 | (disabled) Penalize non-zero roll/pitch |
| `torques` | -0.025 | L2 regularization of joint torques |
| `joint_acceleration` | -1e-6 | L2 regularization of joint accelerations |
| `action_rate` | -0.1 | Smooth actions: L1 on action changes |
| `feet_air_time` | +0.02 | Encourage long swing steps |
| `stand_still` | -0.25 | Joints at default pose when no command |
| `stand_still_joint_velocity` | -0.5 | Zero joint velocity when no command |
| `abduction_angle` | -0.01 | Keep legs from spreading |
| `termination` | -100.0 | Early termination penalty |
| `foot_slip` | -0.2 | Penalize foot slipping |
| `knee_collision` | -10.0 | Penalize knees hitting ground |
| `body_collision` | -0.5 | Penalize body hitting ground |
| `wheels_on_ground` | -150.0 | Penalize wheels lifting up |
| `tracking_sigma` | 0.25 | Tracking reward = exp(-error²/σ) |

### Domain Randomization
- **Kicks**: probability=0.1, velocity=0.10 m/s
- **Noise**: angular_velocity=0.1 rad/s, gravity=0.05, motor_angle=0.05 rad, last_action=0.01
- **Motor gains**: kp multiplier=(0.6, 1.1), kd multiplier=(0.8, 1.5)
- **Mass/inertia**: scale=(0.9, 1.3)
- **Friction**: range=(0.6, 1.4)
- **Action latency**: 0 steps (20%), 1 step (80%)
- **IMU latency**: 0 steps (50%), 1 step (50%)
- **CoM shift**: x=(-0.02, 0.03), y=(-0.005, 0.005), z=(-0.005, 0.005)

### Termination Conditions
- Body center z < 0.05m
- Body angle from vertical > 0.70 rad (~40°)

---

## 8. Available Policies (JSON)

Located in `ros2_ws/src/neural_controller_incomplete/launch/`:

| Policy File | Size | Notes |
|---|---|---|
| `policy_chocolate-shape-4_max_reward_24.37.json` | 2.2 MB | **Active walking policy** — primary 4-leg walking |
| `policy_no_normalization_max_reward_7.27.json` | 2.2 MB | Three-legged policy (velocity-controlled wheels) |
| `policy_Tund_max_reward_7.54.json` | 2.2 MB | Custom training run |
| `policy_updatedcolab_wheeled_harsher_max_reward_49.04.json` | 2.2 MB | Wheeled policy (harsher training) |
| `policy_wheeled_harsher.json` | 5.3 MB | Wheeled policy variant |
| `policy_wheels_legs_V1.json` | 5.4 MB | Hybrid walking+wheels V1 |
| `policy_silver-brook-144_max_reward_17.98.json` | 2.2 MB | Walking variant |
| `policy_ancient-surf-216_max_reward_20.27.json` | 38.9 MB | Large policy |
| `policy_latest.json` | 38.9 MB | Latest policy |
| `policy.json` | 1.3 MB | Default/base policy |

---

## 9. PS5 Controller Mapping & Mode Switching

### Joy Utils / E-Stop Controller

**File**: `ros2_ws/src/joy_utils/src/estop_controller.cpp`

Subscribes to `/joy` and handles:
- **E-stop** (button 12 = PS button): Publishes to `/emergency_stop`, deactivates ALL controllers
- **E-stop release** (button 9 = Options): Re-activates the last active controller
- **Mode switching** (buttons 0, 1, 2 = ×, ○, △ on PS5):
  - Button 0 (×): Activate `neural_controller` (walking)
  - Button 1 (○): Activate `neural_controller_three_legged`
  - Button 2 (△): Activate forward controllers (for `wheeled_pd_controller` / animations)

**Config** (`config.yaml`):
```yaml
joy_util_node:
  controller_names: ["neural_controller", "neural_controller_three_legged",
                      "forward_kp_controller", "forward_kd_controller", "forward_position_controller"]
  switch_button_indices: [0, 1, 2]  # ×, ○, △
  estop_index: 12      # PS button
  estop_release_index: 9  # Options
```

### Teleop / Joystick

**Driver**: `joy_linux` node reading `/dev/input/js0`  
**Teleop node**: `teleop_twist_joy` publishes to `/teleop_cmd_vel`  
- Left stick Y (axis 1) → `linear.x` (scale: 0.75 m/s)
- Left stick X (axis 0) → `linear.y` (scale: 0.5 m/s)
- Right stick X (axis 3) → `angular.yaw` (scale: 2.0 rad/s)
- `require_enable_button: false` (no trigger needed)

### Cmd Vel Mux

**File**: `ros2_ws/src/cmd_vel_mux/src/cmd_vel_mux_node.cpp`

Priority-based multiplexer that selects the highest-priority active velocity source:
1. `/teleop_cmd_vel` (highest priority, requires deadband > 0.05)
2. `/llm_cmd_vel`
3. `/person_following_cmd_vel`

Output: `/cmd_vel` — consumed by both neural controllers and wheeled PD controller.

---

## 10. Launch System

**Main launch file**: `ros2_ws/src/neural_controller_incomplete/launch/launch.py`

```bash
# Real robot:
ros2 launch neural_controller launch.py

# Simulation:
ros2 launch neural_controller launch.py sim:=True

# Disable teleop:
ros2 launch neural_controller launch.py teleop:=False
```

Launches all nodes: controller manager, all controller spawners, joy, teleop, cmd_vel_mux, animation controller, wheeled PD controller, foxglove bridge, camera, bag recorder, IMU, hailo detection, person follower.

Neural controllers start **inactive**. They are activated by pressing × or ○ on the PS5 controller.

---

## 11. ROS2 Topics (Key)

| Topic | Type | Publisher | Subscriber |
|---|---|---|---|
| `/cmd_vel` | `Twist` | `cmd_vel_mux` | `neural_controller`, `wheeled_pd_controller` |
| `/teleop_cmd_vel` | `Twist` | `teleop_twist_joy` | `cmd_vel_mux` |
| `/joint_states` | `JointState` | `joint_state_broadcaster` | `wheeled_pd_controller` |
| `/joint_states_throttled` | `JointState` | throttler (10 Hz) | `animation_controller_py` |
| `/emergency_stop` | `Empty` | `estop_controller` | controllers |
| `/joy` | `Joy` | `joy_linux` | `estop_controller` |
| `/forward_position_controller/commands` | `Float64MultiArray` | animation/wheeled PD | `forward_position_controller` |
| `/forward_kp_controller/commands` | `Float64MultiArray` | animation/wheeled PD | `forward_kp_controller` |
| `/forward_kd_controller/commands` | `Float64MultiArray` | animation/wheeled PD | `forward_kd_controller` |
| `/forward_velocity_controller/commands` | `Float64MultiArray` | wheeled PD | `forward_velocity_controller` |
| `~/animation_select` | `String` | user | `animation_controller_py` |

---

## 12. How to Write New Controllers / Scripts

### Pattern: Python Node Using Forward Controllers

The `wheeled_pd_controller.py` establishes the pattern for creating new Python-based controllers:

1. **Create a ROS2 node** that publishes to the 4 forward command controller topics
2. **Subscribe** to `/joint_states` for current positions and `/cmd_vel` for user input
3. **Publish** 12-element `Float64MultiArray` messages to:
   - `/forward_position_controller/commands` — target positions
   - `/forward_velocity_controller/commands` — target velocities
   - `/forward_kp_controller/commands` — position gains per joint
   - `/forward_kd_controller/commands` — velocity gains per joint
4. **For position-controlled joints**: set `kp > 0`, `kd > 0`, and a target position
5. **For velocity-controlled joints** (wheels): set `kp = 0`, `kd > 0`, and a target velocity
6. **Register** the new executable in `setup.py` under `console_scripts`
7. **Add** a node definition in `launch.py`
8. **Add** parameters in `config.yaml`

### Key Gains Reference
- Walking: kp=7.5, kd=0.25 (all joints position-controlled)
- Wheeled standing: leg_kp=3.25, leg_kd=0.5, wheel_kd=0.5
- E-stop damping: kd=0.1 (all joints)

### Initialization Pattern
All controllers follow a smooth initialization:
1. Read current joint positions from `/joint_states`
2. Over `init_duration` seconds, linearly interpolate from current → target standing pose
3. After initialization, begin normal control

### Creating Animation CSVs
Animation CSVs must have:
- Header row with joint names as column headers (all 12 joints)
- Each row is one keyframe of joint positions (radians)
- Played back at configurable frame rate (default 40 Hz)

---

## 13. Current Working State & Known Issues

### What Works
- ✅ Walking with `policy_chocolate-shape-4_max_reward_24.37.json` (press × on PS5)
- ✅ Driving with `wheeled_pd_controller.py` (press △ on PS5, then use sticks)
- ✅ E-stop and mode switching via PS5 controller
- ✅ Animations via the animation controller

### What's Needed Next
- 🔲 **Leg lifting scripts**: Lift one leg at a time while the other 3 statically hold position for ~30 seconds, so the heat memory polymer lower leg can reshape
- 🔲 **Transition sequence**: Coordinated sequence to go from walking pose → lift each leg (hold ~30s each) → transform → driving pose (and reverse)
- 🔲 **Static three-legged balance**: No active walking/weight-shifting needed — just PD-hold the 3 grounded legs while the 4th is lifted
- 🔲 **Possible: New RL policies** for three-legged balancing (if static balance proves insufficient)

### Important Technical Notes
- The `_3` joints (indices 2, 5, 8, 11) serve dual purpose: position-controlled for walking, velocity-controlled for driving
- When switching between neural controller and forward controllers, only one set can be active
- The `neural_controller_three_legged` config already demonstrates how to mix position and velocity action types
- Default walking pose: `[0.26, 0.0, -0.52, -0.26, 0.0, 0.52, 0.26, 0.0, -0.52, -0.26, 0.0, 0.52]`
- Default wheeled standing pose: `[-0.75, 0.0, 0.0, 0.75, 0.0, 0.0, -0.75, 0.0, 0.0, 0.75, 0.0, 0.0]`

---

## 14. File Quick Reference

| Purpose | File Path |
|---|---|
| Main config | `ros2_ws/src/neural_controller_incomplete/launch/config.yaml` |
| Launch file | `ros2_ws/src/neural_controller_incomplete/launch/launch.py` |
| Walking policy | `ros2_ws/src/neural_controller_incomplete/launch/policy_chocolate-shape-4_max_reward_24.37.json` |
| Neural controller (C++) | `ros2_ws/src/neural_controller_incomplete/src/neural_controller.cpp` |
| Neural controller header | `ros2_ws/src/neural_controller_incomplete/include/neural_controller/neural_controller.hpp` |
| Neural controller params | `ros2_ws/src/neural_controller_incomplete/src/neural_controller_parameters.yaml` |
| Wheeled PD controller | `ros2_ws/src/animation_controller_py/animation_controller_py/wheeled_pd_controller.py` |
| Animation controller | `ros2_ws/src/animation_controller_py/animation_controller_py/animation_controller.py` |
| Setup.py (entry points) | `ros2_ws/src/animation_controller_py/setup.py` |
| E-stop/mode switch | `ros2_ws/src/joy_utils/src/estop_controller.cpp` |
| Cmd vel mux | `ros2_ws/src/cmd_vel_mux/src/cmd_vel_mux_node.cpp` |
| URDF xacro | `ros2_ws/src/pupper_v3_description/description/pupper_v3.urdf.xacro` |
| Joint hardware config | `ros2_ws/src/pupper_v3_description/description/components.xacro` |
| HW interface readme | `ros2_ws/src/control_board_hardware_interface/README.md` |
| Animation CSVs | `ros2_ws/src/animation_controller_py/launch/animations/*.csv` |
| MuJoCo wheeled model | `description_Wheels_pupperv3/description/mujoco_xml/Wheel_pupper.xml` |
| Training notebook | `wheeled_pupper_collab/Updated_wheeled_pupper_Jan_2026.ipynb` |
| MJX training code | `https://github.com/ED1-WELL/mjx_Wheels_pupperv3.git` (branch: `Ethan_test`) |
