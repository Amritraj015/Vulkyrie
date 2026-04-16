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
             */
            ConvexShape(CollisionShapeType type, CollisionShapeName name, f32 margin = 0.0f);

            /** @brief Virtual destructor for the ConvexShape class. */
            virtual ~ConvexShape() = default;

            // Delete the copy constructor and operator.
            ConvexShape(const ConvexShape &) = delete;
            ConvexShape &operator=(const ConvexShape &) = delete;

            // Delete the move constructor and operator.
            ConvexShape(ConvexShape &&) = delete;
            ConvexShape &operator=(ConvexShape &&) = delete;

            /** @brief Get the margin of the convex shape.
             * @return The margin of the convex shape, which is a positive value that provides a buffer around the shape for collision detection purposes. A
             * larger margin can help improve collision detection stability by preventing objects from getting too close to each other, but it can also reduce
             * the accuracy of collision responses. The margin is typically used in algorithms like GJK and EPA to ensure that they can handle cases where
             * objects are very close or even penetrating each other without producing unstable results.
             */
            [[nodiscard]] VE_FORCE_INLINE f32 GetMargin() const {
                return _margin;
            }

            /** @brief Check if the collision shape is convex.
             * @return True if the collision shape is convex, false otherwise.
             */
            [[nodiscard]] VE_FORCE_INLINE constexpr bool IsConvex() const override {
                return true;
            }

        protected:
            const f32 _margin;

            // /** Return a local support point in a given direction with the object margin. */
            // glm::vec3 getLocalSupportPointWithMargin(const glm::vec3 &direction) const;
            //
            // /** Return a local support point in a given direction without the object margin. */
            // virtual glm::vec3 getLocalSupportPointWithoutMargin(const glm::vec3 &direction) const = 0;
    };

} // namespace Vulkyrie
