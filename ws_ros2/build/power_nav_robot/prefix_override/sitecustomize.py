import sys
if sys.prefix == '/usr':
    sys.real_prefix = sys.prefix
    sys.prefix = sys.exec_prefix = '/home/pate/Рабочий стол/Practice_PP/ws_ros2/install/power_nav_robot'
