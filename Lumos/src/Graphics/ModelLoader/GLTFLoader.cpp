#include "LM.h"
#include "ModelLoader.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Maths/BoundingSphere.h"
#include "Entity/Entity.h"
#include "Entity/Component/MeshComponent.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Utilities/AssetsManager.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Matrix4.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef LUMOS_DIST
#define TINYGLTF_NOEXCEPTION
#endif
#include <tinygltf/tiny_gltf.h>

namespace lumos
{
	String AlbedoTexName = "baseColorTexture";
	String NormalTexName = "normalTexture";
	String MetallicTexName = "metallicRoughnessTexture";
	String GlossTexName = "metallicRoughnessTexture";
	String AOTexName = "occlusionTexture";

	struct GLTFTexture
	{
		tinygltf::Image* Image;
		tinygltf::Sampler* Sampler;
	};

	static std::map<int32_t, size_t> ComponentSize
	{
	{ TINYGLTF_COMPONENT_TYPE_BYTE,			  sizeof(int8_t) },
	{ TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,  sizeof(uint8_t) },
	{ TINYGLTF_COMPONENT_TYPE_SHORT,		  sizeof(int16_t) },
	{ TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, sizeof(uint16_t) },
	{ TINYGLTF_COMPONENT_TYPE_INT,			  sizeof(int32_t) },
	{ TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,   sizeof(uint32_t) },
	{ TINYGLTF_COMPONENT_TYPE_FLOAT,		  sizeof(float) },
	{ TINYGLTF_COMPONENT_TYPE_DOUBLE,		  sizeof(double) }
	};


	static std::map<int, int> GLTF_COMPONENT_LENGTH_LOOKUP = {
		{ TINYGLTF_TYPE_SCALAR, 1 },
	{ TINYGLTF_TYPE_VEC2, 2 },
	{ TINYGLTF_TYPE_VEC3, 3 },
	{ TINYGLTF_TYPE_VEC4, 4 },
	{ TINYGLTF_TYPE_MAT2, 4 },
	{ TINYGLTF_TYPE_MAT3, 9 },
	{ TINYGLTF_TYPE_MAT4, 16 }
	};

	static std::map<int, int> GLTF_COMPONENT_BYTE_SIZE_LOOKUP = {
		{ TINYGLTF_COMPONENT_TYPE_BYTE , 1 },
	{ TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, 1 },
	{ TINYGLTF_COMPONENT_TYPE_SHORT, 2 },
	{ TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, 2 },
	{ TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, 4 },
	{ TINYGLTF_COMPONENT_TYPE_FLOAT, 4 }
	};

	static graphics::TextureWrap GetWrapMode(int mode)
	{
		switch (mode)
		{
		case TINYGLTF_TEXTURE_WRAP_REPEAT: return graphics::TextureWrap::REPEAT;
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: return graphics::TextureWrap::CLAMP_TO_EDGE;
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return graphics::TextureWrap::MIRRORED_REPEAT;
		default: return graphics::TextureWrap::REPEAT;
		}
	}

	static graphics::TextureFilter GetFilter(int value)
	{
		switch (value)
		{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
			return graphics::TextureFilter::NEAREST;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			return graphics::TextureFilter::LINEAR;
		default: return graphics::TextureFilter::LINEAR;
		}
	}

