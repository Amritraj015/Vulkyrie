#pragma once

#include "vlkypch.h"
#include "events/enums/key_code.h"
#include "core/time_step.h"
#include "input/inputs.h"

namespace Vulkyrie {

    /** @brief Enumeration for camera movement directions. */
    enum CameraMovement {
        FORWARD,  // Move the camera forward.
        BACKWARD, // Move the camera backward.
        LEFT,     // Move the camera to the left.
        RIGHT,    // Move the camera to the right.
    };

    /** @brief Structure to hold camera settings and configuration. */
    struct CameraSettings {
    public:
        /** @brief The position of the camera in 3D space. */
        glm::vec3 Position = glm::vec3(0.0f, 0.0f, 10.0f);

        /** @brief The front direction vector of the camera. */
        glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);

        /** @brief The up direction vector of the camera. */
        glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

        /** @brief The zoom level (field of view) of the camera. */
        f32 Zoom = 45.0f;

        /** @brief The yaw (horizontal rotation) angle of the camera in degrees. */
        f32 Yaw = -90.0f;

        /** @brief The pitch (vertical rotation) angle of the camera in degrees. */
        f32 Pitch = 0.0f;

        /** @brief Key binding for moving the camera forward. */
        KeyCode MoveForwardKey = KeyCode::W;

        /** @brief Key binding for moving the camera backward. */
        KeyCode MoveBackwardKey = KeyCode::S;

        /** @brief Key binding for moving the camera to the left. */
        KeyCode MoveLeftKey = KeyCode::A;

        /** @brief Key binding for moving the camera to the right. */
        KeyCode MoveRightKey = KeyCode::D;

        /** @brief Key binding for increasing the camera movement speed. */
        KeyCode FastSpeedModifierKey = KeyCode::LeftShift;

        /** @brief Key binding for decreasing the camera movement speed. */
        KeyCode SlowSpeedModifierKey = KeyCode::LeftControl;

        /** @brief Look around sensitivity for camera rotation. */
        f32 Sensitivity = 0.1f;

        /** @brief Slow movement speed, used when the slow movement modifier key is pressed
         * along with another camera movement key. */
        f32 SlowMovementSpeed = 2.5f;

        /** @brief Regular movement speed, used when fast or slow movement modifiers
         * keys are not pressed along with another camera movement key. */
        f32 RegularMovementSpeed = 5.0f;

        /** @brief Fast movement speed, used when the fast movement modifier key is pressed
         * along with another camera movement key. */
        f32 FastMovementSpeed = 10.0f;

