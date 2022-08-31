#pragma once

namespace Vkr
{
    struct QueueFamilyInfo
    {
        u32 graphicsFamilyIndex = -1;
        u32 presentFamilyIndex = -1;
        u32 computeFamilyIndex = -1;
        u32 transferFamilyIndex = -1;
    };
}