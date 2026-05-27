#pragma once

#include "physics/collision/shapes/collision_shape.h"

namespace Vulkyrie {

    /** @brief The `ConvexShape` class represents a convex collision shape, which is a type of collision shape that has the property that any line segment
     * connecting two points on the shape lies entirely within the shape. This class serves as a base class for specific types of convex shapes, such as boxes
     * and convex polyhedra. It provides common functionality for convex shapes, such as handling margins for collision detection and providing an interface for
     * checking if the shape is convex. The `ConvexShape` class is designed to be used in physics simulations and collision detection systems where accurate
     * representation of convex shapes is required. */
    class ConvexShape : public CollisionShape {
    public:
        /** @brief Construct a convex shape with the specified type, name, and margin.
         * @param type The type of the convex shape (e.g., ConvexPolyhedron).
         * @param name The specific name of the convex shape (e.g., Box, ConvexMesh).
         * @param margin The margin to be applied to the convex shape for collision detection purposes. This is an optional parameter that defaults to 0.0f
         * if not provided. A positive margin can help improve collision detection stability by providing a small buffer around the shape.
         * @param id The unique identifier of the convex shape in the overlapping pair.
         */
        explicit ConvexShape(CollisionShapeType type, CollisionShapeName name, f32 margin = 0.0f, u32 id = 0);

        /** @brief Virtual destructor for the ConvexShape class. */
        virtual ~ConvexShape() = default;

        // Delete the copy constructor and operator.
        ConvexShape(const ConvexShape &) = delete;
        ConvexShape &operator=(const ConvexShape &) = delete;

        // Delete the move constructor and operator.
        ConvexShape(ConvexShape &&) = delete;
        ConvexShape &operator=(ConvexShape &&) = delete;

        /** @brief Get the margin of the convex shape.
         * @returns The margin of the convex shape, which is a positive value that provides a buffer around the shape for collision detection purposes. A
         * larger margin can help improve collision detection stability by preventing objects from getting too close to each other, but it can also reduce
         * the accuracy of collision responses. The margin is typically used in algorithms like GJK and EPA to ensure that they can handle cases where
         * objects are very close or even penetrating each other without producing unstable results.
         */
        [[nodiscard]] VE_INLINE f32 GetMargin() const {
            return _margin;
        }

        /** @brief Check if the collision shape is convex.
         * @returns True if the collision shape is convex, false otherwise.
         */
        [[nodiscard]] VE_INLINE constexpr bool IsConvex() const override {
            return true;
        }

        /** @brief Get the local support point on the convex shape in the given direction, applying the margin. The support point is the point on the shape
         * that is farthest in the specified direction, and it is used in collision detection algorithms like GJK to determine if two shapes are
         * intersecting. This function should be implemented by derived classes to provide the specific logic for calculating the support point based on the
         * geometry of the shape, while also taking into account the margin to ensure stability in collision detection. The margin is typically added to the
         * support point in the direction of the input vector to create a buffer around the shape for collision detection purposes.
         * @param direction The direction in which to calculate the support point, represented as a glm::vec3. The direction vector does not need to be
         * normalized.
         * @returns The local support point on the convex shape in the given direction, with the margin applied. This is the point on the shape that is
         * farthest in the specified direction, and it is used for collision detection purposes. */
        virtual glm::vec3 GetLocalSupportPointWithMargin(const glm::vec3 &direction) const;

        /** @brief Get the local support point on the convex shape in the given direction, without applying the margin. The support point is the point on
         * the shape that is farthest in the specified direction, and it is used in collision detection algorithms like GJK to determine if two shapes are
         * intersecting. This function should be implemented by derived classes to provide the specific logic for calculating the support point based on the
         * geometry of the shape.
         * @param direction The direction in which to calculate the support point, represented as a glm::vec3. The direction vector does not need to be
         * normalized.
         * @returns The local support point on the convex shape in the given direction, without applying the margin. This is the point on the shape that is
         * farthest in the specified direction, and it is used for collision detection purposes. */
        virtual glm::vec3 GetLocalSupportPointWithoutMargin(const glm::vec3 &direction) const = 0;

    protected:
        f32 _margin;
    };

} // namespace Vulkyrie
