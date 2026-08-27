#include "hzpch.h"
#include "EntityInspectorPanel.h"
#include "RenderStatsPanel.h"
#include "SceneHierarchyPanel.h"

#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"

#include "Hazel/Renderer/Model/Import/ModelImportTypes.h"
#include "Hazel/AssetsSystem/AssetManager.h"
#include "Hazel/Profile/RenderGPUProfiler.h"

#include <algorithm>
#include <limits>

namespace
{
	struct TimingStatistics
	{
		uint32_t SampleCount = 0;
		double TotalMs = 0.0;
		double MinMs = std::numeric_limits<double>::max();
		double MaxMs = 0.0;

		void Reset()
		{
			SampleCount = 0;
			TotalMs = 0.0;
			MinMs = std::numeric_limits<double>::max();
			MaxMs = 0.0;
		}

		void Add(double timeMs)
		{
			++SampleCount;
			TotalMs += timeMs;
			MinMs = std::min(MinMs, timeMs);
			MaxMs = std::max(MaxMs, timeMs);
		}

		double GetAverageMs() const
		{
			return SampleCount == 0 ? 0.0 : TotalMs / SampleCount;
		}
	};

	struct PassTimingStatistics
	{
		TimingStatistics CPU;
		TimingStatistics GPU;

		void Add(const Engine::RenderPassProfile& profile)
		{
			if (profile.Executed)
				CPU.Add(profile.CpuTimeMs);
			if (profile.GPUExecuted)
				GPU.Add(profile.GPUTimeMs);
		}
	};

	struct RenderProfileStatistics
	{
		PassTimingStatistics Frame;
		PassTimingStatistics SceneCSMShadow;
		PassTimingStatistics SceneShadow2D;
		PassTimingStatistics ScenePointShadow;
		PassTimingStatistics CharacterShadow;
		PassTimingStatistics GBuffer;
		PassTimingStatistics SSAO;
		PassTimingStatistics Lighting;
		PassTimingStatistics ScreenSpaceOutline;
		PassTimingStatistics InvertedHullOutline;
		PassTimingStatistics Character;
		PassTimingStatistics GeometryOutline;
		PassTimingStatistics SSR;
		PassTimingStatistics Transparent;
		PassTimingStatistics TAA;
		PassTimingStatistics Bloom;
		PassTimingStatistics ToneMapping;

		void Reset()
		{
			*this = {};
		}

		void Add(const Engine::RenderProfile& profile)
		{
			Frame.Add(profile.Frame);
			SceneCSMShadow.Add(profile.SceneCSMShadow);
			SceneShadow2D.Add(profile.SceneShadow2D);
			ScenePointShadow.Add(profile.ScenePointShadow);
			CharacterShadow.Add(profile.CharacterShadow);
			GBuffer.Add(profile.GBuffer);
			SSAO.Add(profile.SSAO);
			Lighting.Add(profile.Lighting);
			ScreenSpaceOutline.Add(profile.ScreenSpaceOutline);
			InvertedHullOutline.Add(profile.InvertedHullOutline);
			Character.Add(profile.Character);
			GeometryOutline.Add(profile.GeometryOutline);
			SSR.Add(profile.SSR);
			Transparent.Add(profile.Transparent);
			TAA.Add(profile.TAA);
			Bloom.Add(profile.Bloom);
			ToneMapping.Add(profile.ToneMapping);
		}
	};
}

namespace Engine
{
	const char* EntityInspectorPanel::blendNames[] =
	{
		"Opaque", "Alpha Cutout", "Alpha Blend"
	};

	const char* EntityInspectorPanel::cullNames[] =
	{
		"None", "Back", "Front"
	};

	const char* EntityInspectorPanel::lightNames[] =
	{
		"Directional", "Point", "Spot"
	};
	
	const char* RenderStatsPanel::RenderStatsName[] =
	{
		"Lit", "BaseColor", "WorldNormal",
		"Emissive", "Alpha", "CSM"
	};

	const char* RenderSettingPanel::LightingModesName[] =
	{
		"PBR", "Toon"
	};

	const char* RenderSettingPanel::RenderQualityPresetNames[] =
	{
		"Low", "High"
	};

