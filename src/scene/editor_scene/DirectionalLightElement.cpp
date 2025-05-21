#include "DirectionalLightElement.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "rendering/imgui/ImGuiManager.h"
#include "scene/SceneContext.h"

std::unique_ptr<EditorScene::DirectionalLightElement> EditorScene::DirectionalLightElement::new_default(const SceneContext& scene_context, EditorScene::ElementRef parent) {
    glm::vec3 default_direction = glm::normalize(glm::vec3(0.5f, -1.0f, 0.5f));

    auto light_element = std::make_unique<DirectionalLightElement>(
        parent,
        "New Directional Light",
        default_direction,
        DirectionalLight::create(
            default_direction, // Set via update_instance_data()
            glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}
        ),
        EmissiveEntityRenderer::Entity::create(
            scene_context.model_loader.load_from_file<EmissiveEntityRenderer::VertexData>("cone.obj"),
            EmissiveEntityRenderer::InstanceData{
                glm::mat4{}, // Set via update_instance_data()
                EmissiveEntityRenderer::EmissiveEntityMaterial{
                    glm::vec4{1.0f}
                }
            },
            EmissiveEntityRenderer::RenderData{
                scene_context.texture_loader.default_white_texture()
            }
        )
    );

    light_element->update_instance_data();
    return light_element;
}

std::unique_ptr<EditorScene::DirectionalLightElement> EditorScene::DirectionalLightElement::from_json(const SceneContext& scene_context, EditorScene::ElementRef parent, const json& j) {
    auto light_element = new_default(scene_context, parent);

    light_element->direction = j["direction"];
    light_element->light->colour = j["colour"];
    light_element->visible = j["visible"];
    light_element->visual_length = j["visual_length"];


    light_element->update_instance_data();
    return light_element;
}

json EditorScene::DirectionalLightElement::into_json() const {
    return {
        {"direction",     direction},
        {"colour",       light->colour},
        {"visible",      visible},
        {"visual_length", visual_length},
    };
}

void EditorScene::DirectionalLightElement::add_imgui_edit_section(MasterRenderScene& render_scene, const SceneContext& scene_context) {
    ImGui::Text("Directional Light");
    SceneElement::add_imgui_edit_section(render_scene, scene_context);

    ImGui::Text("Direction");
    bool transformUpdated = false;

    glm::vec3 tempDir = direction;

    if (ImGui::DragFloat3("Direction", &tempDir[0], 0.01f)) {
        if (glm::length(tempDir) > 0.001f) {
            direction = glm::normalize(tempDir);
            transformUpdated = true;
        }
    }
    ImGui::DragDisableCursor(scene_context.window);
    ImGui::Spacing();

    ImGui::Text("Light Properties");
    transformUpdated |= ImGui::ColorEdit3("Colour", &light->colour[0]);
    ImGui::Spacing();
    ImGui::DragFloat("Intensity", &light->colour.a, 0.01f, 0.0f, FLT_MAX);
    ImGui::DragDisableCursor(scene_context.window);

    ImGui::Spacing();
    ImGui::Text("Visuals");

    transformUpdated |= ImGui::Checkbox("Show Visuals", &visible);
    transformUpdated |= ImGui::DragFloat("Visual Scale", &visual_length, 0.01f, 0.0f, 10.0f);
    ImGui::DragDisableCursor(scene_context.window);

    if (transformUpdated) {
        update_instance_data();
    }
}

void EditorScene::DirectionalLightElement::update_instance_data() {
    light -> direction = direction;

    if (visible) {
        // Post multiply by transform so that local transformations are applied first
        glm::vec3 default_direction = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 rotation_axis = glm::cross(default_direction, -direction);

        float rotation_angle = glm::angle(default_direction, -direction);

        if (glm::length(rotation_axis) < 0.001f) {
            rotation_axis = glm::vec3(0.0f, 1.0f, 0.0f);
            if (glm::dot(default_direction, -direction) < 0) {
                rotation_angle = glm::radians(180.0f);
            } else {
                rotation_angle = 0.0f;
            }
        }

        rotation_axis = glm::normalize(rotation_axis);
        glm::mat4 rotation_matrix = glm::rotate(rotation_angle, rotation_axis);
        glm::mat4 model_matrix = glm::scale(glm::vec3(0.2f, 0.2f, visual_length));
        glm::mat4 translation_matrix = glm::translate(glm::vec3(0.0f, 10.0f, 0.0f));
        model_matrix = rotation_matrix * translation_matrix * model_matrix;
        if (!EditorScene::is_null(parent)) {
            model_matrix = (*parent)->transform * model_matrix;
        }

        light_cone -> instance_data.model_matrix = model_matrix;
    } else {
        // Throw off to infinity as a hacky way to make model invisible
        light_cone -> instance_data.model_matrix = glm::scale(glm::vec3(0.0f));
    }
}

const char* EditorScene::DirectionalLightElement::element_type_name() const {
    return ELEMENT_TYPE_NAME;
}
