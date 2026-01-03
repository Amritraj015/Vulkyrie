#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Renderer {
    enum CameraMovement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        // UP,
        // DOWN

    };

    // Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch =
    // PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)

    class Camera {
        public:
            Camera(glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), f32 yaw = -90.0f,
                   f32 pitch = 0.0f)
                : _position(cameraPosition), _worldUp(up), _yaw(yaw), _pitch(pitch), _front(glm::vec3(0.0f, 0.0f, -1.0f)),
                  _mouseSensitivity(0.1f), _zoom(45.0f) {
                UpdateCameraVectors();
            };

            void ProcessKeyboardMovement(CameraMovement direction, f32 deltaTime, f32 movementSpeed = 2.5f) {
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

            glm::vec3 GetPosition() const {
                return _position;
            }

            glm::mat4 GetViewMatrix() const {
                return glm::lookAt(_position, _position + _front, _up);
            }

            // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
            void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true) {
                xOffset *= _mouseSensitivity;
                yOffset *= _mouseSensitivity;

                _yaw += xOffset;
                _pitch += yOffset;

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

    class Camera2D : public Camera {};
    class Camera3D : public Camera {};
} // namespace Vulkyrie::Renderer
