#include "memory/allocators/free_list_allocator.h"

#include "core/asserts.h"
#include "memory/memory_tracker.h"

namespace Vulkyrie {

    namespace {

        /** @brief Rounds a pointer-sized value up to the next multiple of a power-of-two alignment. */
        [[nodiscard]] VE_INLINE size_t AlignUp(size_t value, size_t alignment) {
            return (value + alignment - 1) & ~(alignment - 1);
        }

    } // namespace

    FreeListAllocator::FreeListAllocator(size_t capacityBytes, MemoryTag tag)
        : _tag{ tag } {
        // Every allocation needs a header, an offset word and a minimum payload, so a region smaller than one
        // block could never serve anything.
        _capacity = std::max(capacityBytes, BLOCK_OVERHEAD + MIN_PAYLOAD);

        // Untracked on purpose: reported below through the reserved-pool channel instead, so a bounded budget
        // reads as memory reserved rather than memory handed out.
        _memory = static_cast<std::byte *>(std::malloc(_capacity));
        VASSERT(_memory != nullptr, "Free list allocator failed to reserve its region.");

        MemoryTracker::OnPoolReserve(_tag, static_cast<i64>(_capacity));

        Reset();
    }

    FreeListAllocator::~FreeListAllocator() {
        releaseRegion();
    }

    FreeListAllocator::FreeListAllocator(FreeListAllocator &&other) noexcept
        : _memory{ other._memory }
        , _capacity{ other._capacity }
        , _freeList{ other._freeList }
        , _used{ other._used }
        , _allocationCount{ other._allocationCount }
        , _highWaterMark{ other._highWaterMark }
        , _tag{ other._tag } {
        other._memory = nullptr;
        other._capacity = 0;
        other._freeList = nullptr;
        other._used = 0;
        other._allocationCount = 0;
    }

    FreeListAllocator &FreeListAllocator::operator=(FreeListAllocator &&other) noexcept {
        if (this != &other) {
            releaseRegion();

            _memory = other._memory;
            _capacity = other._capacity;
            _freeList = other._freeList;
            _used = other._used;
            _allocationCount = other._allocationCount;
            _highWaterMark = other._highWaterMark;
            _tag = other._tag;

            // The adopted region was already reported as reserved under the same tag; only the in-use bytes
            // change owner.
            MemoryTracker::OnPoolAllocate(_tag, static_cast<i64>(_used));

            other._memory = nullptr;
            other._capacity = 0;
            other._freeList = nullptr;
            other._used = 0;
            other._allocationCount = 0;
        }

        return *this;
    }

    void FreeListAllocator::Reset() {
        MemoryTracker::OnPoolFree(_tag, static_cast<i64>(_used));

        _used = 0;
        _allocationCount = 0;
        _freeList = nullptr;

        auto *block = reinterpret_cast<BlockHeader *>(_memory);
        *block = BlockHeader{ .Size = _capacity, .PrevSize = 0, .Free = true };

        pushFree(block);
    }

    void *FreeListAllocator::Allocate(size_t size, size_t alignment) {
        VASSERT(alignment > 0 && (alignment & (alignment - 1)) == 0, "Free list allocator alignment must be a power of two.");

        // Zero-sized requests still get a distinct address, matching operator new.
        const size_t requested = std::max<size_t>(size, 1);

        for (FreeNode *node = _freeList; node != nullptr; node = node->Next) {
            auto *block = reinterpret_cast<BlockHeader *>(reinterpret_cast<std::byte *>(node) - sizeof(BlockHeader));

            const auto blockStart = reinterpret_cast<size_t>(block);
            const size_t payload = AlignUp(blockStart + BLOCK_OVERHEAD, alignment);
            const size_t payloadEnd = payload + requested;

            if (payloadEnd > blockStart + block->Size) {
                continue;
            }

            popFree(block);

            // Split the tail off when what is left could serve another allocation; otherwise leave the slack
            // inside this block, because a fragment too small to use is worse than a slightly fat block.
            const size_t consumed = payloadEnd - blockStart;
            const size_t remainder = block->Size - consumed;

            if (remainder >= BLOCK_OVERHEAD + MIN_PAYLOAD) {
                block->Size = consumed;

                auto *tail = reinterpret_cast<BlockHeader *>(reinterpret_cast<std::byte *>(block) + consumed);
                *tail = BlockHeader{ .Size = remainder, .PrevSize = consumed, .Free = true };

                // The block after the tail now has a new physical predecessor.
                if (BlockHeader *afterTail = nextBlock(tail); afterTail != nullptr) {
                    afterTail->PrevSize = remainder;
                }

                pushFree(tail);
            }

            block->Free = false;

            // Record how far back the header is, so Free can find it through whatever alignment padding we
            // inserted here.
            const auto offset = static_cast<PayloadOffset>(payload - blockStart);
            std::memcpy(reinterpret_cast<void *>(payload - sizeof(PayloadOffset)), &offset, sizeof(offset));

            _used += requested;
            _highWaterMark = std::max(_highWaterMark, _used);
            ++_allocationCount;

            MemoryTracker::OnPoolAllocate(_tag, static_cast<i64>(requested));

            return reinterpret_cast<void *>(payload);
        }

        return nullptr;
    }

