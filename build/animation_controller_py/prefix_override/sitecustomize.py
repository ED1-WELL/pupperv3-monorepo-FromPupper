import sys
if sys.prefix == '/usr':
    sys.real_prefix = sys.prefix
    sys.prefix = sys.exec_prefix = '/home/pi/pupperv3-monorepo/ros2_ws/install/animation_controller_py'
