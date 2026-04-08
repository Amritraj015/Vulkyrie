#pragma once

// TODO: Remove this.
#include "audio/audio_system.h"
// TODO: Remove this.

// Core includes
#include "core/application.h"
#include "core/status_codes.h"
#include "core/logger.h"
#include "core/noise_generator.h"

// Debug includes
#include "debug/profiler.h"

// ECS includes
#include "ecs/entity.h"
#include "ecs/entity_manager.h"
#include "ecs/components/transform_component_store.h"

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
#include "renderer/frame_buffer.h"
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
// -----------------------------------------------------------
#include "renderer/frame_graph/frame_graph.h"
#include "renderer/frame_graph/frame_graph_blackboard.h"
#include "renderer/frame_graph/frame_graph_traits.h"
#include "renderer/frame_graph/frame_graph_types.h"
#include "renderer/frame_graph/resource_node.h"
#include "renderer/frame_graph/resource_entry.h"
#include "renderer/frame_graph/pass_node.h"
// -----------------------------------------------------------

// Material includes
#include "materials/material_library.h"

// Physics includes
#include "physics/physics_world_settings.h"
#include "physics/physics_world.h"
#include "physics/collision/shapes/sphere.h"
#include "physics/collision/shapes/plane.h"
#include "physics/collision/shapes/AABB.h"

extern std::unique_ptr<Vulkyrie::Application> CreateApplication();
