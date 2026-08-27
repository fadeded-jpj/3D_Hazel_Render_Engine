#include "hzpch.h"
#include "Model.h"

#include <functional>
#include <iostream>
#include <filesystem>
#include <limits>
#include <cmath>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/GltfMaterial.h"

#include "Hazel/Renderer/Geometry/Mesh.h"
#include "Hazel/Renderer/Material/Material.h"
#include "Hazel/Renderer/Material/MaterialInstance.h"
#include "Hazel/Renderer/Model/Import/Importers.h"
#include "Hazel/Renderer/Model/Import/ModelImporter.h"
#include "Hazel/Renderer/RHI/Buffer.h"
#include "Hazel/Renderer/RHI/Shader.h"
#include "Hazel/Renderer/RHI/Texture.h"
#include "Hazel/Renderer/Resources/RenderResourceCache.h"
#include "Hazel/AssetsSystem/AssetManager.h"
#include "Hazel/Renderer/Shader/ShaderManager.h"
#include "Hazel/Renderer/Material/MaterialSystem.h"

namespace Engine
{
	namespace
	{
		struct BoundsAccumulator
		{
			glm::vec3 Min{ std::numeric_limits<float>::max() };
			glm::vec3 Max{ std::numeric_limits<float>::lowest() };
			bool Valid = false;

			void Expand(const glm::vec3& point)
			{
				Min = glm::min(Min, point);
				Max = glm::max(Max, point);
				Valid = true;
			}
		};

		Bounds CalculateModelBounds(const ModelImportData& data)
		{
			std::vector<BoundsAccumulator> meshBounds(data.meshes.size());
			for (size_t meshIndex = 0; meshIndex < data.meshes.size(); ++meshIndex)
			{
				for (const auto& vertex : data.meshes[meshIndex].Vertices)
					meshBounds[meshIndex].Expand(vertex.position);
			}

			BoundsAccumulator modelBounds;
			auto appendMeshBounds = [&](uint32_t meshIndex, const glm::mat4& transform)
				{
					if (meshIndex >= meshBounds.size() || !meshBounds[meshIndex].Valid)
						return;

					const auto& mesh = meshBounds[meshIndex];
					for (int x = 0; x < 2; ++x)
					{
						for (int y = 0; y < 2; ++y)
						{
							for (int z = 0; z < 2; ++z)
							{
								const glm::vec3 corner{
									x == 0 ? mesh.Min.x : mesh.Max.x,
									y == 0 ? mesh.Min.y : mesh.Max.y,
									z == 0 ? mesh.Min.z : mesh.Max.z
								};
								modelBounds.Expand(glm::vec3(transform * glm::vec4(corner, 1.0f)));
							}
						}
					}
				};

			auto visitNode = [&](auto&& self, const ModelNode& node, const glm::mat4& parentTransform) -> void
				{
					const glm::mat4 nodeTransform = parentTransform * node.LocalTransform;
					for (uint32_t meshIndex : node.MeshIndices)
						appendMeshBounds(meshIndex, nodeTransform);

					for (const auto& child : node.Children)
						self(self, child, nodeTransform);
				};

			visitNode(visitNode, data.rootNode, glm::mat4(1.0f));

			// Keep malformed or hierarchy-less imports usable by falling back to mesh-local bounds.
			if (!modelBounds.Valid)
			{
				for (uint32_t meshIndex = 0; meshIndex < meshBounds.size(); ++meshIndex)
					appendMeshBounds(meshIndex, glm::mat4(1.0f));
			}

			if (!modelBounds.Valid)
				return {};

			Bounds bounds;
			bounds.Origin = (modelBounds.Min + modelBounds.Max) * 0.5f;
			bounds.BoxExtent = (modelBounds.Max - modelBounds.Min) * 0.5f;
			bounds.SphereRadius = glm::length(bounds.BoxExtent);
			return bounds;
		}
	}

	Ref<Model> ModelImporter::ImportFromFile(const std::filesystem::path& filepath, ModelType type)
	{
		return ImportFromFile(filepath, ModelImportSettings{}, type);
	}

