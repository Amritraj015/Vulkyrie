#include "renderer/rhi/capabilities.h"

namespace Vulkyrie {

    namespace {

        // Vulkan packs a version as variant:3 | major:7 | minor:10 | patch:12.
        std::string formatApiVersion(u32 version) {
            return std::format("{}.{}.{}", (version >> 22) & 0x7Fu, (version >> 12) & 0x3FFu, version & 0xFFFu);
        }

        // Driver versions only follow the API packing by convention. NVIDIA publishes its own
        // layout, so a driver reported as "570.86.16" reads as "2.63.234" without this case.
        std::string formatDriverVersion(u32 version, VendorID vendor) {
            if (VendorID::NVIDIA == vendor) {
                return std::format("{}.{}.{}.{}", (version >> 22) & 0x3FFu, (version >> 14) & 0xFFu, (version >> 6) & 0xFFu, version & 0x3Fu);
            }

            return formatApiVersion(version);
        }

    } // namespace

    std::string_view DeviceTypeName(DeviceType deviceType) {
        switch (deviceType) {
            case DeviceType::IntegratedGPU:
                return "Integrated GPU";
            case DeviceType::DiscreteGPU:
                return "Discrete GPU";
            case DeviceType::VirtualGPU:
                return "Virtual GPU";
            case DeviceType::CPU:
                return "CPU";
            default:
                return "Other";
        }
    }

    std::string_view VendorName(VendorID vendor) {
        switch (vendor) {
            case VendorID::AMD:
                return "AMD";
            case VendorID::NVIDIA:
                return "NVIDIA";
            case VendorID::Intel:
                return "Intel";
            case VendorID::Apple:
                return "Apple";
            case VendorID::ARM:
                return "ARM";
            case VendorID::Qualcomm:
                return "Qualcomm";
            case VendorID::ImaginationTechnologies:
                return "Imagination Technologies";
            case VendorID::Broadcom:
                return "Broadcom";
            case VendorID::Mesa:
                return "Mesa";
            default:
                return "Unknown";
        }
    }

    std::string DeviceIdentity::ToString() const {
        std::string text;

        // DeviceName/DriverInfo are fixed-size buffers; std::string_view over the C string keeps
        // the trailing padding out of the log.
        text += std::format("Name           : {}\n", static_cast<const char *>(DeviceName));
        text += std::format("Type           : {}\n", DeviceTypeName(DeviceType));
        text += std::format("Vendor         : {}\n", VendorName(VendorID));
        text += std::format("Device ID      : 0x{:04X}\n", DeviceID);
        text += std::format("API version    : {}\n", formatApiVersion(ApiVersion));
        text += std::format("Driver version : {}\n", formatDriverVersion(DriverVersion, VendorID));
        text += std::format("Driver         : {} (id {})", static_cast<const char *>(DriverInfo), DriverID);

        return text;
    }

    std::string DeviceCapabilities::ToString() const {
        return Identity.ToString();
    }

} // namespace Vulkyrie
