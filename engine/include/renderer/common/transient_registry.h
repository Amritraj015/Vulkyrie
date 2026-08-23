#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "renderer/backend_concepts.h"
#include "renderer/rhi/resource_types.h"

namespace Vulkyrie {

    /** @brief Where a transient descriptor's identity is established, once, so that nothing per-frame has to
     * re-derive it.
     *
     * Registering a descriptor hashes it, deduplicates it against everything already registered, asks the backend
     * what it costs, and hands back an id. From then on the id *is* the descriptor: the frame graph declares
     * transients with it, `Compile` reads memory requirements through it, and `TransientPool` buckets by it. All
     * three are array indices.
     *
     * Register at startup, or when something invalidates a descriptor (a resize produces a new one, and the old id
     * simply stops being used). Not thread-safe and not meant to be: registering during recording would be a
     * setup-time operation on a hot path.
     *
     * @tparam B The renderer backend that sizes the descriptors. */
    template <RendererBackend B> class TransientRegistry final {
    public:
        /** @brief Constructs an empty registry.
         * @param context The backend, asked once per registration what a descriptor costs.
         * @param expectedTextures Reserve hint for distinct texture descriptors.
         * @param expectedBuffers Reserve hint for distinct buffer descriptors. */
        explicit TransientRegistry(typename B::Context &context, size_t expectedTextures, size_t expectedBuffers)
            : mContext(context) {
            mTextures.reserve(expectedTextures);
            mBuffers.reserve(expectedBuffers);
        }

        VE_DELETE_MOVE_AND_COPY(TransientRegistry);

        ~TransientRegistry() = default;

        /** @brief Registers a texture descriptor, or returns the id it already has.
         * @param descriptor The descriptor to register. */
        [[nodiscard]] TransientTextureID Register(const TextureDescriptor &descriptor) {
            return TransientTextureID{ registerIn(mTextures, mTexturesByHash, descriptor, [this](const TextureDescriptor &d) {
                return mContext.GetImageMemoryRequirements(d);
            }) };
        }

        /** @brief Registers a buffer descriptor, or returns the id it already has.
         * @param descriptor The descriptor to register. */
        [[nodiscard]] TransientBufferID Register(const BufferDescriptor &descriptor) {
            return TransientBufferID{ registerIn(mBuffers, mBuffersByHash, descriptor, [this](const BufferDescriptor &d) {
                return mContext.GetBufferMemoryRequirements(d);
            }) };
        }

        /** @brief Returns the descriptor an id was registered for.
         * @param id The id to resolve. */
        [[nodiscard]] VE_INLINE const TextureDescriptor &Descriptor(TransientTextureID id) const {
            VASSERT(id.IsValid() && id.Get() < mTextures.size(), "Transient texture id is out of range; was it registered with this device?");

            return mTextures[id.Get()].Descriptor;
        }

        /** @brief Returns the descriptor an id was registered for.
         * @param id The id to resolve. */
        [[nodiscard]] VE_INLINE const BufferDescriptor &Descriptor(TransientBufferID id) const {
            VASSERT(id.IsValid() && id.Get() < mBuffers.size(), "Transient buffer id is out of range; was it registered with this device?");

            return mBuffers[id.Get()].Descriptor;
        }

        /** @brief Returns what the backend said this descriptor costs, established at registration.
         * @param id The id to size. */
        [[nodiscard]] VE_INLINE const ResourceMemoryRequirements &Requirements(TransientTextureID id) const {
            VASSERT(id.IsValid() && id.Get() < mTextures.size(), "Transient texture id is out of range; was it registered with this device?");

            return mTextures[id.Get()].Requirements;
        }

        /** @brief Returns what the backend said this descriptor costs, established at registration.
         * @param id The id to size. */
        [[nodiscard]] VE_INLINE const ResourceMemoryRequirements &Requirements(TransientBufferID id) const {
            VASSERT(id.IsValid() && id.Get() < mBuffers.size(), "Transient buffer id is out of range; was it registered with this device?");

            return mBuffers[id.Get()].Requirements;
        }

        /** @brief Returns how many distinct texture descriptors are registered. */
        [[nodiscard]] VE_INLINE u32 TextureCount() const noexcept {
            return static_cast<u32>(mTextures.size());
        }

        /** @brief Returns how many distinct buffer descriptors are registered. */
        [[nodiscard]] VE_INLINE u32 BufferCount() const noexcept {
            return static_cast<u32>(mBuffers.size());
        }

    private:
        /** @brief One registered descriptor and everything derived from it. */
        template <typename TDescriptor> struct Registration final {
            TDescriptor Descriptor{};
            ResourceMemoryRequirements Requirements{};
        };

        /** @brief Registers a descriptor if it is new, and returns its index either way.
         *
         * The hash map here is the only one in the system keyed on a descriptor, and it is touched only by
         * registration - never by a frame.
         * @param registrations The dense array ids index into.
         * @param byHash Buckets of candidate indices, for deduplication.
         * @param descriptor The descriptor to register.
         * @param askBackend Invoked with the descriptor when it turns out to be new. */
        template <typename TDescriptor, typename TQuery>
        [[nodiscard]] u32 registerIn(std::vector<Registration<TDescriptor>> &registrations,
                                     std::unordered_map<u64, std::vector<u32>> &byHash,
                                     const TDescriptor &descriptor,
                                     TQuery &&askBackend) {
            const u64 hash = HashDescriptor(descriptor);

            std::vector<u32> &candidates = byHash[hash];

            // The hash picks the bucket; the descriptor decides the match, so a collision costs a compare rather
            // than handing back an id for the wrong shape.
            for (const u32 index : candidates) {
                if (registrations[index].Descriptor == descriptor) {
                    return index;
                }
            }

            const auto index = static_cast<u32>(registrations.size());

            registrations.push_back(Registration<TDescriptor>{ .Descriptor = descriptor, .Requirements = askBackend(descriptor) });
            candidates.push_back(index);

            return index;
        }

        typename B::Context &mContext;

        std::vector<Registration<TextureDescriptor>> mTextures;
        std::vector<Registration<BufferDescriptor>> mBuffers;

        /** @brief Deduplication indices, used only while registering. */
        std::unordered_map<u64, std::vector<u32>> mTexturesByHash;
        std::unordered_map<u64, std::vector<u32>> mBuffersByHash;
    };

} // namespace Vulkyrie