	Ref<Model> ModelImporter::ImportFromFile(
		const std::filesystem::path& filepath,
		const ModelImportSettings& settings,
		ModelType type)
	{
		if (type == ModelType::Default)
		{
			std::filesystem::path p = filepath;
			auto ext = p.extension().string();
			if (ext == ".pmx")
				type = ModelType::PMX;
			else if (ext == ".fbx")
				type = ModelType::FBX;
			else if (ext == ".obj")
				type = ModelType::OBJ;
			else if (ext == ".gltf" || ext == ".glb")
				type = ModelType::GLTF;
			else
				HZ_CORE_ASSERT(false, "Unsupported Model Type {0} !", filepath.string());
		}

		MaterialImportMode materialMode = settings.MaterialMode;
		if (materialMode == MaterialImportMode::Auto)
		{
			materialMode = type == ModelType::PMX
				? MaterialImportMode::Toon
				: MaterialImportMode::PBR;
		}

		auto data = Dispatch(filepath, type);

		return BuildModel(std::move(data), settings, materialMode);
	}

	ModelImportData ModelImporter::Dispatch(const std::filesystem::path& filepath, ModelType type)
	{
		switch (type)
		{
		case ModelType::PMX:
			return PMXModelImporter().ImportFromFile(filepath);
		case ModelType::FBX:
			return AssimpModelImporter().ImportFromFile(filepath);
		case ModelType::OBJ:
			return AssimpModelImporter().ImportFromFile(filepath);
		case ModelType::GLTF:
			return AssimpModelImporter().ImportFromFile(filepath);
		default:
			HZ_CORE_ASSERT(false, "Unsupported Model Type {0} !", filepath.string());
		}
		return ModelImportData();
	}

	Ref<Model> ModelImporter::BuildModel(ModelImportData data, const ModelImportSettings& settings,
		MaterialImportMode materialMode)
	{
		Model* model = new Model();
		model->m_Bounds = CalculateModelBounds(data);

		model->m_FilePath = std::move(data.filepath);
		model->m_Skeleton = std::move(data.skeleton);
		model->m_RootNode = std::move(data.rootNode);
		model->m_GlobalInverseTransform = std::move(data.GlobalInverseTransform);

		model->m_SubMeshes.reserve(data.meshes.size());
		model->m_Materials.reserve(data.materials.size());

		//auto shader = ShaderManager::Get("Default");

		
		for (auto& desc : data.materials)
		{
			BlendMode blend = BlendMode::Opaque;

			MaterialRenderConfig config;
			config.Blend = desc.Common.Blend;
			config.Cull = desc.Common.Cull;

			config.DepthWrite = desc.Common.Blend != BlendMode::AlphaBlend;

			auto material = materialMode == MaterialImportMode::Toon
				? MaterialSystem::GetImportedToonMaterial(config)
				: MaterialSystem::GetImportedPBRMaterial(config);
			if (!material)
				material = MaterialSystem::GetErrorMaterial(config);
			if (!material)
			{
				HZ_CORE_ERROR("Faild to create imported material");
				continue;
			}

			model->m_Materials.push_back(BuildMaterial(desc, material, materialMode));
		}

		for (auto& mesh : data.meshes)
			model->m_SubMeshes.push_back(BuildSubMesh(mesh));

		for (const auto& [subMeshIndex, role] : settings.SubMeshToonMaterialRoles)
		{
			if (subMeshIndex >= model->m_SubMeshes.size() || role >= ToonMaterialRole::Count)
			{
				HZ_CORE_WARN("Ignoring invalid Toon role override for submesh {0}", subMeshIndex);
				continue;
			}

			auto& subMesh = model->m_SubMeshes[subMeshIndex];
			if (subMesh.MaterialIndex >= model->m_Materials.size())
			{
				HZ_CORE_WARN("Ignoring Toon role override for submesh {0}: invalid material index", subMeshIndex);
				continue;
			}

			auto sourceMaterial = model->GetMaterial(subMesh.MaterialIndex);
			if (!sourceMaterial)
				continue;

			auto subMeshMaterial = sourceMaterial->Clone();
			subMeshMaterial->SetToonMaterialRole(role);
			subMesh.MaterialIndex = static_cast<uint32_t>(model->m_Materials.size());
			model->m_Materials.push_back(std::move(subMeshMaterial));
		}

		for (const auto& [subMeshIndex, strength] : settings.SubMeshSSRStrengths)
		{
			if (subMeshIndex >= model->m_SubMeshes.size() || !std::isfinite(strength))
			{
				HZ_CORE_WARN("Ignoring invalid SSR strength override for submesh {0}", subMeshIndex);
				continue;
			}

			auto& subMesh = model->m_SubMeshes[subMeshIndex];
			if (subMesh.MaterialIndex >= model->m_Materials.size())
			{
				HZ_CORE_WARN("Ignoring SSR strength override for submesh {0}: invalid material index", subMeshIndex);
				continue;
			}

			auto sourceMaterial = model->GetMaterial(subMesh.MaterialIndex);
			if (!sourceMaterial)
				continue;

			auto subMeshMaterial = sourceMaterial->Clone();
			MaterialSystem::SetImportedPBRSSRStrength(subMeshMaterial, strength);
			subMesh.MaterialIndex = static_cast<uint32_t>(model->m_Materials.size());
			model->m_Materials.push_back(std::move(subMeshMaterial));
		}

		return Ref<Model>(model);
	}

