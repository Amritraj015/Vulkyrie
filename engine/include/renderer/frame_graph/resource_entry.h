#pragma once

#include "renderer/frame_graph/frame_graph.h"
#include "renderer/frame_graph/frame_graph_traits.h"
#include "renderer/frame_graph/resource_node.h"

namespace Vulkyrie::Renderer {
    class ResourceEntry final {
            friend class FrameGraph;

            enum class Type : u32 { Transient, Imported };

        public:
            ResourceEntry() = delete;
            ~ResourceEntry() = default;

            ResourceEntry(const ResourceEntry &) = delete;
            ResourceEntry &operator=(const ResourceEntry &) = delete;

            ResourceEntry(ResourceEntry &&) = delete;
            ResourceEntry &operator=(ResourceEntry &&) = delete;

            /** @brief Gets the identifier of the resource. */
            [[nodiscard]] auto GetResourceID() {
                return _resourceID;
            }

            /** @brief Retrieves the version number of the resource. */
            [[nodiscard]] auto GetVersion() {
                return _version;
            }

            /** @brief Checks if the resource is transient, meaning it is created and destroyed within the same frame. */
            [[nodiscard]] auto IsTransient() {
                return _type == Type::Transient;
            }

            /** @brief Checks if the resource is imported, meaning it is created outside of the frame graph and managed externally. */
            [[nodiscard]] auto IsImported() {
                return _type == Type::Imported;
            }

            /** @brief Retrieves the actual resource associated with the backend. */
            template <FrameGraphResourceBackend T> [[nodiscard]] T &GetResource() const {
                return static_cast<ResourceModel<T> *>(_concept.get())->resource;
            }

            /** @brief Retrieves the descriptor associated with the resource,
             * which contains the necessary information for creating and managing the resource. */
            template <FrameGraphResourceBackend T> [[nodiscard]] const typename T::Descriptor &GetDescriptor() const {
                return static_cast<ResourceModel<T> *>(_concept.get())->descriptor;
            }

            struct Concept {
                public:
                    virtual ~Concept() = default;

                    /** @brief Creates the resource associated with the given allocator.
                     * @param allocator A pointer to any additional allocator needed for the create operation. */
                    virtual void Create(void *allocator) = 0;

                    /** @brief Destroys the resource associated with the given allocator.
                     * @param allocator A pointer to any additional allocator needed for the destroy operation. */
                    virtual void Destroy(void *allocator) = 0;

                    /** @brief Optional method to perform operations before a resource is read.
                     * @param flags Flags indicating the type of read operation.
                     * @param context A pointer to any additional context needed for the pre-read operation. */
                    virtual void PreRead(u32 flags, void *context) = 0;

                    /** @brief Optional method to perform operations before a resource is written to.
                     * @param flags Flags indicating the type of write operation.
                     * @param context A pointer to any additional context needed for the pre-write operation. */
                    virtual void PreWrite(u32 flags, void *context) = 0;
            };

            template <FrameGraphResourceBackend T> struct ResourceModel : Concept {
                public:
                    T resource;
                    const typename T::Descriptor descriptor;

                    void Create(void *allocator) override {
                        resource.Create(descriptor, allocator);
                    }

                    void Destroy(void *allocator) override {
                        resource.Destroy(descriptor, allocator);
                    }

                    void PreRead(u32 flags, void *context) override {
                        if constexpr (HasPreRead<T>) {
                            resource.PreRead(flags, context);
                        }
                    }

                    void PreWrite(u32 flags, void *context) override {
                        if constexpr (HasPreWrite<T>) {
                            resource.PreWrite(flags, context);
                        }
                    }
            };

        private:
            template <FrameGraphResourceBackend T>
            ResourceEntry(Type type, u32 resourceID, T::Descriptor descriptor, T &&resource)
                : _type{ type }
                , _version{ __INITIAL_RESOURCE_VERSION__ }
                , _resourceID{ resourceID }
                , _concept{ std::make_unique<ResourceModel<T>>(ResourceModel<T>{ std::forward<T>(resource), descriptor }) } {
            }

            const Type _type;
            u32 _version;
            ResourceID _resourceID;
            std::unique_ptr<Concept> _concept;

            PassNode *_creator{ nullptr };
            PassNode *_lastUsedBy{ nullptr };

            inline void SetCreator(PassNode *creator) {
                _creator = creator;
            }

            inline void SetLastUsedBy(PassNode *lastUsedBy) {
                _lastUsedBy = lastUsedBy;
            }

            static constexpr inline u8 __INITIAL_RESOURCE_VERSION__ = 1U;
    };
} // namespace Vulkyrie::Renderer
