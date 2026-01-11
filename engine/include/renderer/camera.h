#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Renderer {
    enum CameraMovement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
    };

    class Camera {
        public:
            Camera(glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), f32 yaw = -90.0f, f32 pitch = 0.0f)
                : _position(cameraPosition), _worldUp(up), _yaw(yaw), _pitch(pitch), _front(glm::vec3(0.0f, 0.0f, -1.0f)), _mouseSensitivity(0.1f),
                  _zoom(45.0f) {
                UpdateCameraVectors();
            };

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

            inline void ProcessMouseScroll(f32 yOffset) {
                _zoom -= yOffset;

                if (_zoom < 1.0f) _zoom = 1.0f;
                if (_zoom > 45.0f) _zoom = 45.0f;
            }

            [[nodiscard]] inline f32 GetZoom() const {
                return _zoom;
            }

            [[nodiscard]] inline glm::vec3 GetPosition() const {
                return _position;
            }

            [[nodiscard]] inline glm::mat4 GetViewMatrix() const {
                return glm::lookAt(_position, _position + _front, _up);
            }

            // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
            inline void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true) {
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
            glm::vec3 _position;
            glm::vec3 _front;
            glm::vec3 _up;
            glm::vec3 _right;
            glm::vec3 _worldUp;

            f32 _yaw;
            f32 _pitch;

            f32 _mouseSensitivity;
            f32 _zoom;

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