	Ref<MaterialInstance> ModelImporter::BuildMaterial(const ImportedMaterialDesc& data,
		const Ref<Material>& material, MaterialImportMode materialMode)
	{
		auto instance = BuildSurfaceMaterial(data.Common, data.Surface, material);

		if (materialMode == MaterialImportMode::Toon)
			return BuildToonMaterial(data.Toon.value_or(ImportedToonMaterialDesc{}), instance);

		return BuildPBRMaterial(data.PBR.value_or(ImportedPBRMaterialDesc{}), instance);
	}

	Ref<MaterialInstance> ModelImporter::BuildSurfaceMaterial(const ImportedMaterialCommon& common, const ImportedSurfaceMaterialDesc& surface, const Ref<Material>& material)
	{
		auto instance = MaterialInstance::Create(material);

		instance->SetFloat("u_AlphaCutoff", common.AlphaCutoff);
		instance->SetInt("u_AlphaMode", common.Blend == BlendMode::AlphaCutout ? 1 : 0);

		instance->SetFloat4("u_BaseColorFactor", surface.BaseColorFactor);
		instance->SetFloat3("u_EmissiveFactor", surface.EmissiveFactor);
		instance->SetFloat("u_EmissiveStrength", surface.EmissiveStrength);

		instance->SetInt("u_HasBaseColorTexture", surface.BaseColorTexture.Valid ? 1 : 0);
		instance->SetInt("u_HasNormalTexture", surface.NormalTexture.Valid ? 1 : 0);
		instance->SetInt("u_HasEmissiveTexture", surface.EmissiveTexture.Valid ? 1 : 0);

		SetTexture(instance, "u_BaseColorTexture", surface.BaseColorTexture);
		SetTexture(instance, "u_NormalTexture", surface.NormalTexture);
		SetTexture(instance, "u_EmissiveTexture", surface.EmissiveTexture);

		return instance;
	}

	Ref<MaterialInstance> ModelImporter::BuildPBRMaterial(const ImportedPBRMaterialDesc& pbr, const Ref<MaterialInstance>& instance)
	{

		instance->SetFloat("u_MetallicFactor",		pbr.MetallicFactor);
		instance->SetFloat("u_RoughnessFactor",		pbr.RoughnessFactor);
		instance->SetFloat("u_SSRStrength",			pbr.SSRStrength);


		instance->SetInt("u_HasSpecularTexture",	pbr.SpecularColorTexture.Valid ? 1 : 0);

		instance->SetInt("u_HasMetallicRoughnessTexture", pbr.MetallicRoughnessTexture.Valid ? 1 : 0);
		
		SetTexture(instance, "u_SpecularTexture", pbr.SpecularColorTexture);
		SetTexture(instance, "u_MetallicRoughnessTexture", pbr.MetallicRoughnessTexture);

		return instance;
	}