	const char* RenderSettingPanel::ToneMappingOperatorNames[] =
	{
		"Bypass",
		"Reinhard",
		"Reinhard Extended",
		"ACES Fitted",
		"PBR Neutral"
	};

	void RenderSettingPanel::OnImGuiRender(RenderView& view)
	{
		ImGui::Begin("Render Setting");

		int qualityPreset = static_cast<int>(view.QualityPreset);
		if (ImGui::Combo("Quality Preset", &qualityPreset,
			RenderQualityPresetNames, IM_ARRAYSIZE(RenderQualityPresetNames)))
		{
			view.QualityPreset = static_cast<RenderQualityPreset>(qualityPreset);
		}
		ImGui::Separator();

		ImGui::Text("Light Model");
		int lightMode = static_cast<int>(view.LightingMode);
		if(ImGui::Combo("Lighting Model Setting", &lightMode, LightingModesName, IM_ARRAYSIZE(LightingModesName)))
		{
			view.LightingMode = static_cast<LightingMode>(lightMode);
		}

		if (view.LightingMode == LightingMode::Toon)
		{
			float threshold = view.ToonShadowThreshold;
			float softness = view.ToonSoftness;
			glm::vec3 shadowTint = view.ToonShadowTint;
			float lightLevel = view.ToonLitLeval;
			float shadowLevel = view.ToonShadowLevel;

			ImGui::Text("Toon Lighing Setting");
			if (ImGui::ColorEdit3("Shadow Color", glm::value_ptr(shadowTint)))
				view.ToonShadowTint = shadowTint;

			//if (ImGui::SliderFloat("Shadow Threshold", &threshold, 0.01f, 1.0f))
			//	view.ToonShadowThreshold = threshold;

			//if (ImGui::SliderFloat("Shadow Softness", &softness, 0.01f, 0.5f))
			//	view.ToonSoftness = softness;

			//if (ImGui::SliderFloat("Toon Light Level", &lightLevel, 0.5f, 0.9f))
			//	view.ToonLitLeval = lightLevel;

			//if (ImGui::SliderFloat("Toon Shadow Level", &shadowLevel, 0.1f, 0.3f))
			//	view.ToonShadowLevel = shadowLevel;

		}

		ImGui::Checkbox("CSM Enabled", &view.CSMEnabled);
		ImGui::SliderFloat("CSM Overlap Ratio", &view.OverlapRatio, 0.0f, 1.0f);

		if (ImGui::CollapsingHeader("Ambient Occlusion", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& ssao = view.SSAO;
			ImGui::Checkbox("Enable SSAO", &ssao.Enabled);
			ImGui::DragFloat("SSAO Radius", &ssao.Radius, 0.01f, 0.01f, 5.0f, "%.2f");
			ImGui::DragFloat("SSAO Depth Bias", &ssao.DepthBias, 0.001f, 0.0f, 0.2f, "%.3f");
			ImGui::SliderFloat("SSAO Intensity", &ssao.Intensity, 0.0f, 4.0f, "%.2f");
		}

		ImGui::ColorEdit3("Outline Color", glm::value_ptr(view.OutlineColor));

		ImGui::SliderFloat("Outline Depth Threshold", &view.OutlineDepthThreshold, 0.0001f, 0.1f);
		ImGui::SliderFloat("Outline Normal Threshold", &view.OutlineNormalThreshold, 0.001f, 0.1f);
		ImGui::SliderFloat("Outline Alpha", &view.OutlineAlpha, 0.0f, 1.0f);

		if (ImGui::CollapsingHeader("Post Processing", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Enable TAA", &view.TAAEnabled);
			ImGui::Separator();

			auto& toneMapping = view.PostProcess.ToneMapping;
			ImGui::Checkbox("Enable Tone Mapping", &toneMapping.Enabled);

			int toneMappingOperator = static_cast<int>(toneMapping.Operator);
			if (ImGui::Combo(
				"Tone Mapping Operator",
				&toneMappingOperator,
				ToneMappingOperatorNames,
				IM_ARRAYSIZE(ToneMappingOperatorNames)))
			{
				toneMapping.Operator = static_cast<ToneMappingOperator>(toneMappingOperator);
			}

			ImGui::SliderFloat(
				"Exposure Compensation",
				&toneMapping.Exposure.CompensationEV,
				-8.0f,
				8.0f,
				"%.2f EV");

			if (toneMapping.Operator == ToneMappingOperator::ReinhardExtended)
			{
				ImGui::DragFloat(
					"Reinhard White Point",
					&toneMapping.Reinhard.WhitePoint,
					0.05f,
					0.01f,
					100.0f,
					"%.2f");
			}

			ImGui::Separator();
			ImGui::TextUnformatted("Bloom");

			auto& bloom = view.PostProcess.Bloom;
			ImGui::Checkbox("Enable Bloom", &bloom.Enabled);
			ImGui::SliderFloat("Bloom Threshold", &bloom.Threshold, 0.0f, 10.0f, "%.2f");
			ImGui::SliderFloat("Bloom Knee", &bloom.Knee, 0.0f, 5.0f, "%.2f");
			ImGui::SliderFloat("Bloom Intensity", &bloom.Intensity, 0.0f, 5.0f, "%.2f");
			ImGui::SliderFloat("Bloom Scatter", &bloom.Scatter, 0.0f, 1.0f, "%.2f");

			int maxMipLevels = static_cast<int>(bloom.MaxMipLevels);
			if (ImGui::SliderInt("Bloom Max Mip Levels", &maxMipLevels, 1, 10))
				bloom.MaxMipLevels = static_cast<uint32_t>(maxMipLevels);
		}

		ImGui::End();
	}

	void RenderStatsPanel::OnImGuiRender(RenderDebugSetting& debug, RendererLog& log)
	{
		constexpr uint32_t WarmupFrameCount = 120;
		constexpr uint32_t SampleFrameCount = 1000;
		static RenderProfileStatistics timingStatistics;
		static uint32_t warmupFrames = 0;
		static uint32_t sampledFrames = 0;
		static bool benchmarkRunning = false;

		if (benchmarkRunning)
		{
			if (warmupFrames < WarmupFrameCount)
			{
				++warmupFrames;
			}
			else
			{
				timingStatistics.Add(log.Profile);
				++sampledFrames;
				if (sampledFrames >= SampleFrameCount)
					benchmarkRunning = false;
			}
		}

		ImGui::Begin("Render Debug");

		int debugMode = static_cast<int>(debug.View);
		if (ImGui::Combo("Debug View Setting", &debugMode, RenderStatsName, IM_ARRAYSIZE(RenderStatsName)))
		{
			debug.View = static_cast<RenderDebugView>(debugMode);
		}
		ImGui::Checkbox("Wire Frame", &debug.Wireframe);

		ImGui::Text("Draw Calls: %u", log.DrawCalls);
		ImGui::Text("Opaque Items: %u", log.OpaqueDrawItems);
		ImGui::Text("Transparent Items: %u", log.TransparentDrawItems);
		ImGui::Text("Lights: %u", log.LightCount);
		if (log.Profile.Frame.GPUExecuted)
			ImGui::Text("Latest GPU Frame: %.3f ms", log.Profile.Frame.GPUTimeMs);

		if (ImGui::BeginTable("Pass Visibility Statistics", 5,
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Pass");
			ImGui::TableSetupColumn("Input");
			ImGui::TableSetupColumn("Visible");
			ImGui::TableSetupColumn("Culled");
			ImGui::TableSetupColumn("Cull Rate");
			ImGui::TableHeadersRow();

			auto drawVisibility = [](const char* name, const RenderPassProfile& profile)
			{
				const auto& statistics = profile.ItemStatistics;
				const float cullRate = statistics.InputItems == 0
					? 0.0f
					: 100.0f * static_cast<float>(statistics.CulledItems) /
						static_cast<float>(statistics.InputItems);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(name);
				ImGui::TableNextColumn();
				ImGui::Text("%u", statistics.InputItems);
				ImGui::TableNextColumn();
				ImGui::Text("%u", statistics.VisibleItems);
				ImGui::TableNextColumn();
				ImGui::Text("%u", statistics.CulledItems);
				ImGui::TableNextColumn();
				ImGui::Text("%.1f%%", cullRate);
			};

			drawVisibility("GBuffer", log.Profile.GBuffer);
			drawVisibility("Inverted Hull", log.Profile.InvertedHullOutline);
			drawVisibility("Character", log.Profile.Character);
			drawVisibility("Geometry Outline", log.Profile.GeometryOutline);
			drawVisibility("Transparent", log.Profile.Transparent);

			ImGui::EndTable();
		}

		ImGui::Separator();
		bool gpuProfilingEnabled = RenderGPUProfiler::IsEnabled();
		if (ImGui::Checkbox("Enable GPU Profiling", &gpuProfilingEnabled))
			RenderGPUProfiler::SetEnabled(gpuProfilingEnabled);

		if (ImGui::Button("Benchmark Pass CPU/GPU (1000 Frames)"))
		{
			RenderGPUProfiler::SetEnabled(true);
			timingStatistics.Reset();
			warmupFrames = 0;
			sampledFrames = 0;
			benchmarkRunning = true;
		}

		ImGui::SameLine();
		if (benchmarkRunning && warmupFrames < WarmupFrameCount)
			ImGui::Text("Warmup %u / %u", warmupFrames, WarmupFrameCount);
		else
			ImGui::Text("Samples %u / %u", sampledFrames, SampleFrameCount);

		if (ImGui::BeginTable("Pass CPU Timing", 5,
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Pass");
			ImGui::TableSetupColumn("Avg ms");
			ImGui::TableSetupColumn("Min ms");
			ImGui::TableSetupColumn("Max ms");
			ImGui::TableSetupColumn("Samples");
			ImGui::TableHeadersRow();

			auto drawTiming = [](const char* name, const TimingStatistics& timing)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(name);
				ImGui::TableNextColumn();
				ImGui::Text("%.5f", timing.GetAverageMs());
				ImGui::TableNextColumn();
				ImGui::Text("%.5f", timing.SampleCount == 0 ? 0.0 : timing.MinMs);
				ImGui::TableNextColumn();
				ImGui::Text("%.5f", timing.MaxMs);
				ImGui::TableNextColumn();
				ImGui::Text("%u", timing.SampleCount);
			};

			drawTiming("Scene CSM Shadow", timingStatistics.SceneCSMShadow.CPU);
			drawTiming("Scene 2D Shadow", timingStatistics.SceneShadow2D.CPU);
			drawTiming("Scene Point Shadow", timingStatistics.ScenePointShadow.CPU);
			drawTiming("Character Shadow", timingStatistics.CharacterShadow.CPU);
			drawTiming("GBuffer", timingStatistics.GBuffer.CPU);
			drawTiming("SSAO", timingStatistics.SSAO.CPU);
			drawTiming("Lighting", timingStatistics.Lighting.CPU);
			drawTiming("Screen Space Outline", timingStatistics.ScreenSpaceOutline.CPU);
			drawTiming("Inverted Hull Outline", timingStatistics.InvertedHullOutline.CPU);
			drawTiming("Character", timingStatistics.Character.CPU);
			drawTiming("Geometry Outline", timingStatistics.GeometryOutline.CPU);
			drawTiming("SSR", timingStatistics.SSR.CPU);
			drawTiming("Transparent", timingStatistics.Transparent.CPU);
			drawTiming("TAA", timingStatistics.TAA.CPU);
			drawTiming("Bloom", timingStatistics.Bloom.CPU);
			drawTiming("Tone Mapping", timingStatistics.ToneMapping.CPU);

			ImGui::EndTable();
		}

		if (ImGui::BeginTable("Pass GPU Timing", 5,
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("GPU Pass");
			ImGui::TableSetupColumn("Avg ms");
			ImGui::TableSetupColumn("Min ms");
			ImGui::TableSetupColumn("Max ms");
			ImGui::TableSetupColumn("Samples");
			ImGui::TableHeadersRow();

			auto drawTiming = [](const char* name, const TimingStatistics& timing)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(name);
				ImGui::TableNextColumn();
				ImGui::Text("%.5f", timing.GetAverageMs());
				ImGui::TableNextColumn();
				ImGui::Text("%.5f", timing.SampleCount == 0 ? 0.0 : timing.MinMs);
				ImGui::TableNextColumn();
				ImGui::Text("%.5f", timing.MaxMs);
				ImGui::TableNextColumn();
				ImGui::Text("%u", timing.SampleCount);
			};

			drawTiming("Deferred Frame", timingStatistics.Frame.GPU);
			drawTiming("Scene CSM Shadow", timingStatistics.SceneCSMShadow.GPU);
			drawTiming("Scene 2D Shadow", timingStatistics.SceneShadow2D.GPU);
			drawTiming("Scene Point Shadow", timingStatistics.ScenePointShadow.GPU);
			drawTiming("Character Shadow", timingStatistics.CharacterShadow.GPU);
			drawTiming("GBuffer", timingStatistics.GBuffer.GPU);
			drawTiming("SSAO", timingStatistics.SSAO.GPU);
			drawTiming("Lighting", timingStatistics.Lighting.GPU);
			drawTiming("Screen Space Outline", timingStatistics.ScreenSpaceOutline.GPU);
			drawTiming("Inverted Hull Outline", timingStatistics.InvertedHullOutline.GPU);
			drawTiming("Character", timingStatistics.Character.GPU);
			drawTiming("Geometry Outline", timingStatistics.GeometryOutline.GPU);
			drawTiming("SSR", timingStatistics.SSR.GPU);
			drawTiming("Transparent", timingStatistics.Transparent.GPU);
			drawTiming("TAA", timingStatistics.TAA.GPU);
			drawTiming("Bloom", timingStatistics.Bloom.GPU);
			drawTiming("Tone Mapping", timingStatistics.ToneMapping.GPU);

			ImGui::EndTable();
		}

		ImGui::End();
	}

	void EntityInspectorPanel::OnImGuiRender(const Ref<Scene>& scene, EditorSelection& selection)
	{
		ImGui::Begin("Entity Inspector");
		if (!selection.HasEntity())
		{
			ImGui::TextUnformatted("Select a Entity in the Scene hierarchy.");
		}
		else
		{
			// Resolve the selected entity, model, submesh, and editable properties.
			const auto& selectedID = selection.EntityID;

			if (auto* transform = scene->TryGetComponent<TransformComponent>(selectedID))
			{
				if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
					TransformRender(transform);
			}

			if (auto* light = scene->TryGetComponent<LightComponent>(selectedID))
			{
				if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
					LightRender(light);
			}

			if (selection.HasSubMesh())
			{
				if (auto* render = scene->TryGetComponent<RenderComponent>(selectedID))
				{
					SubMeshRender(render, selection.SubMeshIndex);
				}
			}
			else
			{
				ImGui::TextDisabled("Select a SubMesh to edit material settings.");
			}

			
		}
		ImGui::End();

	}

	void SceneHierarchyPanel::OnImGuiRender(const Ref<Scene>& scene, EditorSelection& selection)
	{
		ImGui::Begin("Inspector");
		for (const auto& [entityID, entityRecord] : scene->GetEntities())
		{
			const bool selected = entityID == selection.EntityID;
			auto render = scene->TryGetComponent<RenderComponent>(entityID);

			Ref<Model> model = render ? render->ModelAsset : nullptr;

			ImGuiTreeNodeFlags flags =
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_OpenOnDoubleClick |
				ImGuiTreeNodeFlags_SpanAvailWidth;

			if (selected)
				flags |= ImGuiTreeNodeFlags_Selected;

			const bool open = ImGui::TreeNodeEx(entityRecord.name.c_str(), flags);

			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			{
				selection.SelectEntity(entityID);
			}

			if (open)
			{
				if (render && model)
					DrawModelNode(model, model->GetRootNode(), entityID, selection);

				ImGui::TreePop();
			}
		}
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawModelNode(const Engine::Ref<Model>& model, const Model::ModelNode& node, EntityID entityID, EditorSelection& selection)
	{
		ImGui::PushID(&node);
		const bool hasChildren = !node.Children.empty() || !node.MeshIndices.empty();
		const ImGuiTreeNodeFlags nodeFlags =
			hasChildren ? 0 : ImGuiTreeNodeFlags_Leaf;

		const char* nodeName =
			node.Name.empty() ? "ModelNode" : node.Name.c_str();

		const bool open = ImGui::TreeNodeEx(nodeName, nodeFlags);

		if (open)
		{
			for (auto idx : node.MeshIndices)
			{
				const auto& subMesh = model->GetSubMeshes()[idx];
				ImGui::PushID(static_cast<int>(idx));

				const std::string label = subMesh.Name.empty() ? "SubMesh " + std::to_string(idx)
					: subMesh.Name;

				const bool selected =
					selection.EntityID == entityID &&
					selection.HasSubMesh() &&
					selection.SubMeshIndex == idx;

				if (ImGui::Selectable(label.c_str(), selected))
				{
					selection.SelectSubMesh(entityID, idx);
				}

				ImGui::SameLine();
				ImGui::TextDisabled("Material %u", subMesh.MaterialIndex);

				ImGui::PopID();
			}

			for (const auto& child : node.Children)
				DrawModelNode(model, child, entityID, selection);

			ImGui::TreePop();
		}
		ImGui::PopID();
	}
	void EntityInspectorPanel::TransformRender(TransformComponent* transform)
	{
		glm::vec3 translation = transform->GetTranslation();
		glm::vec3 rotationDegrees = glm::degrees(transform->GetRotation());

		if (ImGui::DragFloat3("Translation", glm::value_ptr(translation), 0.05f))
			transform->SetTranslation(translation);

		if (ImGui::DragFloat3("Rotation", glm::value_ptr(rotationDegrees), 1.0f))
			transform->SetRotation(glm::radians(rotationDegrees));

		// TransformComponent exposes scale directly.
		glm::vec3 scale = transform->GetScale();
		if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f, 0.001f, 100.0f))
			transform->SetScale(scale);
	}
	void EntityInspectorPanel::LightRender(LightComponent* light)
	{
		glm::vec3 color = light->Color;
		if (ImGui::ColorEdit3("Color", glm::value_ptr(color)))
			light->Color = color;
		int lightTypeMode = static_cast<int>(light->Type);
		if (ImGui::Combo("Color Type", &lightTypeMode, lightNames, IM_ARRAYSIZE(lightNames)))
		{
			light->Type = static_cast<LightType>(lightTypeMode);
		}
		ImGui::DragFloat("Intensity", &light->Intensity, 0.05f, 0.0f, 100.0f);
		ImGui::Checkbox("Enabled", &light->Enabled);

		if (light->Type == LightType::Point)
			ImGui::DragFloat("Range", &light->Range, 0.05f, 0.01f, 100.0f);
		if (light->Type == LightType::Spot)
		{
			ImGui::DragFloat("Range", &light->Range, 0.05f, 0.01f, 100.0f);
			ImGui::DragFloat("Inner Angle", &light->InnerConeAngle, 0.05f, 0.0f, light->OuterConeAngle);
			ImGui::DragFloat("Outer Angle", &light->OuterConeAngle, 0.05f, light->InnerConeAngle, 1.0f);
		}
	}
	void EntityInspectorPanel::SubMeshRender(RenderComponent* render, uint32_t subMeshIndex)
	{
		if (!render || !render->ModelAsset || subMeshIndex >= render->ModelAsset->GetSubMeshes().size())
			return;

		const auto& submesh = render->ModelAsset->GetSubMeshes()[subMeshIndex];
		const auto& materialIns = render->GetMaterial(submesh.MaterialIndex);
		if (!materialIns)
			return;

		auto& config = materialIns->GetRenderConfig();

		int blendMode = static_cast<int>(config.Blend);
		int cullMode = static_cast<int>(config.Cull);
		bool castShadow = config.CastShadow;

		float baseColorFactorAlpha = materialIns->GetBaseColorFactor().a;
		float ssrStrength = MaterialSystem::GetImportedPBRSSRStrength(materialIns);

		auto toon = MaterialSystem::GetToonParameters(materialIns);
		int toonRole = static_cast<int>(toon.Role);
		static const char* toonRoleNames[] =
		{
			"Default", "Face", "Eye", "Eye Highlight", "Hair", "Skin", "Metal"
		};


		if (ImGui::Combo("Blend Mode", &blendMode, blendNames, IM_ARRAYSIZE(blendNames)))
		{
			render->SetMaterialBlendMode(submesh.MaterialIndex, static_cast<Engine::BlendMode>(blendMode));
		}

		if (ImGui::Combo("Cull Mode", &cullMode, cullNames, IM_ARRAYSIZE(cullNames)))
		{
			render->SetMaterialCullMode(submesh.MaterialIndex, static_cast<Engine::CullMode>(cullMode));
		}
		if (ImGui::SliderFloat("Base Color Factor Alpha", &baseColorFactorAlpha, 0.0f, 1.0f))
		{
			// materialIns->SetBaseColorFactorAlpha(baseColorFactorAlpha);
			render->SetMaterialBaseAlpha(submesh.MaterialIndex, baseColorFactorAlpha);
		}

		if (ImGui::Checkbox("Cast Shadow", &castShadow))
		{
			render->SetMaterialCastShadow(submesh.MaterialIndex, castShadow);
		}

		ImGui::Text("PBR Material Setting");
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("SSR Strength");
		ImGui::SameLine();

		const float ssrApplyButtonWidth = ImGui::CalcTextSize("Apply").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		const float ssrSliderWidth = std::max(
			80.0f,
			ImGui::GetContentRegionAvail().x - ssrApplyButtonWidth - ImGui::GetStyle().ItemSpacing.x);
		ImGui::SetNextItemWidth(ssrSliderWidth);
		if (ImGui::SliderFloat("##SSRStrength", &ssrStrength, 0.0f, 1.0f))
		{
			auto overrideMaterial = render->GetOrCreateMaterialOverride(submesh.MaterialIndex);
			MaterialSystem::SetImportedPBRSSRStrength(overrideMaterial, ssrStrength);
		}

		ImGui::SameLine();
		if (ImGui::Button("Apply##SSRStrength"))
		{
			if (!AssetManager::SetModelSubMeshSSRStrength(
				render->ModelAsset->GetHandle(), subMeshIndex, ssrStrength))
			{
				HZ_CORE_ERROR("Failed to save SSR strength for submesh {0}", subMeshIndex);
			}
		}


		ImGui::Text("Toon Light Setting");
		bool ToonDirty = false;
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Toon Material Role");
		ImGui::SameLine();

		const float applyButtonWidth = ImGui::CalcTextSize("Apply").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		const float roleComboWidth = std::max(
			80.0f,
			ImGui::GetContentRegionAvail().x - applyButtonWidth - ImGui::GetStyle().ItemSpacing.x);
		ImGui::SetNextItemWidth(roleComboWidth);
		if (ImGui::Combo("##ToonMaterialRole", &toonRole, toonRoleNames, IM_ARRAYSIZE(toonRoleNames)))
		{
			toon.Role = static_cast<ToonMaterialRole>(toonRole);
			ToonDirty = true;
		}

		ImGui::SameLine();
		if (ImGui::Button("Apply##ToonMaterialRole"))
		{
			if (!AssetManager::SetModelSubMeshToonMaterialRole(
				render->ModelAsset->GetHandle(), subMeshIndex, toon.Role))
			{
				HZ_CORE_ERROR("Failed to save Toon material role for submesh {0}", subMeshIndex);
			}
		}
		if (ImGui::SliderFloat("Threshold", &toon.Threshold, 0.01f, 1.0f))
			ToonDirty = true;
		if (ImGui::SliderFloat("Softness", &toon.Softness, 0.01f, 1.0f))
			ToonDirty = true;
		if (ImGui::SliderFloat("Light Level", &toon.LitLevel, 0.5f, 1.0f))
			ToonDirty = true;
		if (ImGui::SliderFloat("Shadow Level", &toon.ShadowLevel, 0.1f, 1.0f))
			ToonDirty = true;
		ImGui::Text("Toon Rim Light Setting");
		if (ImGui::ColorEdit3("Rim Color", glm::value_ptr(toon.RimColor)))
			ToonDirty = true;
		if (ImGui::SliderFloat("Rim Intensity", &toon.RimIntensity, 0.0f, 1.0f))
			ToonDirty = true;
		if (ImGui::SliderFloat("Rim Power", &toon.RimPower, 4.0f, 32.0f))
			ToonDirty = true;
		if(ImGui::SliderFloat("Rim Mask", &toon.RimLightMask, 0.0f, 1.0f))
			ToonDirty = true;

		ImGui::Text("Toon Face Shadow Map");
		if (ImGui::SliderFloat("Face Shadow Softness", &toon.FaceShadowSoftness, 0.0f, 0.1f))
			ToonDirty = true;


		if (ToonDirty)
			render->SetToonParmatersOverride(submesh.MaterialIndex, toon);

	}
}