	PBRMataterialTextures LoadMaterial(tinygltf::Material gltfmaterial, tinygltf::Model gltfmodel)
	{
		const auto loadTextureFromParameter = [&](const tinygltf::ParameterMap& parameterMap, const String& textureName)
		{
			GLTFTexture texture{};

			const auto& textureIt = parameterMap.find(textureName);
			if (textureIt != std::end(parameterMap))
			{
				const int textureIndex = static_cast<int>(textureIt->second.json_double_value.at("index"));
				const tinygltf::Texture& gltfTexture = gltfmodel.textures.at(textureIndex);
				if (gltfTexture.source != -1)
				{
					texture.Image = &gltfmodel.images.at(gltfTexture.source);
				}

				if (gltfTexture.sampler != -1)
				{
					texture.Sampler = &gltfmodel.samplers.at(gltfTexture.sampler);
				}
			}

			return texture;
		};

		PBRMataterialTextures textures;

		GLTFTexture albedoTex = loadTextureFromParameter(gltfmaterial.values, AlbedoTexName);

		if (albedoTex.Image)
		{
			graphics::TextureParameters params = graphics::TextureParameters(GetFilter(albedoTex.Sampler->minFilter), GetWrapMode(albedoTex.Sampler->wrapS));

			graphics::Texture2D* texture = graphics::Texture2D::CreateFromSource(albedoTex.Image->width, albedoTex.Image->height, albedoTex.Image->image.data(), params);
			if (texture)
				textures.albedo = std::shared_ptr<graphics::Texture2D>(texture);//material->SetAlbedoMap(texture);
		}
		else
			textures.albedo = nullptr;

		GLTFTexture normalTex = loadTextureFromParameter(gltfmaterial.values, NormalTexName);

		if (normalTex.Image)
		{
			graphics::TextureParameters params = graphics::TextureParameters(GetFilter(normalTex.Sampler->minFilter), GetWrapMode(normalTex.Sampler->wrapS));

			graphics::Texture2D* texture = graphics::Texture2D::CreateFromSource(normalTex.Image->width, normalTex.Image->height, normalTex.Image->image.data(), params);
			if (texture)
				textures.normal = std::shared_ptr<graphics::Texture2D>(texture);//material->SetNormalMap(texture);
		}

		GLTFTexture specularTex = loadTextureFromParameter(gltfmaterial.values, MetallicTexName);

		if (specularTex.Image)
		{
			graphics::TextureParameters params = graphics::TextureParameters(GetFilter(specularTex.Sampler->minFilter), GetWrapMode(specularTex.Sampler->wrapS));

			graphics::Texture2D* texture = graphics::Texture2D::CreateFromSource(specularTex.Image->width, specularTex.Image->height, specularTex.Image->image.data(), params);
			if (texture)
				textures.metallic = std::shared_ptr<graphics::Texture2D>(texture);//material->SetSpecularMap(texture);
		}

		GLTFTexture glossTex = loadTextureFromParameter(gltfmaterial.values, GlossTexName);

		if (glossTex.Image)
		{
			graphics::TextureParameters params = graphics::TextureParameters(GetFilter(glossTex.Sampler->minFilter), GetWrapMode(glossTex.Sampler->wrapS));

			graphics::Texture2D* texture = graphics::Texture2D::CreateFromSource(glossTex.Image->width, glossTex.Image->height, glossTex.Image->image.data(), params);
			if (texture)
				textures.roughness = std::shared_ptr<graphics::Texture2D>(texture);//material->SetGlossMap(texture);
		}

		GLTFTexture occlusionTex = loadTextureFromParameter(gltfmaterial.values, AOTexName);

		if (occlusionTex.Image)
		{
			graphics::TextureParameters params = graphics::TextureParameters(GetFilter(occlusionTex.Sampler->minFilter), GetWrapMode(occlusionTex.Sampler->wrapS));

			graphics::Texture2D* texture = graphics::Texture2D::CreateFromSource(occlusionTex.Image->width, occlusionTex.Image->height, occlusionTex.Image->image.data(), params);
			if (texture)
				textures.ao = std::shared_ptr<graphics::Texture2D>(texture);//material->SetGlossMap(texture);
		}

		return textures;
	}