	Ref<MaterialInstance> ModelImporter::BuildToonMaterial(const ImportedToonMaterialDesc& toon, const Ref<MaterialInstance>& instance)
	{
		instance->SetToonMaterialRole(toon.Role);
		instance->SetFloat("u_Threshold", toon.Threshold);
		instance->SetFloat("u_ToonLitLevel", toon.LitLevel);
		instance->SetFloat("u_ToonShadowLevel", toon.ShadowLevel);
		instance->SetFloat("u_Softness", toon.Softness);
		instance->SetFloat3("u_ToonShadowTint", toon.ShadowTint);

		
		// ------------ for rim light -------------
		instance->SetFloat3("u_RimColor", toon.RimColor);
		instance->SetFloat("u_RimIntensity", toon.RimIntensity);
		instance->SetFloat("u_RimPower", toon.RimPower);
		instance->SetFloat("u_RimLightMask", toon.RimLightMask);

		// ------------ for face shadow --------
		instance->SetFloat("u_FaceShadowOffset", toon.FaceShadowOffset);
		instance->SetFloat("u_FaceShadowSoftness", toon.FaceShadowSoftness);
		instance->SetFloat("u_FaceShadowStrength", toon.FaceShadowStrength);
		instance->SetInt("u_HasFaceShadowMap", toon.FaceShadowMap.Valid ? 1 : 0);


		SetTexture(instance, "u_ToonRampTexture", toon.ToonRampTexture);
		SetTexture(instance, "u_ShadeMaskTexture", toon.ShadeMaskTexture);
		SetTexture(instance, "u_FaceShadowMap", toon.FaceShadowMap);
		SetTexture(instance, "u_HairHighLightMask", toon.HairHighlightMask);
		SetTexture(instance, "u_RimMaskTexture", toon.RimMaskTexture);
		SetTexture(instance, "u_MatCapTexture", toon.MatCapTexture);

		// for more pmx texture
		instance->SetInt("u_HasToonRampTexture", toon.ToonRampTexture.Valid ? 1 : 0);
		SetTexture(instance, "u_ToonRampTexture", toon.ToonRampTexture);

		// sphere map
		instance->SetInt("u_SphereMode", 0);
		if (toon.SphereMode == SphereMapMode::Additive )
		{
			instance->SetInt("u_SphereMode", 2);
			SetTexture(instance, "u_SphereMap", toon.SphereMap);
		}
		else if (toon.SphereMode == SphereMapMode::SubTexture || toon.SphereMode == SphereMapMode::Multiply)
		{
			HZ_CORE_WARN("Unsupported Sphere Mode!");
		}

		// edge setting
		instance->SetInt("u_EdgeEnabled", toon.EdgeEnabled ? 1 : 0);
		instance->SetFloat4("u_EdgeColor", toon.EdgeColor);
		instance->SetFloat("u_EdgeSize", toon.EdgeSize);

		return instance;
	}

