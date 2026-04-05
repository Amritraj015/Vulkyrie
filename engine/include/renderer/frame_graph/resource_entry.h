#pragma once

#include "renderer/frame_graph/frame_graph_traits.h"
#include "renderer/frame_graph/frame_graph_types.h"
#include "renderer/frame_graph/resource_node.h"

namespace Vulkyrie {

    /** @brief Represents a resource entry in the frame graph. Each resource entry encapsulates information about a resource, such as its type (transient or
     * imported), its version for tracking changes, and the actual resource data. Resource entries are used for managing resource lifetimes and ensuring that
     * resources are correctly created, destroyed, and updated within the frame graph.
     * */
    class ResourceEntry final {
            friend class FrameGraph;

            enum class Type : u32 { Transient, Imported };

        public:
            ResourceEntry() = delete;
            ~ResourceEntry() = default;

            ResourceEntry(const ResourceEntry &) = delete;
            ResourceEntry &operator=(const ResourceEntry &) = delete;

            ResourceEntry(ResourceEntry &&) = default;
            ResourceEntry &operator=(ResourceEntry &&) = delete;

            /** @brief Creates the resource associated with this entry using the provided allocator.
             * This method will only be executed for Transient resources, as Persistent resources are expected to be created and managed externally.
             * @param allocator A pointer to any additional allocator needed for the create operation.
             * This can be used to pass in custom allocators or context information required for resource creation.
             * The specific type and usage of the allocator will depend on the implementation of the resource backend and the requirements of the resource being
             * created.
             * */
            void Create(void *allocator) {
                if (_type == Type::Transient) {
                    _concept->Create(allocator);
                }
            }

            /** @brief Destroys the resource associated with this entry using the provided allocator.
             * This method will only be executed for Transient resources, as Persistent resources are expected to be managed externally and should not
             * be automatically destroyed by the frame graph.
             * @param allocator A pointer to any additional allocator needed for the destroy operation.
             * This can be used to pass in custom allocators or context information required for resource destruction.
             * The specific type and usage of the allocator will depend on the implementation of the resource backend and the requirements of the resource being
             * destroyed. */
            void Destroy(void *allocator) {
                if (_type == Type::Transient) {
                    _concept->Destroy(allocator);
                }
            }

            /** @brief Optional method to perform operations before a resource is read.
             * @param flags Flags indicating the type of read operation. These flags can be used for optimization or to specify special handling for certain
             * types of reads. The specific meaning and usage of the flags will depend on the implementation of the resource backend and the requirements of the
             * resource being read.
             * @param context A pointer to any additional context needed for the pre-read operation. This can be used to pass in custom context information
             * required for pre-read operations, such as a rendering context or command buffer. The specific type and usage of the context will depend on the
             * implementation of the resource backend and the requirements of the resource being read. */
            void PreRead(i32 flags, void *context) {
                _concept->PreRead(flags, context);
            }

            /** @brief Optional method to perform operations before a resource is written to.
             * @param flags Flags indicating the type of write operation. These flags can be used for optimization or to specify special handling for certain
             * types of writes. The specific meaning and usage of the flags will depend on the implementation of the resource backend and the requirements of
             * the resource being written to.
             * @param context A pointer to any additional context needed for the pre-write operation. This can be used to pass in custom context information
             * required for pre-write operations, such as a rendering context or command buffer. The specific type and usage of the context will depend on the
             * implementation of the resource backend and the requirements of the resource being written to. */
            void PreWrite(i32 flags, void *context) {
                _concept->PreWrite(flags, context);
            }

            /** @brief Gets the identifier of the resource entry. */
            [[nodiscard]] ResourceEntryID GetResourceEntryID() const {
                return _resourceEntryID;
            }

            /** @brief Checks if the resource is transient, meaning it is created and destroyed within the same frame. */
            [[nodiscard]] auto IsTransient() const {
                return _type == Type::Transient;
            }

