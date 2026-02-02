#pragma once

// Core includes
#include "core/application.h"
#include "core/status_codes.h"
#include "core/logger.h"
#include "core/noise_generator.h"

// Debug includes
#include "debug/profiler.h"

// Events includes
#include "events/event_dispatcher.h"
#include "events/application/window_closed_event.h"
#include "events/application/window_created_event.h"
#include "events/application/window_resized_event.h"
#include "events/keyboard/key_char_event.h"
#include "events/keyboard/key_pressed_event.h"
#include "events/keyboard/key_released_event.h"
#include "events/mouse/mouse_button_pressed_event.h"
#include "events/mouse/mouse_button_released_event.h"
#include "events/mouse/mouse_moved_event.h"
#include "events/mouse/mouse_scrolled_event.h"

// Input includes
#include "input/inputs.h"

// Renderer includes
#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "renderer/camera.h"
#include "renderer/buffer_element.h"
#include "renderer/buffer_layout.h"
#include "renderer/vertex_array.h"
#include "renderer/vertex_buffer.h"
#include "renderer/index_buffer.h"
#include "renderer/light.h"
#include "renderer/mesh.h"
#include "renderer/model.h"
#include "renderer/texture_image_format.h"
#include "renderer/texture_specification.h"
#include "renderer/texture.h"
#include "renderer/texture_2D.h"
#include "renderer/texture_cube_map.h"

// Material includes
#include "materials/material_library.h"

extern Vulkyrie::Core::Application *CreateApplication();