	std::vector<std::shared_ptr<Material>> LoadMaterials(tinygltf::Model &gltfModel)
    {
        std::vector<std::shared_ptr<graphics::Texture2D>> loadedTextures;
        std::vector<std::shared_ptr<Material>> loadedMaterials;
        for (tinygltf::Texture &gltfTexture : gltfModel.textures)
        {
            GLTFTexture imageAndSampler{};

            if (gltfTexture.source != -1)
            {
                imageAndSampler.Image = &gltfModel.images.at(gltfTexture.source);
            }

            if (gltfTexture.sampler != -1)
            {
                imageAndSampler.Sampler = &gltfModel.samplers.at(gltfTexture.sampler);
            }

            if (imageAndSampler.Image)
            {
				graphics::TextureParameters params = graphics::TextureParameters(GetFilter(imageAndSampler.Sampler->minFilter), GetWrapMode(imageAndSampler.Sampler->wrapS));

				graphics::Texture2D* texture2D = graphics::Texture2D::CreateFromSource(imageAndSampler.Image->width, imageAndSampler.Image->height, imageAndSampler.Image->image.data(), params);
                if (texture2D)
                    loadedTextures.push_back(std::shared_ptr<graphics::Texture2D>(texture2D));
            }
        }

        for (tinygltf::Material &mat : gltfModel.materials)
        {
            std::shared_ptr<Material> pbrMaterial = std::make_shared<Material>();
            PBRMataterialTextures textures;
            MaterialProperties properties;
            
            // metallic-roughness workflow:
            auto baseColorTexture = mat.values.find("baseColorTexture");
            auto metallicRoughnessTexture = mat.values.find("metallicRoughnessTexture");
            auto baseColorFactor = mat.values.find("baseColorFactor");
            auto roughnessFactor = mat.values.find("roughnessFactor");
            auto metallicFactor = mat.values.find("metallicFactor");
            
            // common workflow:
            auto normalTexture = mat.additionalValues.find("normalTexture");
            //auto emissiveTexture = mat.additionalValues.find("emissiveTexture");
            auto occlusionTexture = mat.additionalValues.find("occlusionTexture");
            //auto emissiveFactor = mat.additionalValues.find("emissiveFactor");
            //auto alphaCutoff = mat.additionalValues.find("alphaCutoff");
            //auto alphaMode = mat.additionalValues.find("alphaMode");

            if (baseColorTexture != mat.values.end())
            {
                textures.albedo = loadedTextures[gltfModel.textures[baseColorTexture->second.TextureIndex()].source];
            }
            
            if (normalTexture != mat.additionalValues.end())
            {
                textures.normal = loadedTextures[gltfModel.textures[normalTexture->second.TextureIndex()].source];
            }
            
            if (metallicRoughnessTexture != mat.values.end())
            {
                textures.metallic = loadedTextures[gltfModel.textures[metallicRoughnessTexture->second.TextureIndex()].source];
            }

			if (occlusionTexture != mat.additionalValues.end())
			{
				textures.ao = loadedTextures[gltfModel.textures[occlusionTexture->second.TextureIndex()].source];
			}
            
            if (roughnessFactor != mat.values.end())
            {
                properties.glossColour = static_cast<float>(roughnessFactor->second.Factor());
            }
            
            if (metallicFactor != mat.values.end())
            {
                properties.specularColour = maths::Vector4(static_cast<float>(metallicFactor->second.Factor()));
            }
            
            if (baseColorFactor != mat.values.end())
            {
                properties.albedoColour = maths::Vector4((float)baseColorFactor->second.ColorFactor()[0],(float)baseColorFactor->second.ColorFactor()[1],(float)baseColorFactor->second.ColorFactor()[2],1.0f);
            }
            
            // Extensions
            auto specularGlossinessWorkflow = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
            if (specularGlossinessWorkflow != mat.extensions.end())
            {
                if (specularGlossinessWorkflow->second.Has("diffuseTexture"))
                {
                    int index = specularGlossinessWorkflow->second.Get("diffuseTexture").Get("index").Get<int>();
                    textures.albedo = loadedTextures[gltfModel.textures[index].source];

                }
                
                if (specularGlossinessWorkflow->second.Has("specularGlossinessTexture"))
                {
                    int index = specularGlossinessWorkflow->second.Get("specularGlossinessTexture").Get("index").Get<int>();
                    textures.roughness = loadedTextures[gltfModel.textures[index].source];
                }
                
                if (specularGlossinessWorkflow->second.Has("diffuseFactor"))
                {
                    auto& factor = specularGlossinessWorkflow->second.Get("diffuseFactor");
                    properties.albedoColour.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
                    properties.albedoColour.y = factor.ArrayLen() > 1 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int>()) : 1.0f;
                    properties.albedoColour.z = factor.ArrayLen() > 2 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int>()) : 1.0f;
                    properties.albedoColour.w = factor.ArrayLen() > 3 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int>()) : 1.0f;
                }
                if (specularGlossinessWorkflow->second.Has("specularFactor"))
                {
                    auto& factor = specularGlossinessWorkflow->second.Get("specularFactor");
                    properties.specularColour.x = factor.ArrayLen() > 0 ? float(factor.Get(0).IsNumber() ? factor.Get(0).Get<double>() : factor.Get(0).Get<int>()) : 1.0f;
                    properties.specularColour.y = factor.ArrayLen() > 0 ? float(factor.Get(1).IsNumber() ? factor.Get(1).Get<double>() : factor.Get(1).Get<int>()) : 1.0f;
                    properties.specularColour.z = factor.ArrayLen() > 0 ? float(factor.Get(2).IsNumber() ? factor.Get(2).Get<double>() : factor.Get(2).Get<int>()) : 1.0f;
                    properties.specularColour.w = factor.ArrayLen() > 0 ? float(factor.Get(3).IsNumber() ? factor.Get(3).Get<double>() : factor.Get(3).Get<int>()) : 1.0f;
                }
                if (specularGlossinessWorkflow->second.Has("glossinessFactor"))
                {
                    auto& factor = specularGlossinessWorkflow->second.Get("glossinessFactor");
                    properties.glossColour = maths::Vector4(1.0f - float(factor.IsNumber() ? factor.Get<double>() : factor.Get<int>()));
                }
            }

            pbrMaterial->SetTextures(textures);
            pbrMaterial->SetMaterialProperites(properties);
            loadedMaterials.push_back(pbrMaterial);
        }

        return loadedMaterials;
    }
    
	graphics::Mesh* LoadMesh(tinygltf::Model& model, tinygltf::Mesh& mesh, std::vector<std::shared_ptr<Material>>& materials)
    {
        for (auto& primitive : mesh.primitives)
        {
            const tinygltf::Accessor &indices = model.accessors[primitive.indices];
            
            const uint numVertices = static_cast<uint>(indices.count);
			graphics::Vertex* tempvertices = new graphics::Vertex[numVertices];
            uint* indicesArray = new uint[numVertices];
            
            size_t maxNumVerts = 0;
            
            std::shared_ptr<maths::BoundingSphere> boundingBox = std::make_shared<maths::BoundingSphere>();
            
            for (auto& attribute : primitive.attributes)
            {
                // Get accessor info
                auto& accessor = model.accessors.at(attribute.second);
                auto& bufferView = model.bufferViews.at(accessor.bufferView);
                auto& buffer = model.buffers.at(bufferView.buffer);
                int componentLength = GLTF_COMPONENT_LENGTH_LOOKUP.at(accessor.type);
                int componentTypeByteSize = GLTF_COMPONENT_BYTE_SIZE_LOOKUP.at(accessor.componentType);
                
                // Extra vertex data from buffer
                size_t bufferOffset = bufferView.byteOffset + accessor.byteOffset;
                int bufferLength = static_cast<int>(accessor.count) * componentLength * componentTypeByteSize;
                auto first = buffer.data.begin() + bufferOffset;
                auto last = buffer.data.begin() + bufferOffset + bufferLength;
                std::vector<byte> data = std::vector<byte>(first, last);
                
                // -------- Position attribute -----------
                
                if (attribute.first == "POSITION")
                {
                    size_t positionCount = accessor.count;
                    maxNumVerts = maths::Max(maxNumVerts, positionCount);
                    maths::Vector3Simple* positions = reinterpret_cast<maths::Vector3Simple*>(data.data());
                    for (auto p = 0; p < positionCount; ++p)
                    {
                        //positions[p] = glm::vec3(matrix * glm::vec4(positions[p], 1.0f));
                        tempvertices[p].Position = maths::ToVector(positions[p]);
                        
                        boundingBox->ExpandToFit(tempvertices[p].Position);
                    }
                }
                
                // -------- Normal attribute -----------
                
                else if (attribute.first == "NORMAL")
                {
                    size_t normalCount = accessor.count;
                    maxNumVerts = maths::Max(maxNumVerts, normalCount);
                    maths::Vector3Simple* normals = reinterpret_cast<maths::Vector3Simple*>(data.data());
                    for (auto p = 0; p < normalCount; ++p)
                    {
                        tempvertices[p].Normal = maths::ToVector(normals[p]);
                    }
                }
                
                // -------- Texcoord attribute -----------
                
                else if (attribute.first == "TEXCOORD_0")
                {
                    size_t uvCount = accessor.count;
                    maxNumVerts = maths::Max(maxNumVerts, uvCount);
                    maths::Vector2Simple* uvs = reinterpret_cast<maths::Vector2Simple*>(data.data());
                    for (auto p = 0; p < uvCount; ++p)
                    {
                        tempvertices[p].TexCoords = ToVector(uvs[p]);
                    }
                }
                
                // -------- Colour attribute -----------
                
                else if (attribute.first == "COLOR_0")
                {
                    size_t uvCount = accessor.count;
                    maxNumVerts = maths::Max(maxNumVerts, uvCount);
                    maths::Vector4Simple* colours = reinterpret_cast<maths::Vector4Simple*>(data.data());
                    for (auto p = 0; p < uvCount; ++p)
                    {
                        tempvertices[p].Colours = ToVector(colours[p]);
                    }
                }
                
                // -------- Tangent attribute -----------
                
                else if (attribute.first == "TANGENT")
                {
                    size_t uvCount = accessor.count;
                    maxNumVerts = maths::Max(maxNumVerts, uvCount);
                    maths::Vector3Simple* uvs = reinterpret_cast<maths::Vector3Simple*>(data.data());
                    for (auto p = 0; p < uvCount; ++p)
                    {
                        tempvertices[p].Tangent = ToVector(uvs[p]);
                    }
                }
            }
            
            std::shared_ptr<Material> pbrMaterial = materials[primitive.material];
            
            std::shared_ptr<graphics::VertexArray> va;
            va.reset(graphics::VertexArray::Create());
            
			graphics::VertexBuffer* buffer = graphics::VertexBuffer::Create(graphics::BufferUsage::STATIC);
            buffer->SetData(sizeof(graphics::Vertex) * numVertices, tempvertices);
            
            graphics::BufferLayout layout;
            layout.Push<maths::Vector3>("position");
            layout.Push<maths::Vector4>("colour");
            layout.Push<maths::Vector2>("texCoord");
            layout.Push<maths::Vector3>("normal");
            layout.Push<maths::Vector3>("tangent");
            buffer->SetLayout(layout);
            
            va->PushBuffer(buffer);
            
            // -------- Indices ----------
            {
                // Get accessor info
                auto indexAccessor = model.accessors.at(primitive.indices);
                auto indexBufferView = model.bufferViews.at(indexAccessor.bufferView);
                auto indexBuffer = model.buffers.at(indexBufferView.buffer);
                
                int componentLength = GLTF_COMPONENT_LENGTH_LOOKUP.at(indexAccessor.type);
                int componentTypeByteSize = GLTF_COMPONENT_BYTE_SIZE_LOOKUP.at(indexAccessor.componentType);
                
                // Extra index data
                size_t bufferOffset = indexBufferView.byteOffset + indexAccessor.byteOffset;
                int bufferLength = static_cast<int>(indexAccessor.count) * componentLength * componentTypeByteSize;
                auto first = indexBuffer.data.begin() + bufferOffset;
                auto last = indexBuffer.data.begin() + bufferOffset + bufferLength;
                std::vector<byte> data = std::vector<byte>(first, last);
                
                size_t indicesCount = indexAccessor.count;
                if (componentTypeByteSize == 2)
                {
                    uint16_t* in = reinterpret_cast<uint16_t*>(data.data()); //TODO: Test different models to check size - uint32 or 16
                    for (auto iCount = 0; iCount < indicesCount; iCount++)
                    {
                        indicesArray[iCount] = in[iCount];
                    }
                }
                else if (componentTypeByteSize == 4)
                {
                    uint32_t* in = reinterpret_cast<uint32_t*>(data.data());
                    for (auto iCount = 0; iCount < indicesCount; iCount++)
                    {
                        indicesArray[iCount] = in[iCount];
                    }
                }
            }
            
            std::shared_ptr<graphics::IndexBuffer> ib;
            ib.reset(graphics::IndexBuffer::Create(indicesArray, numVertices));
            
            auto lMesh = new graphics::Mesh(va, ib, pbrMaterial, boundingBox);
            
            delete[] tempvertices;
            delete[] indicesArray;
            
            return lMesh;
        }
        
        return nullptr;
    }
    
    void LoadNode(int nodeIndex, Entity* parent, tinygltf::Model& model, std::vector<std::shared_ptr<Material>>& materials, std::vector<graphics::Mesh*> meshes)
    {
        if (nodeIndex < 0)
        {
            return;
        }
        
        auto& node = model.nodes[nodeIndex];
        
        auto name = node.name;
        if(name == "")
            name = "Mesh : " + StringFormat::ToString(nodeIndex);
        auto meshEntity = std::make_shared<Entity>(name);
        meshEntity->AddComponent(std::make_unique<TransformComponent>(maths::Matrix4()));
        
        if(parent)
            parent->AddChildObject(meshEntity);
        
        if(node.mesh >= 0)
        {
            if (node.skin >= 0)
            {
                auto lMesh = std::shared_ptr<graphics::Mesh>(meshes[node.mesh]);
                meshEntity->AddComponent(std::make_unique<MeshComponent>(lMesh));
                meshEntity->SetBoundingRadius(lMesh->GetBoundingSphere()->SphereRadius());
            }
            else
            {
                auto lMesh = std::shared_ptr<graphics::Mesh>(meshes[node.mesh]);
                meshEntity->AddComponent(std::make_unique<MeshComponent>(lMesh));
                meshEntity->SetBoundingRadius(lMesh->GetBoundingSphere()->SphereRadius());
            }
        }
        
        TransformComponent* transform = meshEntity->GetTransform();
        
        if (!node.scale.empty())
        {
            transform->m_Transform.SetLocalScale(maths::Vector3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]), static_cast<float>(node.scale[2])));
        }
        if (!node.rotation.empty())
        {
            transform->m_Transform.SetLocalOrientation(maths::Quaternion(static_cast<float>(node.rotation[0]), static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]), static_cast<float>(node.rotation[3])));
        }
        if (!node.translation.empty())
        {
            transform->m_Transform.SetLocalPosition(maths::Vector3(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]), static_cast<float>(node.translation[2])));
        }
        if (!node.matrix.empty())
        {
            auto lTransform = maths::Matrix4(reinterpret_cast<float*>(node.matrix.data()));
            transform->m_Transform.SetLocalTransform(lTransform);
            transform->m_Transform.ApplyTransform(); // this creates S, R, T vectors from local matrix
        }

		transform->m_Transform.UpdateMatrices();
        
        if (!node.children.empty())
        {
            for (int child : node.children)
            {
                LoadNode(child, meshEntity.get(), model, materials, meshes);
            }
        }
    }

    std::shared_ptr<Entity> ModelLoader::LoadGLTF(const String& path)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		std::string ext = StringFormat::GetFilePathExtension(path);

		loader.SetImageLoader(tinygltf::LoadImageData, nullptr);
		loader.SetImageWriter(tinygltf::WriteImageData, nullptr);

		bool ret;

		if (ext == "glb") // assume binary glTF.
		{
			ret = tinygltf::TinyGLTF().LoadBinaryFromFile(&model, &err, &warn, path);
		}
		else // assume ascii glTF.
		{
			ret = tinygltf::TinyGLTF().LoadASCIIFromFile(&model, &err, &warn, path);
		}

		if (!err.empty())
		{
			LUMOS_CORE_ERROR(err);
		}

		if (!warn.empty())
		{
			LUMOS_CORE_ERROR(warn);
		}

		if (!ret)
		{
			LUMOS_CORE_ERROR("Failed to parse glTF");
		}

        auto LoadedMaterials = LoadMaterials(model);
        
        String directory = path.substr(0, path.find_last_of('/'));
        
        String name = directory.substr(directory.find_last_of('/') + 1);

		auto entity = std::make_shared<Entity>(name);
		entity->AddComponent(std::make_unique<TransformComponent>(maths::Matrix4()));

        auto meshes = std::vector<graphics::Mesh*>();
        
        for (auto& mesh : model.meshes)
        {
            meshes.emplace_back(LoadMesh(model,mesh,LoadedMaterials));
        }
        
        const tinygltf::Scene &gltfScene = model.scenes[maths::Max(0, model.defaultScene)];
        for (size_t i = 0; i < gltfScene.nodes.size(); i++)
        {
            LoadNode(gltfScene.nodes[i], entity.get(), model, LoadedMaterials, meshes);
        }
        
		return entity;
	}

}