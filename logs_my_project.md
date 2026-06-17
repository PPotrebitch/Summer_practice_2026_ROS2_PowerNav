# Бортовой журнал проекта

## Глава 0: Об системе.
1) OS: Linux Ubuntu 24.04.2 LTS

2) дистрибутив ROS 2: ROS 2 Jazzy


## Глава 1: ROS2 пакеты.

Я выбрал для проекты следующие пакеты:

###  Симуляция и окружение (Gazebo)

Эта группа обеспечивает связь ROS 2 с симулятором Gazebo Harmonic.

*   **`ros-jazzy-ros-gz`**: Мета-пакет, который устанавливает все необходимые компоненты для интеграции ROS 2 и Gazebo.
*   **`ros-jazzy-ros-gz-sim`**: Содержит удобные launch-файлы и утилиты для запуска Gazebo Sim из ROS 2.
*   **`ros-jazzy-ros-gz-bridge`**: Обеспечивает двунаправленный обмен данными (топики, сервисы, действия) между ROS 2 и Gazebo Transport.
*   **`ros-jazzy-ros-gz-image`**: Специализированный мост для передачи изображений из Gazebo в ROS 2 с использованием `image_transport`.

### Навигация и картографирование (Nav2)

Основа для автономного перемещения робота.

*   **`ros-jazzy-navigation2`**: Основной пакет со стеком навигации Nav2.
*   **`ros-jazzy-nav2-bringup`**: Содержит launch-файлы и конфигурации для быстрого запуска Nav2.
*   **`ros-jazzy-slam-toolbox`**: Инструмент для создания карт (SLAM) в режиме реального времени.

###  Управление роботом (ROS 2 Control)

Данные пакеты нужны для управления аппаратной частью  робота в симуляции.

*   **`ros-jazzy-ros2-control`**: Основной фреймворк для управления роботами в ROS 2.
*   **`ros-jazzy-ros2-controllers`**: Набор стандартных контроллеров, включая:
    *   **`diff_drive_controller`**: Контроллер для роботов с дифференциальным приводом.
    *   **`joint_state_broadcaster`**: Публикует состояние всех сочленений робота.
*   **`ros-jazzy-robot-state-publisher`**: Публикует состояние робота (TF-трансформы) на основе URDF.
*   **`ros-jazzy-xacro`**: Утилита для работы с макросами в URDF-файлах, упрощает описание робота.

### Поведенческие деревья (Behavior Trees)

Для реализации сложной логики, например, автоматической парковки.

*   **`ros-jazzy-py-trees-ros`**: Реализация поведенческих деревьев на Python, интегрированная с ROS 2.

###  Детектирование AprilTag (для расширенной стыковки)

Для точной навигации на финальном этапе парковки. 

*   **`ros-jazzy-apriltag-detector`**: Базовый пакет с интерфейсами для детекторов AprilTag.
*   **`ros-jazzy-apriltag-detector-mit`**: Плагин с реализацией детектора от MIT.

###  Вспомогательные и отладочные инструменты

*   **`ros-jazzy-rviz2`**: Основной инструмент для 3D-визуализации данных.
*   **`ros-jazzy-teleop-twist-keyboard`**: Позволяет управлять роботом с клавиатуры для ручного тестирования.
*   **`ros-jazzy-joy`**: Поддержка джойстика для управления роботом.

---

### Итоговая команда для установки

Можно установить все перечисленные пакеты одной командой:

```bash
sudo apt update
sudo apt install ros-jazzy-ros-gz ros-jazzy-ros-gz-sim ros-jazzy-ros-gz-bridge ros-jazzy-ros-gz-image \
ros-jazzy-navigation2 ros-jazzy-nav2-bringup ros-jazzy-slam-toolbox \
ros-jazzy-ros2-control ros-jazzy-ros2-controllers ros-jazzy-robot-state-publisher ros-jazzy-xacro \
ros-jazzy-py-trees-ros ros-jazzy-behaviortree-cpp-v3 \
ros-jazzy-apriltag-detector ros-jazzy-apriltag-detector-mit \
ros-jazzy-rviz2 ros-jazzy-teleop-twist-keyboard ros-jazzy-joy
```


## Глава 2: Создания новых пакетов

Для проект создадим новый пакет, который будет называться "power_nav_robot":

```bash
cd ~/ros2_ws/src
ros2 pkg create power_nav_robot --build-type ament_python --license Apache-2.0 --dependencies rclpy std_msgs geometry_msgs nav2_msgs sensor_msgs cv_bridge
```

Что мы добавили в зависимости:

- rclpy — для Python-нод.

- std_msgs, geometry_msgs — базовые сообщения.

- nav2_msgs — для работы с Nav2 (действия и сообщения).

- sensor_msgs — для работы с лидаром и камерой.

- cv_bridge — для обработки изображений (пригодится для AprilTag).


После создания пакета добавьте в него нужные директории:
```bash
cd ~/ros2_ws/src/power_nav_robot
mkdir -p launch config urdf worlds scripts rviz maps
```

Незабываем после каждого изменеия пересобирать:
```bash
cd ~/ros2_ws
colcon build
source install/setup.bash
```
