# Pupper V3 Wheeled RL Policy Training Guide

This guide explains how to train a reinforcement learning (RL) policy for the Pupper V3 Wheeled robot using the MuJoCo MJX framework.

## Table of Contents
- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [Training Setup](#training-setup)
- [Training Configuration](#training-configuration)
- [Running Training](#running-training)
- [Deploying the Policy](#deploying-the-policy)
- [Troubleshooting](#troubleshooting)

---

## Overview

The Pupper V3 Wheeled robot uses a hybrid leg-wheel design with 12 actuated joints:
- **Joints 1-2, 4-5, 7-8, 10-11**: Position-controlled leg joints (8 total)
- **Joints 3, 6, 9, 12**: Velocity-controlled wheel joints (4 total)

The RL training uses:
- **Framework**: Brax PPO (Proximal Policy Optimization)
- **Physics**: MuJoCo MJX (GPU-accelerated)
- **Training Script**: [`ai/rl/pupper_mjx_rl_use_this.py`](file:///c:/Users/starg/wheeled_pupper/Tund_wheel_V3_Fork/pupperv3-monorepo-FromPupper/ai/rl/pupper_mjx_rl_use_this.py)

---

## Prerequisites

### 1. Hardware Requirements
- **GPU**: NVIDIA GPU with CUDA support (L4, A100, or H100 recommended)
  - L4: ~400,000 env steps/sec
  - A100: ~760,000 env steps/sec
  - H100: ~121,000 env steps/sec
- **RAM**: 16GB+ recommended
- **VRAM**: 8GB+ recommended

### 2. Software Requirements
- Python 3.8+
- CUDA 11.x or 12.x
- JAX with GPU support
- MuJoco 3.x
- Brax
- Weights & Biases (wandb) account

### 3. Installation

```bash
# Navigate to the RL directory
cd pupperv3-monorepo-FromPupper/ai/rl

# Install dependencies (if not already done)
pip install -r requirements.txt

# Install JAX with CUDA support (example for CUDA 12)
pip install --upgrade "jax[cuda12_pip]" -f https://storage.googleapis.com/jax-releases/jax_cuda_releases.html
```

---

## Training Setup

### 1. Authenticate with Weights & Biases

```bash
wandb login
```

Enter your API key when prompted. You can find this at https://wandb.ai/authorize

### 2. Configure Training Parameters

The main configuration is located in [`ai/rl/pupper_mjx_rl_use_this.py`](file:///c:/Users/starg/wheeled_pupper/Tund_wheel_V3_Fork/pupperv3-monorepo-FromPupper/ai/rl/pupper_mjx_rl_use_this.py).

Key sections to modify:

#### Environment Selection
```python
# Around line 52-60
env_name = "pupper_v3_wheeled"  # For wheeled robot
# env_name = "pupper_v3"  # For legged-only robot
```

#### Training Configuration
```python
class Training:
    ppo = PPO(
        num_timesteps=100_000_000,  # Total training steps
        num_evals=20,               # Number of evaluation checkpoints
        reward_scaling=10,          # Reward scaling factor
        episode_length=1000,        # Steps per episode
        normalize_observations=True,
        action_repeat=1,
        unroll_length=10,
        num_minibatches=32,
        num_updates_per_batch=8,
        discounting=0.95,
        learning_rate=3e-4,
        entropy_cost=1e-3,
        num_envs=4096,             # Parallel environments
        batch_size=2048,
        seed=0,
    )
```

#### Policy Architecture
```python
class Policy:
    hidden_layer_sizes = (256, 128, 128, 128)  # Neural network layer sizes
    activation = "elu"                          # Activation function
    action_scale = 0.75                        # Action scaling
    use_imu = True                             # Use IMU observations
    observation_history = 4                    # Observation history length
```

#### Joint Limits for Wheeled Robot
```python
class Simulation:
    joint_upper_limits = [
        1.51, 3.14, float('inf'),  # Front right (hip, knee, wheel)
        1.51, 3.14, float('inf'),  # Front left
        1.91, 3.14, float('inf'),  # Back right
        1.91, 3.14, float('inf'),  # Back left
    ]
    
    joint_lower_limits = [
        -2.22, -0.42, -float('inf'),  # Front right
        -2.22, -0.42, -float('inf'),  # Front left
        -1.82, -0.42, -float('inf'),  # Back right
        -1.82, -0.42, -float('inf'),  # Back left
    ]
```

> **Important**: The `inf` values for joints 3, 6, 9, and 12 indicate these are continuous rotation (wheel) joints.

#### Domain Randomization
```python
class Training:
    # Friction randomization
    friction_range = (0.6, 1.4)
    
    # PD controller gain randomization
    position_control_kp_multiplier_range = (0.9, 1.1)
    position_control_kd_multiplier_range = (0.9, 1.1)
    
    # Center of mass randomization
    body_com_x_shift_range = (0, 0)
    body_com_y_shift_range = (0, 0)
    body_com_z_shift_range = (0, 0)
    
    # Mass and inertia randomization
    body_mass_scale_range = (0.9, 1.1)
    body_inertia_scale_range = (0.9, 1.1)
```

---

## Training Configuration

### Key Hyperparameters Explained

| Parameter | Description | Wheeled Default | Legged Default |
|-----------|-------------|----------------|----------------|
| `num_timesteps` | Total training steps | 100M | 100M |
| `episode_length` | Steps per episode | 1000 | 1000 |
| `num_envs` | Parallel environments | 4096 | 4096 |
| `learning_rate` | PPO learning rate | 3e-4 | 3e-4 |
| `action_scale` | Policy output scaling | 0.75 | 0.75 |
| `kp` | Position control gain | 5.0 | 5.0 |
| `kd` | Damping gain | 0.25 | 0.25 |
| `observation_history` | History buffer length | 4 | 4 |

### Reward Function

The reward function is defined in the environment file. Key components:
1. **Tracking reward**: Encourages following velocity commands
2. **Torque penalty**: Penalizes high joint torques
3. **Energy penalty**: Penalizes high energy consumption
4. **Orientation penalty**: Keeps robot upright
5. **Joint limit penalty**: Keeps joints within safe ranges

---

## Running Training

### 1. Start Training

```bash
cd pupperv3-monorepo-FromPupper/ai/rl
python pupper_mjx_rl_use_this.py
```

### 2. Monitor Training

Training progress will be logged to:
- **Console**: Real-time statistics
- **Weights & Biases**: Detailed metrics at https://wandb.ai

Key metrics to monitor:
- `eval/episode_reward`: Episode reward (target: >40 for good performance)
- `training/policy_loss`: Policy network loss
- `training/value_loss`: Value network loss
- `training/entropy`: Policy entropy (exploration)

### 3. Training Outputs

Training creates an`output_<run_name>` folder containing:

```
output_<run_name>/
├── checkpoint_<step>.pkl          # Model checkpoints
├── policy_<run_name>_<step>.json  # Exported policies
└── policy_visualization_*.mp4     # Visualization videos
```

---

## Deploying the Policy

### 1. Locate the Trained Policy

After training completes, find the best policy:
```bash
ls -lh output_*/policy_*.json
```

Look for the file with the highest `max_reward` value, e.g.:
```
policy_updatedcolab_wheeled_harsher_max_reward_49.04.json
```

### 2. Copy Policy to ROS2 Package

```bash
# Copy the policy file
cp output_*/policy_*.json ros2_ws/src/neural_controller_incomplete/launch/

# Or use the download script on the robot
cd ros2_ws/src/neural_controller_incomplete
python download_latest_policy.py --run <run_id>
```

### 3. Update Configuration

Edit [`ros2_ws/src/neural_controller_incomplete/launch/config.yaml`](file:///c:/Users/starg/wheeled_pupper/Tund_wheel_V3_Fork/pupperv3-monorepo-FromPupper/ros2_ws/src/neural_controller_incomplete/launch/config.yaml):

#### For Wheeled Robot (Three-Legged Controller)
```yaml
neural_controller_three_legged:
  ros__parameters:
    model_path: "$(find-pkg-share neural_controller)/launch/policy_<your_new_policy>.json"
    
    # Action types: position for legs, velocity for wheels
    action_types:
      [
        "position",  # leg_front_r_1
        "position",  # leg_front_r_2
        "velocity",  # leg_front_r_3 (WHEEL)
        "position",  # leg_front_l_1
        "position",  # leg_front_l_2
        "velocity",  # leg_front_l_3 (WHEEL)
        "position",  # leg_back_r_1
        "position",  # leg_back_r_2
        "velocity",  # leg_back_r_3 (WHEEL)
        "position",  # leg_back_l_1
        "position",  # leg_back_l_2
        "velocity",  # leg_back_l_3 (WHEEL)
      ]
```

> **Critical**: The wheel joints (3, 6, 9, 12) MUST be set to `"velocity"` mode!

#### For Legged Robot (Regular Controller)
```yaml
neural_controller:
  ros__parameters:
    model_path: "$(find-pkg-share neural_controller)/launch/policy_<your_new_policy>.json"
    
    # All position control for legged robot
    action_types:
      [
        "position", "position", "position",  # Front right
        "position", "position", "position",  # Front left
        "position", "position", "position",  # Back right
        "position", "position", "position",  # Back left
      ]
```

### 4. Rebuild and Deploy

```bash
# Rebuild the ROS2 workspace
cd ros2_ws
colcon build --packages-select neural_controller

# Deploy to robot (if applicable)
scp -r install/<your_user>@<robot_ip>:~/pupperv3-monorepo/ros2_ws/
```

### 5. Test the Policy

```bash
# On the robot, launch the neural controller
ros2 launch neural_controller three_legged_controller.launch.py  # For wheeled
# OR
ros2 launch neural_controller neural_controller.launch.py        # For legged
```

---

## Troubleshooting

### Training Issues

#### GPU Out of Memory
**Solution**: Reduce `num_envs` or reduce network size:
```python
ppo = PPO(num_envs=2048)  # Reduced from 4096
```

#### Training Not Converging
**Solution**: 
1. Reduce `learning_rate`
2. Increase `reward_scaling`
3. Adjust domain randomization ranges
4. Check reward function in environment code

#### Very Slow Training
**Solution**:
1. Verify GPU is being used: `nvidia-smi`
2. Reduce environment complexity
3. Adjust `unroll_length` and `num_minibatches`

### Deployment Issues

#### Robot Falls Immediately
**Possible causes**:
1. **Wrong action types**: Wheels must be `"velocity"`, legs must be `"position"`
2. **Joint limits mismatch**: Check `default_joint_pos` in config
3. **Policy not compatible**: Ensure policy was trained for wheeled robot

**Solution**:
```bash
# Verify action types in config.yaml
grep -A 15 "action_types" ros2_ws/src/neural_controller_incomplete/launch/config.yaml
```

#### Wheels Not Spinning
**Cause**: Wheels are set to `"position"` instead of `"velocity"`

**Solution**: Change action types for joints 3, 6, 9, 12 to `"velocity"` in `config.yaml`

#### Robot Behavior Erratic
**Possible causes**:
1. IMU data issues
2. Policy observation mismatch
3. Wrong default joint positions

**Solution**:
1. Check IMU calibration
2. Verify `use_imu` matches between training and deployment
3. Check `default_joint_pos` in config

---

## Advanced Topics

### Curriculum Learning

You can progressively increase difficulty:

1. **Phase 1**: Flat ground only
```python
CONFIG.training.terrainclass = FlatTerrain
```

2. **Phase 2**: Add simple obstacles
```python
CONFIG.training.terrain_obstacle_num = 10
```

3. **Phase 3**: Complex terrain
```python
CONFIG.training.terrainclass = HField  # Height field terrain
```

### Restarting from Checkpoint

To continue training from a previous run:

```python
CONFIG.training.checkpoint_run_number = 123  # Your previous run ID

# This will download and resume from checkpoint
```

### Exporting for Different Platforms

The policy is exported in JSON format compatible with:
- RTNeural (C++ inference)
- ONNXRuntime
- Pure JAX

---

## References

- [Brax PPO Documentation](https://github.com/google/brax/blob/main/brax/training/agents/ppo/train.py)
- [MuJoco MJX](https://mujoco.readthedocs.io/en/stable/mjx.html)
- [Pupper V3 Environment Code](file:///c:/Users/starg/wheeled_pupper/Tund_wheel_V3_Fork/pupperv3-monorepo-FromPupper/ai/pupperv3_mjx/envs/pupper_v3_wheeled.py)

---

## Appendix: Policy File Format

Trained policy JSON structure:
```json
{
  "use_imu": true,
  "control_orientation": true,
  "observation_history": 4,
  "action_scale": 0.75,
  "kp": 5.0,
  "kd": 0.25,
  "default_joint_pos": [0.26, 0.0, -0.52, ...],
  "joint_upper_limits": [1.51, 3.14, inf, ...],
  "joint_lower_limits": [-2.22, -0.42, -inf, ...],
  "layers": [
    {
      "type": "dense",
      "activation": "elu",
      "shape": [null, 256],
      "weights": [[...], [...]]
    },
    ...
  ]
}
```

Key fields:
- `joint_*_limits`: `inf` indicates wheel joints
- `default_joint_pos`: Neutral leg positions
- `layers`: Neural network weights and architecture
