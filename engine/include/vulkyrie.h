#pragma once

// Core includes
#include "core/application.h"
#include "core/application_manager.h"
#include "core/status_codes.h"
#include "core/logger.h"

// Events includes
#include "events/event_dispatcher.h"
#include "events/application/window_close_event.h"
#include "events/application/window_created_event.h"
#include "events/application/window_resize_event.h"
#include "events/keyboard/key_char_event.h"
#include "events/keyboard/key_pressed_event.h"
#include "events/keyboard/key_released_event.h"
#include "events/mouse/mouse_button_pressed_event.h"
#include "events/mouse/mouse_button_released_event.h"
#include "events/mouse/mouse_moved_event.h"
#include "events/mouse/mouse_scrolled_event.h"

extern Vulkyrie::Core::Application *CreateApplication();
