#include "core/uuid.h"

namespace Vulkyrie {
    static std::random_device RANDOM_DEVICE;
    static std::mt19937_64 RANDOM_ENGINE(RANDOM_DEVICE());
    static std::uniform_int_distribution<u64> UNIFORM_DISTRIBUTION;

    UUID::UUID()
        : _uuid(UNIFORM_DISTRIBUTION(RANDOM_ENGINE)) {
    }

    UUID::UUID(u64 uuid)
        : _uuid(uuid) {
    }
} // namespace Vulkyrie