        // Make move-only
        CameraSettings() = default;
        CameraSettings(const CameraSettings &) = delete; // no copies
        CameraSettings(CameraSettings &&) = default;     // moves allowed
        CameraSettings &operator=(const CameraSettings &) = delete;
        CameraSettings &operator=(CameraSettings &&) = default;
    };

    /** @brief A class representing a camera for navigating a scene. */
    class Camera {
    public:
        // Factory method to handle defaults and move-only semantics
        static Camera Create(CameraSettings settings = makeDefaultCameraSettings()) {
            return Camera(std::move(settings));
        }

        /** @brief Sets the position of the camera in world space.
         * @param position The new position of the camera as a glm::vec3.
         */
        void SetPosition(const glm::vec3 &position) {
            _settings.Position = position;

            updateCameraVectors();
        }

        /** @brief Sets the movement speeds for the camera.
         * @param slowMovementSpeed The speed for slow movement.
         * @param regularMovementSpeed The speed for regular movement.
         * @param fastMovementSpeed The speed for fast movement.
         */
        void SetMovementSpeed(f32 slowMovementSpeed, f32 regularMovementSpeed, f32 fastMovementSpeed) {
            _settings.SlowMovementSpeed = slowMovementSpeed;
            _settings.RegularMovementSpeed = regularMovementSpeed;
            _settings.FastMovementSpeed = fastMovementSpeed;
        }

        /** @brief Sets a constant movement speed for all movement modes.
         * @param movementSpeed The speed to set for all movement modes.
         */
        void SetConstantMovementSpeed(f32 movementSpeed) {
            _settings.SlowMovementSpeed = _settings.RegularMovementSpeed = _settings.FastMovementSpeed = movementSpeed;
        }

        /** @brief Processes input received from any keyboard-like input system to move the camera's position.
         * @param direction The direction in which to move the camera.
         * @param deltaTime The time elapsed since the last frame. Used to ensure consistent movement speed regardless of frame rate.
         * @param movementSpeed The speed at which the camera moves. Default is 2.5 units per second.
         */
        VE_INLINE void ProcessKeyboardMovement(CameraMovement direction, f32 deltaTime, f32 movementSpeed = 2.5f) {
            f32 velocity = movementSpeed * deltaTime;

            switch (direction) {
                case FORWARD:
                    _settings.Position += _settings.Front * velocity;
                    break;
                case BACKWARD:
                    _settings.Position -= _settings.Front * velocity;
                    break;
                case LEFT:
                    _settings.Position -= _right * velocity;
                    break;
                case RIGHT:
                    _settings.Position += _right * velocity;
                    break;
            }
        }

        /** @brief Updates the camera's position and orientation based on input. Should be called every frame.
         * @param deltaTime The time elapsed since the last frame.
         */
        VE_INLINE void OnUpdate(Timestep deltaTime) {
            f32 dt = static_cast<f32>(deltaTime);
            f32 cameraSpeed = _settings.RegularMovementSpeed;

            if (IsKeyPressed(_settings.FastSpeedModifierKey)) {
                cameraSpeed = _settings.FastMovementSpeed;
            } else if (IsKeyPressed(_settings.SlowSpeedModifierKey)) {
                cameraSpeed = _settings.SlowMovementSpeed;
            }

            if (IsKeyPressed(_settings.MoveForwardKey)) ProcessKeyboardMovement(FORWARD, dt, cameraSpeed);
            if (IsKeyPressed(_settings.MoveBackwardKey)) ProcessKeyboardMovement(BACKWARD, dt, cameraSpeed);
            if (IsKeyPressed(_settings.MoveLeftKey)) ProcessKeyboardMovement(LEFT, dt, cameraSpeed);
            if (IsKeyPressed(_settings.MoveRightKey)) ProcessKeyboardMovement(RIGHT, dt, cameraSpeed);
        }

        /** @brief Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis.
         * @param scrollOffset The offset value of the scroll in the y direction. Positive values indicate scrolling up, negative values indicate scrolling
         * down.
         */
        VE_INLINE void ProcessMouseScroll(f32 scrollOffset) {
            _settings.Zoom -= scrollOffset;

            if (_settings.Zoom < 1.0f) _settings.Zoom = 1.0f;
            if (_settings.Zoom > 45.0f) _settings.Zoom = 45.0f;
        }

        /** @brief Returns the front vector of the camera. */
        [[nodiscard]] VE_INLINE glm::vec3 GetFront() const {
            return _settings.Front;
        }

        /** @brief Returns the zoom level (field of view) of the camera. */
        [[nodiscard]] VE_INLINE f32 GetZoom() const {
            return _settings.Zoom;
        }

        /** @brief Returns the current position of the camera in world space. */
        [[nodiscard]] VE_INLINE glm::vec3 GetPosition() const {
            return _settings.Position;
        }

        /** @brief Calculates and returns the view matrix using the camera's position and orientation.
         * @returns The view matrix as a glm::mat4.
         */
        [[nodiscard]] VE_INLINE glm::mat4 GetViewMatrix() const {
            return glm::lookAt(_settings.Position, _settings.Position + _settings.Front, _up);
        }

        /** @brief Processes input received from a mouse movement event to adjust the camera's yaw and pitch angles.
         * @param xOffset The offset value of the mouse movement in the x direction.
         * @param yOffset The offset value of the mouse movement in the y direction.
         * @param constrainPitch A boolean indicating whether to constrain the pitch angle to prevent screen flipping. Default is true.
         */
        VE_INLINE void ProcessMouseMovement(f32 xOffset, f32 yOffset, bool constrainPitch = true) {
            // initialize the last mouse positions on the first mouse movement event.
            if (_firstMouseMove) {
                _lastMouseX = xOffset;
                _lastMouseY = yOffset;
                _firstMouseMove = false;
            }

            // calculate the yaw and pitch offsets from the last mouse positions.
            _settings.Yaw += (xOffset - _lastMouseX) * _settings.Sensitivity;
            _settings.Pitch += (_lastMouseY - yOffset) * _settings.Sensitivity;

            // make sure that when pitch is out of bounds, screen doesn't get flipped
            if (constrainPitch) {
                if (_settings.Pitch > 89.0f) _settings.Pitch = 89.0f;
                if (_settings.Pitch < -89.0f) _settings.Pitch = -89.0f;
            }

            // update Front, Right and Up Vectors using the updated Euler angles
            updateCameraVectors();

            // update the last mouse positions.
            _lastMouseX = xOffset;
            _lastMouseY = yOffset;
        }

    private:
        /** @brief Constructs a Camera object with specified camera settings.
         * @param settings The settings for the camera.
         */
        Camera(CameraSettings &&settings)
            : _settings(std::move(settings))
            , _firstMouseMove(true) {
            updateCameraVectors();
        }

        /** @brief The settings for the camera. */
        CameraSettings _settings;

        /** @brief The right direction vector of the camera. */
        glm::vec3 _right;

        /** @brief The up direction vector of the camera. */
        glm::vec3 _up;

        /** @brief Flag to indicate if this is the first mouse movement event. */
        bool _firstMouseMove;

        /** @brief The last recorded mouse X position. */
        f32 _lastMouseX;

        /** @brief The last recorded mouse Y position. */
        f32 _lastMouseY;

        /** @brief Updates the camera's front, right, and up vectors based on the current yaw and pitch angles. */
        VE_INLINE void updateCameraVectors() {
            // calculate the new Front vector
            glm::vec3 front;
            front.x = cos(glm::radians(_settings.Yaw)) * cos(glm::radians(_settings.Pitch));
            front.y = sin(glm::radians(_settings.Pitch));
            front.z = sin(glm::radians(_settings.Yaw)) * cos(glm::radians(_settings.Pitch));
            _settings.Front = glm::normalize(front);

            // also re-calculate the Right and Up vector
            // normalize the vectors, because their length gets closer to 0 the more
            _right = glm::normalize(glm::cross(_settings.Front, _settings.WorldUp));
            _up = glm::normalize(glm::cross(_right, _settings.Front));
        }

        /** @brief Creates a CameraSettings object with default values.
         * @returns A CameraSettings object initialized with default values.
         */
        static CameraSettings makeDefaultCameraSettings() {
            return CameraSettings{}; // uses member defaults
        }
    };
} // namespace Vulkyrie
