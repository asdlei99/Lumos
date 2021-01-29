#pragma once


#include "API/IndexBuffer.h"
#include "API/VertexBuffer.h"
#include "Graphics/API/CommandBuffer.h"
#include "Graphics/API/DescriptorSet.h"
#include "Maths/Maths.h"
#include "Material.h"

#include <array>

namespace Lumos
{
	namespace Graphics
	{
		class Texture2D;

		struct LUMOS_EXPORT BasicVertex
		{
			Maths::Vector3 Position;
			Maths::Vector2 TexCoords;
		};

		struct LUMOS_EXPORT Vertex
		{
			Vertex()
				: Position(Maths::Vector3(0.0f))
				, Colours(Maths::Vector4(0.0f))
				, TexCoords(Maths::Vector2(0.0f))
				, Normal(Maths::Vector3(0.0f))
				, Tangent(Maths::Vector3(0.0f))
			{

			}

			Maths::Vector3 Position;
			Maths::Vector4 Colours;
			Maths::Vector2 TexCoords;
			Maths::Vector3 Normal;
			Maths::Vector3 Tangent;

			bool operator==(const Vertex& other) const
			{
				return Position == other.Position  && TexCoords == other.TexCoords && Colours == other.Colours && Normal == other.Normal && Tangent == other.Tangent;
			}
		};

		class LUMOS_EXPORT Mesh
		{
		public:

			Mesh();
			Mesh(const Mesh& mesh);
			Mesh(Ref<VertexBuffer>& vertexBuffer, Ref<IndexBuffer>& indexBuffer, const Ref<Maths::BoundingBox>& boundingBox);

			virtual ~Mesh();

			const Ref<VertexBuffer>& GetVertexBuffer() const { return m_VertexBuffer; }
			const Ref<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }
			const Ref<Material>& GetMaterial() const { return m_Material; }
			const Ref<Maths::BoundingBox>& GetBoundingBox() const { return m_BoundingBox; }

			void SetMaterial(const Ref<Material>& material) { m_Material = material; }

			bool& GetActive() { return m_Active; }
			void SetName(const std::string& name) { m_Name = name; }

		protected:

			static Maths::Vector3 GenerateTangent(const Maths::Vector3 &a, const Maths::Vector3 &b, const Maths::Vector3 &c, const Maths::Vector2 &ta, const Maths::Vector2 &tb, const Maths::Vector2 &tc);
			static Maths::Vector3* GenerateNormals(uint32_t numVertices, Maths::Vector3* vertices, uint32_t* indices, uint32_t numIndices);
			static Maths::Vector3* GenerateTangents(uint32_t numVertices, Maths::Vector3* vertices, uint32_t* indices, uint32_t numIndices, Maths::Vector2* texCoords);

			Ref<VertexBuffer> m_VertexBuffer;
			Ref<IndexBuffer> m_IndexBuffer;
			Ref<Material> m_Material;
			Ref<Maths::BoundingBox> m_BoundingBox;

			std::string m_Name;
			
			bool m_Active = true;
		};
	}
}

namespace std
{
	template<> struct hash<Lumos::Graphics::Vertex>
	{
		size_t operator()(Lumos::Graphics::Vertex const& vertex) const
		{
			return ((hash<Lumos::Maths::Vector3>()(vertex.Position) ^
				(hash<Lumos::Maths::Vector2>()(vertex.TexCoords) << 1) ^
				(hash<Lumos::Maths::Vector4>()(vertex.Colours) << 1) ^
				(hash<Lumos::Maths::Vector3>()(vertex.Normal) << 1) ^
				(hash<Lumos::Maths::Vector3>()(vertex.Tangent) << 1)));
		}

	};
}