    void FreeListAllocator::Free(void *pointer) {
        if (pointer == nullptr) {
            return;
        }

        // Checked before `headerFromPayload`, which reads the offset stored below the payload - itself an
        // out-of-bounds read for a pointer that never came from this region.
        VASSERT(static_cast<std::byte *>(pointer) >= _memory + BLOCK_OVERHEAD && static_cast<std::byte *>(pointer) < _memory + _capacity,
                "Free list allocator freed a pointer from outside its region.");

        BlockHeader *block = headerFromPayload(pointer);

        VASSERT(reinterpret_cast<std::byte *>(block) >= _memory && reinterpret_cast<std::byte *>(block) < _memory + _capacity,
                "Free list allocator freed a pointer whose recorded header offset lands outside its region.");
        VASSERT(!block->Free, "Free list allocator freed the same pointer twice.");

        // The payload size is recoverable from the block: everything between the returned pointer and the block's
        // end was charged to the caller.
        const auto payloadAddress = reinterpret_cast<size_t>(pointer);
        const auto blockStart = reinterpret_cast<size_t>(block);
        const size_t charged = blockStart + block->Size - payloadAddress;

        _used -= std::min(_used, charged);
        --_allocationCount;

        MemoryTracker::OnPoolFree(_tag, static_cast<i64>(charged));

        block->Free = true;
        pushFree(coalesce(block));
    }

    size_t FreeListAllocator::LargestFreeBlock() const {
        size_t largest = 0;

        for (const FreeNode *node = _freeList; node != nullptr; node = node->Next) {
            const auto *block = reinterpret_cast<const BlockHeader *>(reinterpret_cast<const std::byte *>(node) - sizeof(BlockHeader));

            if (block->Size > largest) {
                largest = block->Size;
            }
        }

        return largest > BLOCK_OVERHEAD ? largest - BLOCK_OVERHEAD : 0;
    }

    FreeListAllocator::BlockHeader *FreeListAllocator::headerFromPayload(void *pointer) {
        PayloadOffset offset = 0;
        std::memcpy(&offset, static_cast<const std::byte *>(pointer) - sizeof(PayloadOffset), sizeof(offset));

        return reinterpret_cast<BlockHeader *>(static_cast<std::byte *>(pointer) - offset);
    }

    FreeListAllocator::BlockHeader *FreeListAllocator::nextBlock(BlockHeader *block) const {
        auto *next = reinterpret_cast<std::byte *>(block) + block->Size;

        return next < _memory + _capacity ? reinterpret_cast<BlockHeader *>(next) : nullptr;
    }

    FreeListAllocator::BlockHeader *FreeListAllocator::previousBlock(BlockHeader *block) const {
        if (block->PrevSize == 0) {
            return nullptr;
        }

        return reinterpret_cast<BlockHeader *>(reinterpret_cast<std::byte *>(block) - block->PrevSize);
    }

    void FreeListAllocator::pushFree(BlockHeader *block) {
        auto *node = reinterpret_cast<FreeNode *>(reinterpret_cast<std::byte *>(block) + sizeof(BlockHeader));

        node->Prev = nullptr;
        node->Next = _freeList;

        if (_freeList != nullptr) {
            _freeList->Prev = node;
        }

        _freeList = node;
        block->Free = true;
    }

    void FreeListAllocator::popFree(BlockHeader *block) {
        auto *node = reinterpret_cast<FreeNode *>(reinterpret_cast<std::byte *>(block) + sizeof(BlockHeader));

        if (node->Prev != nullptr) {
            node->Prev->Next = node->Next;
        } else {
            _freeList = node->Next;
        }

        if (node->Next != nullptr) {
            node->Next->Prev = node->Prev;
        }

        node->Next = nullptr;
        node->Prev = nullptr;
        block->Free = false;
    }

    FreeListAllocator::BlockHeader *FreeListAllocator::coalesce(BlockHeader *block) {
        // Merge forward first, so the backward merge below sees the largest possible successor and the physical
        // chain only has to be fixed up once.
        if (BlockHeader *next = nextBlock(block); next != nullptr && next->Free) {
            popFree(next);
            block->Size += next->Size;

            if (BlockHeader *afterNext = nextBlock(block); afterNext != nullptr) {
                afterNext->PrevSize = block->Size;
            }
        }

        if (BlockHeader *previous = previousBlock(block); previous != nullptr && previous->Free) {
            popFree(previous);
            previous->Size += block->Size;

            if (BlockHeader *afterPrevious = nextBlock(previous); afterPrevious != nullptr) {
                afterPrevious->PrevSize = previous->Size;
            }

            block = previous;
        }

        return block;
    }

    void FreeListAllocator::releaseRegion() {
        if (_memory == nullptr) {
            return;
        }

        MemoryTracker::OnPoolFree(_tag, static_cast<i64>(_used));
        MemoryTracker::OnPoolRelease(_tag, static_cast<i64>(_capacity));

        std::free(_memory);

        _memory = nullptr;
        _capacity = 0;
        _used = 0;
        _allocationCount = 0;
        _freeList = nullptr;
    }

} // namespace Vulkyrie