	static std::vector<unsigned int> BuildOutlineEdgeIndices(const ImportedMeshData& mesh)
	{
		constexpr unsigned int Invalid = UINT_MAX;

		if (mesh.Vertices.empty() || mesh.Indices.empty())
			return {};

		struct PositionKey
		{
			int64_t X;
			int64_t Y;
			int64_t Z;

			bool operator==(const PositionKey& other) const
			{
				return X == other.X && Y == other.Y && Z == other.Z;
			}
		};

		struct PositionKeyHash
		{
			size_t operator()(const PositionKey& key) const
			{
				size_t seed = std::hash<int64_t>{}(key.X);
				seed ^= std::hash<int64_t>{}(key.Y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= std::hash<int64_t>{}(key.Z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				return seed;
			}
		};

		glm::vec3 boundsMin = mesh.Vertices.front().position;
		glm::vec3 boundsMax = boundsMin;
		for (const auto& vertex : mesh.Vertices)
		{
			boundsMin = glm::min(boundsMin, vertex.position);
			boundsMax = glm::max(boundsMax, vertex.position);
		}

		const float boundsDiagonal = glm::length(boundsMax - boundsMin);
		const float weldEpsilon = std::max(boundsDiagonal * 1e-6f, 1e-6f);
		const double inverseEpsilon = 1.0 / static_cast<double>(weldEpsilon);

		auto makePositionKey = [inverseEpsilon](const glm::vec3& position)
			{
				return PositionKey{
					static_cast<int64_t>(std::llround(position.x * inverseEpsilon)),
					static_cast<int64_t>(std::llround(position.y * inverseEpsilon)),
					static_cast<int64_t>(std::llround(position.z * inverseEpsilon))
				};
			};

		std::unordered_map<PositionKey, uint32_t, PositionKeyHash> canonicalVertices;
		canonicalVertices.reserve(mesh.Vertices.size());

		std::vector<uint32_t> canonicalIDs(mesh.Vertices.size());
		uint32_t nextCanonicalID = 0;
		for (uint32_t i = 0; i < mesh.Vertices.size(); ++i)
		{
			const auto key = makePositionKey(mesh.Vertices[i].position);
			auto [it, inserted] = canonicalVertices.emplace(key, nextCanonicalID);
			if (inserted)
				++nextCanonicalID;
			canonicalIDs[i] = it->second;
		}

		// 记录一个三角形边， 和邻接边为这个的 两个三角形
		struct EdgeRecord
		{
			unsigned int Start;
			unsigned int End;
			unsigned int Opposite0;				// 三角形顶点 1
			unsigned int Opposite1;				// 三角形顶点 2 （如有）
			unsigned int IncidentCount;

			EdgeRecord(unsigned int start, unsigned int end, unsigned int opposite)
				: Start(start), End(end), Opposite0(opposite),
				  Opposite1(Invalid), IncidentCount(1)
			{
			}
		};

		std::unordered_map<uint64_t, EdgeRecord> edges;

		auto addEdge = [&](unsigned int start, unsigned int end, unsigned int opposite)
			{
				HZ_CORE_ASSERT(start < canonicalIDs.size() && end < canonicalIDs.size(),
					"Outline edge vertex index is out of range");

				const auto canonicalStart = canonicalIDs[start];
				const auto canonicalEnd = canonicalIDs[end];
				if (canonicalStart == canonicalEnd)
					return;

				const auto minIndex = std::min(canonicalStart, canonicalEnd);
				const auto maxIndex = std::max(canonicalStart, canonicalEnd);
				const auto key = (static_cast<uint64_t>(minIndex) << 32) | maxIndex;

				auto [it, insert] = edges.emplace(key, EdgeRecord{ start, end, opposite });

				if (!insert)
				{
					auto& edge = it->second;
					if (edge.IncidentCount == 1)
						edge.Opposite1 = opposite;
					edge.IncidentCount++;
				}
			};

		HZ_CORE_ASSERT(mesh.Indices.size() % 3 == 0, "Triangle index count must be divisible by 3");

		for (auto i = 0; i < mesh.Indices.size(); i += 3)
		{
			auto i0 = mesh.Indices[i + 0];
			auto i1 = mesh.Indices[i + 1];
			auto i2 = mesh.Indices[i + 2];

			addEdge(i0, i1, i2);
			addEdge(i1, i2, i0);
			addEdge(i2, i0, i1);
		}

		std::vector<unsigned int> res;
		res.reserve(edges.size() * 4);

		size_t boundaryEdgeCount = 0;
		size_t manifoldEdgeCount = 0;
		size_t nonManifoldEdgeCount = 0;

		for (auto& [key, edge] : edges)
		{
			if (edge.IncidentCount == 1)
				++boundaryEdgeCount;
			else if (edge.IncidentCount == 2)
				++manifoldEdgeCount;
			else
				++nonManifoldEdgeCount;

			// GL_LINES_ADJACENCY:
			// opposite0, edgeStart, edgeEnd, opposite1

			res.push_back(edge.Opposite0);
			res.push_back(edge.Start);
			res.push_back(edge.End);

			res.push_back(edge.Opposite1 != Invalid ? edge.Opposite1 : edge.Start);
		}

#ifdef HZ_DEBUG
		HZ_CORE_TRACE(
			"Outline topology '{}': vertices={}, canonical={}, edges={}, boundary={}, manifold={}, non-manifold={}",
			mesh.Name, mesh.Vertices.size(), canonicalVertices.size(), edges.size(),
			boundaryEdgeCount, manifoldEdgeCount, nonManifoldEdgeCount);
#endif

		return res;
	}

	Model::SubMesh ModelImporter::BuildSubMesh(const ImportedMeshData& data)
	{
		auto submesh = Model::SubMesh();

		submesh.Name = data.Name;
		submesh.MaterialIndex = data.MaterialIndex;

		BoundsAccumulator bounds;
		for (const auto& vertex : data.Vertices)
			bounds.Expand(vertex.position);

		if (bounds.Valid)
			submesh.LocalBounds = { bounds.Min, bounds.Max };

		BufferLayout layout = {
			{ ShaderDataType::Float3,	"a_Pos" },
			{ ShaderDataType::Float3,	"a_Normal" },
			{ ShaderDataType::Float4,	"a_Tangent" },
			{ ShaderDataType::Float2,	"a_TexCoord" },
			{ ShaderDataType::Int4,		"a_BonesID" },
			{ ShaderDataType::Float4,	"a_BonesWeights" }
		};
		
		MeshData meshData;
		meshData.Layout = layout;
		meshData.VerticesData = data.Vertices.data();
		meshData.VertexBufferSize = static_cast<uint32_t>(data.Vertices.size() * sizeof(MeshVertex));
		meshData.VertexCount = (uint32_t)data.Vertices.size();
		meshData.IndicesData = data.Indices.data();
		meshData.IndexCount = (uint32_t)data.Indices.size();

		submesh.Mesh = Mesh::Create(meshData);

		// 创建 Outline submesh
		auto outlineIndices = BuildOutlineEdgeIndices(data);

		MeshData outlineData;
		outlineData.Layout = layout;
		outlineData.VerticesData = data.Vertices.data();
		outlineData.VertexBufferSize = static_cast<uint32_t>(data.Vertices.size() * sizeof(MeshVertex));
		outlineData.VertexCount = (uint32_t)data.Vertices.size();
		outlineData.IndexCount = (uint32_t)outlineIndices.size();
		outlineData.IndicesData = outlineIndices.data();
		outlineData.Topology = PrimitiveTopology::LinesAdjacency;

		submesh.OutlineEdgeMesh = Mesh::Create(outlineData);

		return submesh;
	}

	void ModelImporter::SetTexture(const Ref<MaterialInstance>& instance, const std::string& name, const ImportedTextureSlot& slot)
	{
		if (!slot.Valid)
			return;

		TextureLoadOptions options;
		options.ColorSpace = slot.ColorSpace;

		Ref<Texture> texture = nullptr;

		if (slot.ObjectPath)
			texture = AssetManager::GetAsset<Texture2D>(*slot.ObjectPath);
		else
			texture = RenderResourceCache::GetTexture(slot.Path, options);

		if (texture)
		{
			instance->SetTexture(name, texture);
		}
		else
		{
			auto flagName = name.substr(0, 2) + "Has" + name.substr(2);
			instance->SetInt(flagName, 0);
		}
	}

	//Ref<Model> Model::Import(const std::string& filepath)
	//{
	//	
	//	// TODO: 根据不同的模型类型，选择不同的解析方式
	//	return PMXModelImporter::ImportFromFile(filepath);
	//}

	Ref<MaterialInstance> Model::GetMaterial(uint32_t idx) const
	{
		if (m_Materials.empty())
			return nullptr;

		if (idx >= m_Materials.size())
			return m_Materials[0];

		return m_Materials[idx];
	}
	
}
