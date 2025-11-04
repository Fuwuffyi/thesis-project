#include "BaseScene.hpp"

#include "core/scene/Scene.hpp"
#include "core/scene/Node.hpp"

#include "core/resource/ResourceManager.hpp"

#include "core/scene/MeshLoaderHelper.hpp"

#include "core/scene/components/LightComponent.hpp"
#include "core/scene/components/ParticleSystemComponent.hpp"

void LoadSponzaGeometry(Scene& scene, ResourceManager& resourceManager);

// TODO: This is a test scene, add a serialization/deserialization system
void LoadBaseScene(Scene& scene, ResourceManager& resourceManager, const GraphicsAPI api) {
   LoadSponzaGeometry(scene, resourceManager);
   // Get light nodes
   std::vector<Node*> lightNodes;
   lightNodes.push_back(scene.FindNode("lamp_1stfloor_entrance"));
   for (uint32_t i = 0; i <= 12; ++i) {
      lightNodes.push_back(
         scene.FindNode(std::string("lamps_1stfloor_") + (i < 10 ? "0" : "") + std::to_string(i)));
   }
   for (uint32_t i = 1; i <= 8; ++i) {
      lightNodes.push_back(
         scene.FindNode(std::string("lamps_2ndfloor_") + (i < 10 ? "0" : "") + std::to_string(i)));
   }
   // Add lights
   std::random_device rd;
   std::mt19937 gen(rd());
   std::uniform_real_distribution<float> colorDist(0.3f, 1.0f);
   for (auto ln : lightNodes) {
      Node* n = scene.CreateChildNode(ln, "light_object");
      n->GetComponent<TransformComponent>()->SetPosition(glm::vec3(0.0f, 0.0f, -0.6f));
      LightComponent* light = n->AddComponent<LightComponent>();
      light->SetType(LightComponent::LightType::Spot);
      light->SetColor(glm::vec3(colorDist(gen), colorDist(gen), colorDist(gen)));
      light->SetIntensity(3.0f);
      light->SetOuterCone(glm::radians(35.0f));
      light->SetInnerCone(glm::radians(50.0f));
   }
   // Setup base particle node
   Node* particlesNode = scene.CreateNode("particles");
   particlesNode->GetComponent<TransformComponent>()->SetPosition(glm::vec3(0.0f, 2.0f, 0.0f));
   particlesNode->AddComponent<ParticleSystemComponent>();
}