            /** @brief Checks if the resource is imported, meaning it is created outside of the frame graph and managed externally. */
            [[nodiscard]] auto IsImported() const {
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

            /** @brief A concept that defines the operations for creating, destroying, and managing the resource associated with this entry.
             * This allows for type-erased handling of different resource backends while still providing the necessary functionality for resource management. */
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
                    constexpr virtual void PreRead(i32 flags, void *context) = 0;

                    /** @brief Optional method to perform operations before a resource is written to.
                     * @param flags Flags indicating the type of write operation.
                     * @param context A pointer to any additional context needed for the pre-write operation. */
                    constexpr virtual void PreWrite(i32 flags, void *context) = 0;
            };

            template <FrameGraphResourceBackend T> struct ResourceModel : Concept {
                public:
                    /** @brief Constructs a ResourceModel with the specified descriptor and resource.
                     * @param descriptor The descriptor containing the necessary information for creating and managing the resource. The specific fields and
                     * requirements of the descriptor will depend on the implementation of the resource backend and the requirements of the resource being
                     * created.
                     * @param resource The actual resource associated with the backend. This should be an instance of the type that satisfies the
                     * FrameGraphResourceBackend concept, and it should be initialized with any necessary information for creating and managing the resource. */
                    ResourceModel(const typename T::Descriptor &descriptor, T &&resource)
                        : descriptor(descriptor)
                        , resource(std::move(resource)) {
                    }

                    /** @brief The descriptor containing the necessary information for creating and managing the resource. */
                    const typename T::Descriptor descriptor;

                    /** @brief The actual resource associated with the backend. */
                    T resource;

                    void Create(void *allocator) override {
                        resource.Create(descriptor, allocator);
                    }

                    void Destroy(void *allocator) override {
                        resource.Destroy(descriptor, allocator);
                    }

                    constexpr void PreRead(i32 flags, void *context) override {
                        if constexpr (HasPreRead<T>) {
                            resource.PreRead(flags, context);
                        }
                    }

                    constexpr void PreWrite(i32 flags, void *context) override {
                        if constexpr (HasPreWrite<T>) {
                            resource.PreWrite(flags, context);
                        }
                    }
            };

        private:
            /** @brief Constructs a ResourceEntry with the specified type, resource entry ID, descriptor, and resource.
             * @param type The type of the resource (Transient or Imported).
             * @param resourceEntryID The identifier for this resource entry, which is used for tracking and management within the frame graph.
             * @param descriptor The descriptor containing the necessary information for creating and managing the resource.
             * @param resource The actual resource associated with the backend. */
            template <FrameGraphResourceBackend T>
            explicit ResourceEntry(Type type, ResourceEntryID resourceEntryID, const typename T::Descriptor &descriptor, T &&resource)
                : _type{ type }
                , _version{ INITIAL_RESOURCE_VERSION }
                , _resourceEntryID{ resourceEntryID }
                , _concept{ std::make_unique<ResourceModel<T>>(ResourceModel<T>{ descriptor, std::forward<T>(resource) }) } {
            }

            /** @brief The initial version number for resources. */
            static constexpr u8 INITIAL_RESOURCE_VERSION = 1U;

            /** @brief The type of the resource, which can be either Transient or Imported.
             * This indicates how the resource is managed and its lifecycle within the frame graph. */
            const Type _type;

            /** @brief The version number of the resource, which is used for tracking changes and ensuring that
             * resources are correctly updated and managed within the frame graph. */
            u32 _version;

            /** @brief The identifier for this resource entry, which is used for tracking and management within the frame graph. */
            ResourceEntryID _resourceEntryID;

            /** @brief The concept that defines the operations for creating, destroying, and managing the resource associated with this entry.
             * This allows for type-erased handling of different resource backends while still providing the necessary functionality for resource management. */
            std::unique_ptr<Concept> _concept;

            /** @brief Pointer to the pass node that creates this resource.
             * This is used for tracking dependencies and managing resource lifetimes within the frame graph. */
            [[maybe_unused]] PassNode *_creator{ nullptr };

            /** @brief Pointer to the last pass node that uses this resource.
             * This is used for tracking dependencies and managing resource lifetimes within the frame graph. */
            [[maybe_unused]] PassNode *_lastUsedBy{ nullptr };
    };
} // namespace Vulkyrie
