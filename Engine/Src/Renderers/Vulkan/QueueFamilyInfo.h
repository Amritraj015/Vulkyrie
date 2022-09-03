#pragma once

namespace Vkr
{
    struct QueueFamilyInfo
    {
        i32 graphicsFamilyIndex = -1;
		i32 presentFamilyIndex = -1;
		i32 computeFamilyIndex = -1;
		i32 transferFamilyIndex = -1;
    };
}