void LoadSponzaGeometry(Scene& scene, ResourceManager& resourceManager) {
   const auto t1 = resourceManager.LoadTexture(
      "col_head_2ndfloor_03_BaseColor", "resources/textures/col_head_2ndfloor_03_BaseColor.png");
   const auto t2 = resourceManager.LoadTexture(
      "brickwall_02_Metalness", "resources/textures/brickwall_02_Metalness.png", true, false);
   const auto t3 = resourceManager.LoadTexture(
      "brickwall_02_Roughness", "resources/textures/brickwall_02_Roughness.png", true, false);
   const auto t4 = resourceManager.LoadTexture(
      "curtain_fabric_Normal", "resources/textures/curtain_fabric_Normal.png", true, false);
   const auto t5 = resourceManager.LoadTexture(
      "brickwall_01_Normal", "resources/textures/brickwall_01_Normal.png", true, false);
   const auto t6 = resourceManager.LoadTexture(
      "door_stoneframe_01_BaseColor", "resources/textures/door_stoneframe_01_BaseColor.png");
   const auto t7 = resourceManager.LoadTexture(
      "ornament_01_Roughness", "resources/textures/ornament_01_Roughness.png", true, false);
   const auto t8 = resourceManager.LoadTexture(
      "curtain_fabric_blue_BaseColor", "resources/textures/curtain_fabric_blue_BaseColor.png");
   const auto t10 = resourceManager.LoadTexture(
      "arch_stone_wall_01_Roughness", "resources/textures/arch_stone_wall_01_Roughness.png", true,
      false);
   const auto t11 = resourceManager.LoadTexture(
      "door_stoneframe_02_Normal", "resources/textures/door_stoneframe_02_Normal.png", true, false);
   const auto t13 = resourceManager.LoadTexture(
      "col_brickwall_01_BaseColor", "resources/textures/col_brickwall_01_BaseColor.png");
   const auto t14 = resourceManager.LoadTexture(
      "wood_door_01_Roughness", "resources/textures/wood_door_01_Roughness.png", true, false);
   const auto t15 = resourceManager.LoadTexture(
      "door_stoneframe_01_Metalness", "resources/textures/door_stoneframe_01_Metalness.png", true,
      false);
   const auto t16 = resourceManager.LoadTexture(
      "door_stoneframe_01_Roughness", "resources/textures/door_stoneframe_01_Roughness.png", true,
      false);
   const auto t18 = resourceManager.LoadTexture("brickwall_02_BaseColor",
                                                "resources/textures/brickwall_02_BaseColor.png");
   const auto t19 = resourceManager.LoadTexture(
      "col_head_2ndfloor_03_Roughness", "resources/textures/col_head_2ndfloor_03_Roughness.png",
      true, false);
   const auto t20 = resourceManager.LoadTexture(
      "col_1stfloor_Metalness", "resources/textures/col_1stfloor_Metalness.png", true, false);
   const auto t21 = resourceManager.LoadTexture(
      "roof_tiles_01_Roughness", "resources/textures/roof_tiles_01_Roughness.png", true, false);
   const auto t22 = resourceManager.LoadTexture(
      "ceiling_plaster_01_Metalness", "resources/textures/ceiling_plaster_01_Metalness.png", true,
      false);
   const auto t23 = resourceManager.LoadTexture(
      "col_1stfloor_Roughness", "resources/textures/col_1stfloor_Roughness.png", true, false);
   const auto t24 = resourceManager.LoadTexture(
      "floor_tiles_01_Roughness", "resources/textures/floor_tiles_01_Roughness.png", true, false);
   const auto t25 = resourceManager.LoadTexture("wood_door_01_BaseColor",
                                                "resources/textures/wood_door_01_BaseColor.png");
   const auto t26 = resourceManager.LoadTexture("stone_trims_01_BaseColor",
                                                "resources/textures/stone_trims_01_BaseColor.png");
   const auto t27 = resourceManager.LoadTexture(
      "col_head_2ndfloor_02_Normal", "resources/textures/col_head_2ndfloor_02_Normal.png", true,
      false);
   const auto t28 =
      resourceManager.LoadTexture("col_brickwall_01_Roughness",
                                  "resources/textures/col_brickwall_01_Roughness.png", true, false);
   const auto t29 = resourceManager.LoadTexture(
      "ceiling_plaster_01_BaseColor", "resources/textures/ceiling_plaster_01_BaseColor.png");
   const auto t30 = resourceManager.LoadTexture(
      "wood_tile_01_Metalness", "resources/textures/wood_tile_01_Metalness.png", true, false);
   const auto t31 = resourceManager.LoadTexture(
      "curtain_fabric_Metalness", "resources/textures/curtain_fabric_Metalness.png", true, false);
   const auto t32 = resourceManager.LoadTexture(
      "stone_01_tile_Roughness", "resources/textures/stone_01_tile_Roughness.png", true, false);
   const auto t33 =
      resourceManager.LoadTexture("col_brickwall_01_Metalness",
                                  "resources/textures/col_brickwall_01_Metalness.png", true, false);
   const auto t34 = resourceManager.LoadTexture(
      "roof_tiles_01_Metalness", "resources/textures/roof_tiles_01_Metalness.png", true, false);
   const auto t35 = resourceManager.LoadTexture("metal_door_01_BaseColor",
                                                "resources/textures/metal_door_01_BaseColor.png");
   const auto t36 = resourceManager.LoadTexture(
      "ornament_01_Metalness", "resources/textures/ornament_01_Metalness.png", true, false);
   const auto t37 = resourceManager.LoadTexture("stone_trims_02_BaseColor",
                                                "resources/textures/stone_trims_02_BaseColor.png");
   const auto t38 = resourceManager.LoadTexture(
      "ceiling_plaster_01_Roughness", "resources/textures/ceiling_plaster_01_Roughness.png", true,
      false);
   const auto t39 = resourceManager.LoadTexture(
      "ceiling_plaster_02_BaseColor", "resources/textures/ceiling_plaster_02_BaseColor.png");
   const auto t40 = resourceManager.LoadTexture("window_frame_01_BaseColor",
                                                "resources/textures/window_frame_01_BaseColor.png");
   const auto t41 = resourceManager.LoadTexture(
      "metal_door_01_Normal", "resources/textures/metal_door_01_Normal.png", true, false);
   const auto t42 = resourceManager.LoadTexture(
      "ornament_01_Normal", "resources/textures/ornament_01_Normal.png", true, false);
   const auto t43 = resourceManager.LoadTexture(
      "window_frame_01_Metalness", "resources/textures/window_frame_01_Metalness.png", true, false);
   const auto t44 = resourceManager.LoadTexture(
      "brickwall_02_Normal", "resources/textures/brickwall_02_Normal.png", true, false);
   const auto t45 = resourceManager.LoadTexture(
      "floor_tiles_01_Normal", "resources/textures/floor_tiles_01_Normal.png", true, false);
   const auto t46 = resourceManager.LoadTexture(
      "brickwall_01_Roughness", "resources/textures/brickwall_01_Roughness.png", true, false);
   const auto t47 = resourceManager.LoadTexture(
      "ceiling_plaster_02_Normal", "resources/textures/ceiling_plaster_02_Normal.png", true, false);
   const auto t48 = resourceManager.LoadTexture(
      "door_stoneframe_02_Roughness", "resources/textures/door_stoneframe_02_Roughness.png", true,
      false);
   const auto t49 = resourceManager.LoadTexture(
      "floor_tiles_01_Metalness", "resources/textures/floor_tiles_01_Metalness.png", true, false);
   const auto t50 = resourceManager.LoadTexture(
      "stone_01_tile_Metalness", "resources/textures/stone_01_tile_Metalness.png", true, false);
   const auto t51 = resourceManager.LoadTexture(
      "stones_2ndfloor_01_Normal", "resources/textures/stones_2ndfloor_01_Normal.png", true, false);
   const auto t52 = resourceManager.LoadTexture(
      "col_head_1stfloor_Roughness", "resources/textures/col_head_1stfloor_Roughness.png", true,
      false);
   const auto t53 = resourceManager.LoadTexture(
      "brickwall_01_Metalness", "resources/textures/brickwall_01_Metalness.png", true, false);
   const auto t54 = resourceManager.LoadTexture(
      "ceiling_plaster_02_Roughness", "resources/textures/ceiling_plaster_02_Roughness.png", true,
      false);
   const auto t55 = resourceManager.LoadTexture(
      "col_brickwall_01_Normal", "resources/textures/col_brickwall_01_Normal.png", true, false);
   const auto t56 = resourceManager.LoadTexture(
      "stone_trims_01_Metalness", "resources/textures/stone_trims_01_Metalness.png", true, false);
   const auto t57 = resourceManager.LoadTexture(
      "arch_stone_wall_01_BaseColor", "resources/textures/arch_stone_wall_01_BaseColor.png");
   const auto t58 = resourceManager.LoadTexture(
      "ceiling_plaster_02_Metalness", "resources/textures/ceiling_plaster_02_Metalness.png", true,
      false);
   const auto t59 = resourceManager.LoadTexture("lionhead_01_BaseColor",
                                                "resources/textures/lionhead_01_BaseColor.png");
   const auto t60 = resourceManager.LoadTexture(
      "arch_stone_wall_01_Metalness", "resources/textures/arch_stone_wall_01_Metalness.png", true,
      false);
   const auto t61 = resourceManager.LoadTexture(
      "wood_tile_01_Roughness", "resources/textures/wood_tile_01_Roughness.png", true, false);
   const auto t62 = resourceManager.LoadTexture(
      "col_head_1stfloor_BaseColor", "resources/textures/col_head_1stfloor_BaseColor.png");
   const auto t63 = resourceManager.LoadTexture(
      "lionhead_01_Normal", "resources/textures/lionhead_01_Normal.png", true, false);
   const auto t64 = resourceManager.LoadTexture("brickwall_01_BaseColor",
                                                "resources/textures/brickwall_01_BaseColor.png");
   const auto t65 = resourceManager.LoadTexture("stone_01_tile_BaseColor",
                                                "resources/textures/stone_01_tile_BaseColor.png");
   const auto t66 = resourceManager.LoadTexture(
      "window_frame_01_Normal", "resources/textures/window_frame_01_Normal.png", true, false);
   const auto t67 = resourceManager.LoadTexture(
      "curtain_fabric_green_BaseColor", "resources/textures/curtain_fabric_green_BaseColor.png");
   const auto t68 = resourceManager.LoadTexture(
      "wood_tile_01_Normal", "resources/textures/wood_tile_01_Normal.png", true, false);
   const auto t69 = resourceManager.LoadTexture(
      "col_head_1stfloor_Metalness", "resources/textures/col_head_1stfloor_Metalness.png", true,
      false);
   const auto t70 = resourceManager.LoadTexture(
      "col_head_1stfloor_Normal", "resources/textures/col_head_1stfloor_Normal.png", true, false);
   const auto t71 = resourceManager.LoadTexture(
      "arch_stone_wall_01_Normal", "resources/textures/arch_stone_wall_01_Normal.png", true, false);
   const auto t72 = resourceManager.LoadTexture(
      "curtain_fabric_Roughness", "resources/textures/curtain_fabric_Roughness.png", true, false);
   const auto t73 = resourceManager.LoadTexture(
      "wood_door_01_Metalness", "resources/textures/wood_door_01_Metalness.png", true, false);
   const auto t74 = resourceManager.LoadTexture(
      "stone_trims_01_Roughness", "resources/textures/stone_trims_01_Roughness.png", true, false);
   const auto t75 = resourceManager.LoadTexture(
      "door_stoneframe_02_BaseColor", "resources/textures/door_stoneframe_02_BaseColor.png");
   const auto t76 = resourceManager.LoadTexture(
      "stone_trims_02_Normal", "resources/textures/stone_trims_02_Normal.png", true, false);
   const auto t77 = resourceManager.LoadTexture(
      "col_head_2ndfloor_03_Normal", "resources/textures/col_head_2ndfloor_03_Normal.png", true,
      false);
   const auto t78 = resourceManager.LoadTexture(
      "stones_2ndfloor_01_Metalness", "resources/textures/stones_2ndfloor_01_Metalness.png", true,
      false);
   const auto t79 = resourceManager.LoadTexture(
      "metal_door_01_Roughness", "resources/textures/metal_door_01_Roughness.png", true, false);
   const auto t80 = resourceManager.LoadTexture(
      "roof_tiles_01_Normal", "resources/textures/roof_tiles_01_Normal.png", true, false);
   const auto t81 = resourceManager.LoadTexture(
      "window_frame_01_Roughness", "resources/textures/window_frame_01_Roughness.png", true, false);
   const auto t82 = resourceManager.LoadTexture("col_1stfloor_BaseColor",
                                                "resources/textures/col_1stfloor_BaseColor.png");
   const auto t83 = resourceManager.LoadTexture(
      "curtain_fabric_red_BaseColor", "resources/textures/curtain_fabric_red_BaseColor.png");
   const auto t84 = resourceManager.LoadTexture(
      "col_head_2ndfloor_02_Roughness", "resources/textures/col_head_2ndfloor_02_Roughness.png",
      true, false);
   const auto t85 = resourceManager.LoadTexture(
      "col_head_2ndfloor_02_Metalness", "resources/textures/col_head_2ndfloor_02_Metalness.png",
      true, false);
   const auto t86 = resourceManager.LoadTexture(
      "wood_door_01_Normal", "resources/textures/wood_door_01_Normal.png", true, false);
   const auto t87 = resourceManager.LoadTexture(
      "ceiling_plaster_01_Normal", "resources/textures/ceiling_plaster_01_Normal.png", true, false);
   const auto t88 = resourceManager.LoadTexture("ornament_01_BaseColor",
                                                "resources/textures/ornament_01_BaseColor.png");
   const auto t89 = resourceManager.LoadTexture(
      "stone_trims_02_Roughness", "resources/textures/stone_trims_02_Roughness.png", true, false);
   const auto t90 = resourceManager.LoadTexture(
      "col_head_2ndfloor_03_Metalness", "resources/textures/col_head_2ndfloor_03_Metalness.png",
      true, false);
   const auto t91 = resourceManager.LoadTexture(
      "lionhead_01_Metalness", "resources/textures/lionhead_01_Metalness.png", true, false);
   const auto t92 = resourceManager.LoadTexture(
      "lionhead_01_Roughness", "resources/textures/lionhead_01_Roughness.png", true, false);
   const auto t93 = resourceManager.LoadTexture(
      "door_stoneframe_02_Metalness", "resources/textures/door_stoneframe_02_Metalness.png", true,
      false);
   const auto t94 = resourceManager.LoadTexture(
      "stone_trims_01_Normal", "resources/textures/stone_trims_01_Normal.png", true, false);
   const auto t95 = resourceManager.LoadTexture(
      "col_head_2ndfloor_02_BaseColor", "resources/textures/col_head_2ndfloor_02_BaseColor.png");
   const auto t96 = resourceManager.LoadTexture(
      "stones_2ndfloor_01_Roughness", "resources/textures/stones_2ndfloor_01_Roughness.png", true,
      false);
   const auto t97 = resourceManager.LoadTexture("wood_tile_01_BaseColor",
                                                "resources/textures/wood_tile_01_BaseColor.png");
   const auto t98 = resourceManager.LoadTexture(
      "door_stoneframe_01_Normal", "resources/textures/door_stoneframe_01_Normal.png", true, false);
   const auto t99 = resourceManager.LoadTexture("floor_tiles_01_BaseColor",
                                                "resources/textures/floor_tiles_01_BaseColor.png");
   const auto t100 = resourceManager.LoadTexture("roof_tiles_01_BaseColor",
                                                 "resources/textures/roof_tiles_01_BaseColor.png");
   const auto t101 = resourceManager.LoadTexture(
      "metal_door_01_Metalness", "resources/textures/metal_door_01_Metalness.png", true, false);
   const auto t102 = resourceManager.LoadTexture(
      "col_1stfloor_Normal", "resources/textures/col_1stfloor_Normal.png", true, false);
   const auto t103 = resourceManager.LoadTexture(
      "stone_trims_02_Metalness", "resources/textures/stone_trims_02_Metalness.png", true, false);
   const auto t104 = resourceManager.LoadTexture(
      "stones_2ndfloor_01_BaseColor", "resources/textures/stones_2ndfloor_01_BaseColor.png");
   const auto t105 = resourceManager.LoadTexture(
      "stone_01_tile_Normal", "resources/textures/stone_01_tile_Normal.png", true, false);
   // Create the model's materials
   const auto m1 = resourceManager.CreateMaterial("material_1", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m1)) {
      mat->SetTexture("albedoTexture", t57);
      mat->SetTexture("normalTexture", t71);
      mat->SetTexture("roughnessTexture", t10);
      mat->SetTexture("metallicTexture", t60);
   }
   const auto m2 = resourceManager.CreateMaterial("material_2", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m2)) {
      mat->SetTexture("albedoTexture", t26);
      mat->SetTexture("normalTexture", t94);
      mat->SetTexture("roughnessTexture", t74);
      mat->SetTexture("metallicTexture", t56);
   }
   const auto m3 = resourceManager.CreateMaterial("material_3", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m3)) {
      mat->SetTexture("albedoTexture", t35);
      mat->SetTexture("normalTexture", t41);
      mat->SetTexture("roughnessTexture", t79);
      mat->SetTexture("metallicTexture", t101);
   }
   const auto m4 = resourceManager.CreateMaterial("material_4", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m4)) {
      mat->SetTexture("albedoTexture", t18);
      mat->SetTexture("normalTexture", t44);
      mat->SetTexture("roughnessTexture", t3);
      mat->SetTexture("metallicTexture", t2);
   }
   const auto m5 = resourceManager.CreateMaterial("material_5", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m5)) {
      mat->SetTexture("albedoTexture", t57);
      mat->SetTexture("normalTexture", t71);
      mat->SetTexture("roughnessTexture", t10);
      mat->SetTexture("metallicTexture", t60);
   }
   const auto m6 = resourceManager.CreateMaterial("material_6", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m6)) {
      mat->SetTexture("albedoTexture", t26);
      mat->SetTexture("normalTexture", t94);
      mat->SetTexture("roughnessTexture", t74);
      mat->SetTexture("metallicTexture", t56);
   }
   const auto m7 = resourceManager.CreateMaterial("material_7", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m7)) {
      mat->SetTexture("albedoTexture", t40);
      mat->SetTexture("normalTexture", t66);
      mat->SetTexture("roughnessTexture", t81);
      mat->SetTexture("metallicTexture", t43);
   }
   const auto m8 = resourceManager.CreateMaterial("material_8", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m8)) {
      mat->SetTexture("albedoTexture", t65);
      mat->SetTexture("normalTexture", t105);
      mat->SetTexture("roughnessTexture", t32);
      mat->SetTexture("metallicTexture", t50);
   }
   const auto m9 = resourceManager.CreateMaterial("material_9", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m9)) {
      mat->SetTexture("albedoTexture", t99);
      mat->SetTexture("normalTexture", t45);
      mat->SetTexture("roughnessTexture", t24);
      mat->SetTexture("metallicTexture", t49);
   }
   const auto m10 = resourceManager.CreateMaterial("material_10", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m10)) {
      mat->SetTexture("albedoTexture", t104);
      mat->SetTexture("normalTexture", t51);
      mat->SetTexture("roughnessTexture", t96);
      mat->SetTexture("metallicTexture", t78);
   }
   const auto m11 = resourceManager.CreateMaterial("material_11", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m11)) {
      mat->SetTexture("albedoTexture", t35);
      mat->SetTexture("normalTexture", t41);
      mat->SetTexture("roughnessTexture", t79);
      mat->SetTexture("metallicTexture", t101);
   }
   const auto m12 = resourceManager.CreateMaterial("material_12", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m12)) {
      mat->SetParameter("albedo", glm::vec3(1.0f, 1.0f, 1.0f));
      mat->SetParameter("roughness", 0.1f);
      mat->SetParameter("metallic", 0.0f);
   }
   const auto m13 = resourceManager.CreateMaterial("material_13", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m13)) {
      mat->SetTexture("albedoTexture", t18);
      mat->SetTexture("normalTexture", t44);
      mat->SetTexture("roughnessTexture", t3);
      mat->SetTexture("metallicTexture", t2);
   }
   const auto m14 = resourceManager.CreateMaterial("material_14", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m14)) {
      mat->SetTexture("albedoTexture", t39);
      mat->SetTexture("normalTexture", t87);
      mat->SetTexture("roughnessTexture", t38);
      mat->SetTexture("metallicTexture", t22);
   }
   const auto m15 = resourceManager.CreateMaterial("material_15", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m15)) {
      mat->SetTexture("albedoTexture", t104);
      mat->SetTexture("normalTexture", t51);
      mat->SetTexture("roughnessTexture", t96);
      mat->SetTexture("metallicTexture", t78);
   }
   const auto m16 = resourceManager.CreateMaterial("material_16", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m16)) {
      mat->SetTexture("albedoTexture", t37);
      mat->SetTexture("normalTexture", t76);
      mat->SetTexture("roughnessTexture", t89);
      mat->SetTexture("metallicTexture", t103);
   }
   const auto m17 = resourceManager.CreateMaterial("material_17", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m17)) {
      mat->SetTexture("albedoTexture", t39);
      mat->SetTexture("normalTexture", t87);
      mat->SetTexture("roughnessTexture", t54);
      mat->SetTexture("metallicTexture", t58);
   }
   const auto m18 = resourceManager.CreateMaterial("material_18", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m18)) {
      mat->SetTexture("albedoTexture", t40);
      mat->SetTexture("normalTexture", t66);
      mat->SetTexture("roughnessTexture", t81);
      mat->SetTexture("metallicTexture", t43);
   }
   const auto m19 = resourceManager.CreateMaterial("material_19", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m19)) {
      mat->SetTexture("albedoTexture", t65);
      mat->SetTexture("normalTexture", t105);
      mat->SetTexture("roughnessTexture", t32);
      mat->SetTexture("metallicTexture", t50);
   }
   const auto m20 = resourceManager.CreateMaterial("material_20", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m20)) {
      mat->SetTexture("albedoTexture", t82);
      mat->SetTexture("normalTexture", t102);
      mat->SetTexture("roughnessTexture", t23);
      mat->SetTexture("metallicTexture", t20);
   }
   const auto m21 = resourceManager.CreateMaterial("material_21", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m21)) {
      mat->SetTexture("albedoTexture", t62);
      mat->SetTexture("normalTexture", t70);
      mat->SetTexture("roughnessTexture", t52);
      mat->SetTexture("metallicTexture", t69);
   }
   const auto m22 = resourceManager.CreateMaterial("material_22", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m22)) {
      mat->SetTexture("albedoTexture", t64);
      mat->SetTexture("normalTexture", t5);
      mat->SetTexture("roughnessTexture", t46);
      mat->SetTexture("metallicTexture", t53);
   }
   const auto m23 = resourceManager.CreateMaterial("material_23", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m23)) {
      mat->SetParameter("albedo", glm::vec3(1.0f, 1.0f, 1.0f));
      mat->SetParameter("roughness", 0.1f);
      mat->SetParameter("metallic", 0.0f);
   }
   const auto m24 = resourceManager.CreateMaterial("material_24", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m24)) {
      mat->SetParameter("albedo", glm::vec3(1.0f, 1.0f, 1.0f));
      mat->SetParameter("roughness", 0.1f);
      mat->SetParameter("metallic", 0.0f);
   }
   const auto m25 = resourceManager.CreateMaterial("material_25", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m25)) {
      mat->SetTexture("albedoTexture", t59);
      mat->SetTexture("normalTexture", t63);
      mat->SetTexture("roughnessTexture", t92);
      mat->SetTexture("metallicTexture", t91);
   }
   const auto m26 = resourceManager.CreateMaterial("material_26", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m26)) {
      mat->SetTexture("albedoTexture", t75);
      mat->SetTexture("normalTexture", t11);
      mat->SetTexture("roughnessTexture", t48);
      mat->SetTexture("metallicTexture", t93);
   }
   const auto m27 = resourceManager.CreateMaterial("material_27", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m27)) {
      mat->SetParameter("albedo", glm::vec3(1.0f, 1.0f, 1.0f));
      mat->SetParameter("roughness", 0.1f);
      mat->SetParameter("metallic", 0.0f);
   }
   const auto m28 = resourceManager.CreateMaterial("door_stoneframe_01", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m28)) {
      mat->SetTexture("albedoTexture", t6);
      mat->SetTexture("normalTexture", t98);
      mat->SetTexture("roughnessTexture", t16);
      mat->SetTexture("metallicTexture", t15);
   }
   const auto m29 = resourceManager.CreateMaterial("material_29", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m29)) {
      mat->SetTexture("albedoTexture", t64);
      mat->SetTexture("normalTexture", t44);
      mat->SetTexture("roughnessTexture", t46);
      mat->SetTexture("metallicTexture", t53);
   }
   const auto m30 = resourceManager.CreateMaterial("material_30", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m30)) {
      mat->SetTexture("albedoTexture", t37);
      mat->SetTexture("normalTexture", t76);
      mat->SetTexture("roughnessTexture", t89);
      mat->SetTexture("metallicTexture", t103);
   }
   const auto m31 = resourceManager.CreateMaterial("material_31", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m31)) {
      mat->SetTexture("albedoTexture", t75);
      mat->SetTexture("normalTexture", t11);
      mat->SetTexture("roughnessTexture", t48);
      mat->SetTexture("metallicTexture", t93);
   }
   const auto m32 = resourceManager.CreateMaterial("material_32", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m32)) {
      mat->SetTexture("albedoTexture", t25);
      mat->SetTexture("normalTexture", t86);
      mat->SetTexture("roughnessTexture", t14);
      mat->SetTexture("metallicTexture", t73);
   }
   const auto m33 = resourceManager.CreateMaterial("material_33", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m33)) {
      mat->SetTexture("albedoTexture", t97);
      mat->SetTexture("normalTexture", t68);
      mat->SetTexture("roughnessTexture", t61);
      mat->SetTexture("metallicTexture", t30);
   }
   const auto m34 = resourceManager.CreateMaterial("material_34", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m34)) {
      mat->SetTexture("albedoTexture", t29);
      mat->SetTexture("normalTexture", t87);
      mat->SetTexture("roughnessTexture", t38);
      mat->SetTexture("metallicTexture", t22);
   }
   const auto m35 = resourceManager.CreateMaterial("material_35", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m35)) {
      mat->SetTexture("albedoTexture", t13);
      mat->SetTexture("normalTexture", t55);
      mat->SetTexture("roughnessTexture", t28);
      mat->SetTexture("metallicTexture", t33);
   }
   const auto m36 = resourceManager.CreateMaterial("material_36", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m36)) {
      mat->SetTexture("albedoTexture", t1);
      mat->SetTexture("normalTexture", t77);
      mat->SetTexture("roughnessTexture", t19);
      mat->SetTexture("metallicTexture", t90);
   }
   const auto m37 = resourceManager.CreateMaterial("material_37", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m37)) {
      mat->SetTexture("albedoTexture", t95);
      mat->SetTexture("normalTexture", t27);
      mat->SetTexture("roughnessTexture", t84);
      mat->SetTexture("metallicTexture", t85);
   }
   const auto m38 = resourceManager.CreateMaterial("material_38", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m38)) {
      mat->SetTexture("albedoTexture", t99);
      mat->SetTexture("normalTexture", t45);
      mat->SetTexture("roughnessTexture", t24);
      mat->SetTexture("metallicTexture", t49);
   }
   const auto m39 = resourceManager.CreateMaterial("material_39", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m39)) {
      mat->SetTexture("albedoTexture", t6);
      mat->SetTexture("normalTexture", t98);
      mat->SetTexture("roughnessTexture", t16);
      mat->SetTexture("metallicTexture", t15);
   }
   const auto m40 = resourceManager.CreateMaterial("roof_tiles", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m40)) {
      mat->SetTexture("albedoTexture", t100);
      mat->SetTexture("normalTexture", t80);
      mat->SetTexture("roughnessTexture", t21);
      mat->SetTexture("metallicTexture", t34);
   }
   const auto m41 = resourceManager.CreateMaterial("ornament", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m41)) {
      mat->SetTexture("albedoTexture", t88);
      mat->SetTexture("normalTexture", t42);
      mat->SetTexture("roughnessTexture", t7);
      mat->SetTexture("metallicTexture", t36);
   }
   const auto m42 = resourceManager.CreateMaterial("curtain_red", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m42)) {
      mat->SetTexture("albedoTexture", t83);
      mat->SetTexture("normalTexture", t4);
      mat->SetTexture("roughnessTexture", t72);
      mat->SetTexture("metallicTexture", t31);
   }
   const auto m43 = resourceManager.CreateMaterial("material_43", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m43)) {
      mat->SetTexture("albedoTexture", t35);
      mat->SetTexture("normalTexture", t41);
      mat->SetTexture("roughnessTexture", t79);
      mat->SetTexture("metallicTexture", t101);
   }
   const auto m44 = resourceManager.CreateMaterial("curtain_blue", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m44)) {
      mat->SetTexture("albedoTexture", t8);
      mat->SetTexture("normalTexture", t4);
      mat->SetTexture("roughnessTexture", t72);
      mat->SetTexture("metallicTexture", t31);
   }
   const auto m45 = resourceManager.CreateMaterial("curtain_green", "PBR");
   if (const auto mat = resourceManager.GetMaterial(m45)) {
      mat->SetTexture("albedoTexture", t67);
      mat->SetTexture("normalTexture", t4);
      mat->SetTexture("roughnessTexture", t72);
      mat->SetTexture("metallicTexture", t31);
   }
   // Load the main model
   Node* sponzaNode = MeshLoaderHelper::LoadSceneAsChildNode(
      scene, scene.GetRootNode(), resourceManager, "sponza", "resources/meshes/sponza.fbx", {},
      {m1,  m2,  m3,  m4,  m5,  m6,  m7,  m8,  m9,  m10, m11, m12, m13, m14, m15,
       m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30,
       m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45});
   sponzaNode->GetTransform()->SetScale(glm::vec3(0.01f));
   // Add a directional light for sun
   Node* sunNode = scene.CreateNode("light_sun");
   TransformComponent* sunTransform = sunNode->GetComponent<TransformComponent>();
   sunTransform->SetRotation(glm::vec3(-45.0f, 45.0f, 0.0f));
   LightComponent* lightSun = sunNode->AddComponent<LightComponent>();
   lightSun->SetType(LightComponent::LightType::Directional);
   lightSun->SetColor(glm::vec3(1.0f, 1.0f, 0.95f));
   lightSun->SetIntensity(1.0f);
}
