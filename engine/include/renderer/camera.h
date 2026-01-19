#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Renderer {

    /** @brief Enumeration for camera movement directions. */
    enum CameraMovement {
        FORWARD,  // Move the camera forward.
        BACKWARD, // Move the camera backward.
        LEFT,     // Move the camera to the left.
        RIGHT,    // Move the camera to the right.
    };

    /** @brief A class representing a camera for navigating a scene. */
    class Camera {
        public:
            /** @brief Constructs a Camera object with specified position, up vector, yaw, and pitch.
             * @param cameraPosition The initial position of the camera in world space. Default is (0, 0, 0).
             * @param up The up vector of the world. Default is (0, 1, 0).
             * @param yaw The initial yaw angle (in degrees) of the camera. Default is -90 degrees.
             * @param pitch The initial pitch angle (in degrees) of the camera. Default is 0 degrees.
             */
            Camera(glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), f32 yaw = -90.0f, f32 pitch = 0.0f)
                : _position(cameraPosition), _front(glm::vec3(0.0f, 0.0f, -1.0f)), _worldUp(up), _yaw(yaw), _pitch(pitch), _mouseSensitivity(0.1f),
                  _zoom(45.0f) {
                UpdateCameraVectors();
            };

            /** @brief Processes input received from any keyboard-like input system to move the camera's position.
             * @param direction The direction in which to move the camera.
             * @param deltaTime The time elapsed since the last frame. Used to ensure consistent movement speed regardless of frame rate.
             * @param movementSpeed The speed at which the camera moves. Default is 2.5 units per second.
             */
            inline void ProcessKeyboardMovement(CameraMovement direction, f32 deltaTime, f32 movementSpeed = 2.5f) {
                f32 velocity = movementSpeed * deltaTime;

                switch (direction) {
                    case FORWARD:
                        _position += _front * velocity;
                        break;
                    case BACKWARD:
                        _position -= _front * velocity;
                        break;
                    case LEFT:
                        _position -= _right * velocity;
                        break;
                    case RIGHT:
                        _position += _right * velocity;
                        break;
                }
            }

            /** @brief Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis.
             * @param yOffset The offset value of the scroll in the y direction. Positive values indicate scrolling up, negative values indicate scrolling down.
             */
            inline void ProcessMouseScroll(f32 yOffset) {
                _zoom -= yOffset;

                if (_zoom < 1.0f) _zoom = 1.0f;
                if (_zoom > 45.0f) _zoom = 45.0f;
            }

            /** @brief Returns the front vector of the camera. */
            [[nodiscard]] inline glm::vec3 GetFront() const {
                return _front;
            }

            /** @brief Returns the zoom level (field of view) of the camera. */
            [[nodiscard]] inline f32 GetZoom() const {
                return _zoom;
            }

            /** @brief Returns the current position of the camera in world space. */
            [[nodiscard]] inline glm::vec3 GetPosition() const {
                return _position;
            }

            /** @brief Calculates and returns the view matrix using the camera's position and orientation.
             * @return The view matrix as a glm::mat4.
             */
            [[nodiscard]] inline glm::mat4 GetViewMatrix() const {
                return glm::lookAt(_position, _position + _front, _up);
            }

            /** @brief Processes input received from a mouse movement event to adjust the camera's yaw and pitch angles.
             * @param xOffset The offset value of the mouse movement in the x direction.
             * @param yOffset The offset value of the mouse movement in the y direction.
             * @param constrainPitch A boolean indicating whether to constrain the pitch angle to prevent screen flipping. Default is true.
             */
            inline void ProcessMouseMovement(f32 xOffset, f32 yOffset, bool constrainPitch = true) {
                _yaw += xOffset * _mouseSensitivity;
                _pitch += yOffset * _mouseSensitivity;

                // make sure that when pitch is out of bounds, screen doesn't get flipped
                if (constrainPitch) {
                    if (_pitch > 89.0f) _pitch = 89.0f;
                    if (_pitch < -89.0f) _pitch = -89.0f;
                }

                // update Front, Right and Up Vectors using the updated Euler angles
                UpdateCameraVectors();
            }

        private:
            /** @brief The position of the camera in world space. */
            glm::vec3 _position;

            /** @brief The front vector of the camera, indicating the direction it is facing. */
            glm::vec3 _front;

            /** @brief The up vector of the camera, indicating the upward direction relative to the camera's orientation. */
            glm::vec3 _up;

            /** @brief The right vector of the camera, indicating the rightward direction relative to the camera's orientation. */
            glm::vec3 _right;

            /** @brief The world's up vector, used as a reference for calculating the camera's orientation. */
            glm::vec3 _worldUp;

            /** @brief The yaw angle (in degrees) of the camera, representing rotation around the vertical axis. */
            f32 _yaw;

            /** @brief The pitch angle (in degrees) of the camera, representing rotation around the horizontal axis. */
            f32 _pitch;

            /** @brief The mouse sensitivity factor, affecting how much the camera rotates in response to mouse movement. */
            f32 _mouseSensitivity;

            /** @brief The zoom level (field of view) of the camera, affecting how wide the camera's view is. */
            f32 _zoom;

            /** @brief Updates the camera's front, right, and up vectors based on the current yaw and pitch angles. */
            void UpdateCameraVectors() {
                // calculate the new Front vector
                glm::vec3 front;
                front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
                front.y = sin(glm::radians(_pitch));
                front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
                _front = glm::normalize(front);

                // also re-calculate the Right and Up vector
                // normalize the vectors, because their length gets closer to 0 the more
                _right = glm::normalize(glm::cross(_front, _worldUp));
                _up = glm::normalize(glm::cross(_right, _front));
            }
    };
} // namespace Vulkyrie::Renderer
