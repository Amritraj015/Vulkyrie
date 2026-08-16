#include "core/utilities/uuid.h"

namespace Vulkyrie {

    namespace {
        inline std::random_device RANDOM_DEVICE;
        inline std::mt19937_64 RANDOM_ENGINE(RANDOM_DEVICE());
        inline std::uniform_int_distribution<u64> UNIFORM_DISTRIBUTION;
    } // namespace

    UUID::UUID()
        : _uuid(UNIFORM_DISTRIBUTION(RANDOM_ENGINE)) {
    }

    UUID::UUID(u64 uuid)
        : _uuid(uuid) {
    }

} // namespace Vulkyrie
