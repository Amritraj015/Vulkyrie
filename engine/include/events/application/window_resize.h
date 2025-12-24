// #pragma once

// #include "defines.h"
// #include "events/event.h"
// #include "events/enums/event_category.h"
// #include "events/enums/event_type.h"

// namespace Vulkyrie::Events {
//     class WindowResizeEvent : public Event {
//         public:
//             WindowResizeEvent(const u32 width, const u32 height) : _width(width), _height(height) {
//             }

//             [[nodiscard]] inline u32 GetWidth() const {
//                 return _width;
//             }

//             [[nodiscard]] inline u32 GetHeight() const {
//                 return _height;
//             }

//             [[nodiscard]] inline i32 GetCategoryFlags() const override {
//                 return _categoryFlags;
//             }

//             [[nodiscard]] inline EventType GetEventType() const override {
//                 return EventType::WindowResize;
//             }

//         private:
//             const u32 _width, _height;
//             const static i32 _categoryFlags = std::to_underlying(EventCategory::ApplicationEvent);
//     };
// } // namespace Vulkyrie::Events