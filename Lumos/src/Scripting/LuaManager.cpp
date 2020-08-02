#include "lmpch.h"
#include "LuaManager.h"
#include "Maths/Transform.h"
#include "Core/OS/Window.h"
#include "Core/VFS.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Core/Engine.h"
#include "Core/OS/Input.h"
#include "Scene/SceneManager.h"
#include "LuaScriptComponent.h"
#include "Scene/SceneGraph.h"
#include "Graphics/Camera/ThirdPersonCamera.h"

#include "Scene/Component/Components.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Camera/Camera2D.h"

#include "Graphics/Sprite.h"
#include "Graphics/Light.h"
#include "Graphics/API/Texture.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "Utilities/RandomNumberGenerator.h"

#include "ImGuiLua.h"
#include "PhysicsLua.h"
#include "MathsLua.h"

#include <imgui/imgui.h>

#ifdef CUSTOM_SMART_PTR
namespace sol
{
	template<typename T>
	struct unique_usertype_traits<Lumos::Ref<T>>
	{
		typedef T type;
		typedef Lumos::Ref<T> actual_type;
		static const bool value = true;

		static bool is_null(const actual_type& ptr)
		{
			return ptr == nullptr;
		}

		static type* get(const actual_type& ptr)
		{
			return ptr.get();
		}
	};

	template<typename T>
	struct unique_usertype_traits<Lumos::UniqueRef<T>>
	{
		typedef T type;
		typedef Lumos::UniqueRef<T> actual_type;
		static const bool value = true;

		static bool is_null(const actual_type& ptr)
		{
			return ptr == nullptr;
		}

		static type* get(const actual_type& ptr)
		{
			return ptr.get();
		}
	};

	template<typename T>
	struct unique_usertype_traits<Lumos::WeakRef<T>>
	{
		typedef T type;
		typedef Lumos::WeakRef<T> actual_type;
		static const bool value = true;

		static bool is_null(const actual_type& ptr)
		{
			return ptr == nullptr;
		}

		static type* get(const actual_type& ptr)
		{
			return ptr.get();
		}
	};
}

#endif

namespace Lumos
{
	LuaManager::LuaManager()
		: m_State(nullptr)
	{
	}

	void LuaManager::OnInit()
	{
		m_State.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table);

		BindInputLua(m_State);
		BindMathsLua(m_State);
		BindImGuiLua(m_State);
		BindECSLua(m_State);
		BindLogLua(m_State);
		BindSceneLua(m_State);
		BindPhysicsLua(m_State);
	}

	LuaManager::~LuaManager()
	{
	}

	void LuaManager::OnInit(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		auto view = registry.view<LuaScriptComponent>();

		if(view.empty())
			return;

		for(auto entity : view)
		{
			auto& luaScript = registry.get<LuaScriptComponent>(entity);
			luaScript.OnInit();
		}
	}

	void LuaManager::OnUpdate(Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		auto view = registry.view<LuaScriptComponent>();

		if(view.empty())
			return;

		float dt = Engine::Get().GetTimeStep().GetElapsedMillis();

		for(auto entity : view)
		{
			auto& luaScript = registry.get<LuaScriptComponent>(entity);
			luaScript.OnUpdate(dt);
		}
	}

	WindowProperties LuaManager::LoadConfigFile(const std::string& file)
	{
		WindowProperties windowProperties;

		std::string physicalPath;
		if(!VFS::Get()->ResolvePhysicalPath(file, physicalPath))
			return windowProperties;

		m_State.script_file(physicalPath);
		windowProperties.Title = m_State.get<std::string>("title");
		windowProperties.Width = m_State.get<int>("width");
		windowProperties.Height = m_State.get<int>("height");
		windowProperties.RenderAPI = m_State.get<int>("renderAPI");
		windowProperties.Fullscreen = m_State.get<bool>("fullscreen");
		windowProperties.Borderless = m_State.get<bool>("borderless");
		windowProperties.ShowConsole = m_State.get<bool>("showConsole");

		windowProperties.FilePath = file;

		return windowProperties;
	}

	entt::entity GetEntityByName(entt::registry& registry, const std::string& name)
	{
		entt::entity e = entt::null;
		registry.view<NameComponent>().each([&](const entt::entity& entity, const NameComponent& component) {
			if(name == component.name)
			{
				e = entity;
			}
		});

		return e;
	}

	void LuaManager::BindLogLua(sol::state& state)
	{
		auto log = state.create_table("Log");

		log.set_function("Trace", [&](sol::this_state s, std::string_view message) {
			Lumos::Debug::Log::Trace(message);
		});

		log.set_function("Info", [&](sol::this_state s, std::string_view message) {
			Lumos::Debug::Log::Trace(message);
		});

		log.set_function("Warn", [&](sol::this_state s, std::string_view message) {
			Lumos::Debug::Log::Warning(message);
		});

		log.set_function("Error", [&](sol::this_state s, std::string_view message) {
			Lumos::Debug::Log::Error(message);
		});

		log.set_function("Critical", [&](sol::this_state s, std::string_view message) {
			Lumos::Debug::Log::Critical(message);
		});
	}

	void LuaManager::BindInputLua(sol::state& state)
	{
		auto input = state["Input"].get_or_create<sol::table>();

		input.set_function("GetKeyPressed", [](Lumos::InputCode::Key key) -> bool {
			return Input::GetInput()->GetKeyPressed(key);
		});

		input.set_function("GetKeyHeld", [](Lumos::InputCode::Key key) -> bool {
			return Input::GetInput()->GetKeyHeld(key);
		});

		input.set_function("GetMouseClicked", [](Lumos::InputCode::MouseKey key) -> bool {
			return Input::GetInput()->GetMouseClicked(key);
		});

		input.set_function("GetMouseHeld", [](Lumos::InputCode::MouseKey key) -> bool {
			return Input::GetInput()->GetMouseHeld(key);
		});

		input.set_function("GetMousePosition", []() -> Maths::Vector2 {
			return Input::GetInput()->GetMousePosition();
		});

		input.set_function("GetScrollOffset", []() -> float {
			return Input::GetInput()->GetScrollOffset();
		});

		std::initializer_list<std::pair<sol::string_view, Lumos::InputCode::Key>> keyItems =
			{
				{"A", Lumos::InputCode::Key::A},
				{"B", Lumos::InputCode::Key::B},
				{"C", Lumos::InputCode::Key::C},
				{"D", Lumos::InputCode::Key::D},
				{"E", Lumos::InputCode::Key::E},
				{"F", Lumos::InputCode::Key::F},
				{"H", Lumos::InputCode::Key::G},
				{"G", Lumos::InputCode::Key::H},
				{"I", Lumos::InputCode::Key::I},
				{"J", Lumos::InputCode::Key::J},
				{"K", Lumos::InputCode::Key::K},
				{"L", Lumos::InputCode::Key::L},
				{"M", Lumos::InputCode::Key::M},
				{"N", Lumos::InputCode::Key::N},
				{"O", Lumos::InputCode::Key::O},
				{"P", Lumos::InputCode::Key::P},
				{"Q", Lumos::InputCode::Key::Q},
				{"R", Lumos::InputCode::Key::R},
				{"S", Lumos::InputCode::Key::S},
				{"T", Lumos::InputCode::Key::T},
				{"U", Lumos::InputCode::Key::U},
				{"V", Lumos::InputCode::Key::V},
				{"W", Lumos::InputCode::Key::W},
				{"X", Lumos::InputCode::Key::X},
				{"Y", Lumos::InputCode::Key::Y},
				{"Z", Lumos::InputCode::Key::Z},
				//{ "UNKOWN", Lumos::InputCode::Key::Unknown },
				{"Space", Lumos::InputCode::Key::Space},
				{"Escape", Lumos::InputCode::Key::Escape},
				{ "APOSTROPHE", Lumos::InputCode::Key::Apostrophe },
				{"Comma", Lumos::InputCode::Key::Comma},
				{ "MINUS", Lumos::InputCode::Key::Minus },
				{ "PERIOD", Lumos::InputCode::Key::Period },
				{ "SLASH", Lumos::InputCode::Key::Slash },
				{ "SEMICOLON", Lumos::InputCode::Key::Semicolon },
				{ "EQUAL", Lumos::InputCode::Key::Equal },
				{ "LEFT_BRACKET", Lumos::InputCode::Key::LeftBracket },
				{ "BACKSLASH", Lumos::InputCode::Key::Backslash },
				{ "RIGHT_BRACKET", Lumos::InputCode::Key::RightBracket },
				//{ "BACK_TICK", Lumos::InputCode::Key::BackTick },
				{"Enter", Lumos::InputCode::Key::Enter},
				{"Tab", Lumos::InputCode::Key::Tab},
				{"Backspace", Lumos::InputCode::Key::Backspace},
				{"Insert", Lumos::InputCode::Key::Insert},
				{"Delete", Lumos::InputCode::Key::Delete},
				{"Right", Lumos::InputCode::Key::Right},
				{"Left", Lumos::InputCode::Key::Left},
				{"Down", Lumos::InputCode::Key::Down},
				{"Up", Lumos::InputCode::Key::Up},
				{"PageUp", Lumos::InputCode::Key::PageUp},
				{"PageDown", Lumos::InputCode::Key::PageDown},
				{"Home", Lumos::InputCode::Key::Home},
				{"End", Lumos::InputCode::Key::End},
				{ "CAPS_LOCK", Lumos::InputCode::Key::CapsLock },
				{ "SCROLL_LOCK", Lumos::InputCode::Key::ScrollLock },
				{"NumLock", Lumos::InputCode::Key::NumLock},
				{"PrintScreen", Lumos::InputCode::Key::PrintScreen},
				{"Pasue", Lumos::InputCode::Key::Pause},
				{"LeftShift", Lumos::InputCode::Key::LeftShift},
				{"LeftControl", Lumos::InputCode::Key::LeftControl},
				{ "LEFT_ALT", Lumos::InputCode::Key::LeftAlt },
				{ "LEFT_SUPER", Lumos::InputCode::Key::LeftSuper },
				{"RightShift", Lumos::InputCode::Key::RightShift},
				{"RightControl", Lumos::InputCode::Key::RightControl},
				{ "RIGHT_ALT", Lumos::InputCode::Key::RightAlt },
				{ "RIGHT_SUPER", Lumos::InputCode::Key::RightSuper },
				{"Menu", Lumos::InputCode::Key::Menu},
				{"F1", Lumos::InputCode::Key::F1},
				{"F2", Lumos::InputCode::Key::F2},
				{"F3", Lumos::InputCode::Key::F3},
				{"F4", Lumos::InputCode::Key::F4},
				{"F5", Lumos::InputCode::Key::F5},
				{"F6", Lumos::InputCode::Key::F6},
				{"F7", Lumos::InputCode::Key::F7},
				{"F8", Lumos::InputCode::Key::F8},
				{"F9", Lumos::InputCode::Key::F9},
				{"F10", Lumos::InputCode::Key::F10},
				{"F11", Lumos::InputCode::Key::F11},
				{"F12", Lumos::InputCode::Key::F12},
				{"Keypad0", Lumos::InputCode::Key::D0},
				{"Keypad1", Lumos::InputCode::Key::D1},
				{"Keypad2", Lumos::InputCode::Key::D2},
				{"Keypad3", Lumos::InputCode::Key::D3},
				{"Keypad4", Lumos::InputCode::Key::D4},
				{"Keypad5", Lumos::InputCode::Key::D5},
				{"Keypad6", Lumos::InputCode::Key::D6},
				{"Keypad7", Lumos::InputCode::Key::D7},
				{"Keypad8", Lumos::InputCode::Key::D8},
				{"Keypad9", Lumos::InputCode::Key::D9},
                {"Decimal", Lumos::InputCode::Key::Period},
                {"Divide", Lumos::InputCode::Key::Slash},
                {"Multiply", Lumos::InputCode::Key::KPMultiply},
                {"Subtract", Lumos::InputCode::Key::Minus},
				{"Add", Lumos::InputCode::Key::KPAdd},
				{ "KP_EQUAL",    Lumos::InputCode::Key::KPEqual }
			};
		state.new_enum<Lumos::InputCode::Key, false>("Key", keyItems); // false makes it read/write in Lua, but its faster

		std::initializer_list<std::pair<sol::string_view, Lumos::InputCode::MouseKey>> mouseItems =
			{
				{"Left", Lumos::InputCode::MouseKey::ButtonLeft},
				{"Right", Lumos::InputCode::MouseKey::ButtonRight},
				{"Middle", Lumos::InputCode::MouseKey::ButtonMiddle},
			};
		state.new_enum<Lumos::InputCode::MouseKey, false>("MouseButton", mouseItems);
	}

	Ref<Graphics::Texture2D> LoadTexture(const std::string& name, const std::string& path)
	{
		return Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path));
	}

	Ref<Graphics::Texture2D> LoadTextureWithParams(const std::string& name, const std::string& path, Lumos::Graphics::TextureFilter filter, Lumos::Graphics::TextureWrap wrapMode)
	{
		return Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(name, path, Graphics::TextureParameters(filter, filter, wrapMode)));
	}

	void LuaManager::BindECSLua(sol::state& state)
	{
		sol::usertype<entt::registry> reg_type = state.new_usertype<entt::registry>("Registry");
		reg_type.set_function("Create", static_cast<entt::entity (entt::registry::*)()>(&entt::registry::create));
		reg_type.set_function("Destroy", static_cast<void (entt::registry::*)(entt::entity)>(&entt::registry::destroy));
		reg_type.set_function("Valid", &entt::registry::valid);

		state.set_function("GetEntityByName", &GetEntityByName);

		sol::usertype<NameComponent> nameComponent_type = state.new_usertype<NameComponent>("NameComponent");
		nameComponent_type["name"] = &NameComponent::name;
		REGISTER_COMPONENT_WITH_ECS(state, NameComponent, static_cast<NameComponent& (entt::registry::*)(const entt::entity)>(&entt::registry::emplace<NameComponent>));

		sol::usertype<LuaScriptComponent> script_type = state.new_usertype<LuaScriptComponent>("LuaScriptComponent", sol::constructors<sol::types<std::string, Scene*>>());
		REGISTER_COMPONENT_WITH_ECS(state, LuaScriptComponent, static_cast<LuaScriptComponent& (entt::registry::*)(const entt::entity, std::string&&, Scene*&&)>(&entt::registry::emplace<LuaScriptComponent, std::string, Scene*>));

		using namespace Maths;
		REGISTER_COMPONENT_WITH_ECS(state, Transform, static_cast<Transform& (entt::registry::*)(const entt::entity)>(&entt::registry::emplace<Transform>));

		using namespace Graphics;
		sol::usertype<Sprite> sprite_type = state.new_usertype<Sprite>("Sprite", sol::constructors<sol::types<Maths::Vector2, Maths::Vector2, Maths::Vector4>, Sprite(const Ref<Graphics::Texture2D>&, const Maths::Vector2&, const Maths::Vector2&, const Maths::Vector4&)>());
		sprite_type.set_function("SetTexture", &Sprite::SetTexture);

		REGISTER_COMPONENT_WITH_ECS(state, Sprite, static_cast<Sprite& (entt::registry::*)(const entt::entity, const Vector2&, const Vector2&, const Vector4&)>(&entt::registry::emplace<Sprite, const Vector2&, const Vector2&, const Vector4&>));

		REGISTER_COMPONENT_WITH_ECS(state, Light, static_cast<Light& (entt::registry::*)(const entt::entity)>(&entt::registry::emplace<Light>));

		sol::usertype<MeshComponent> meshComponent_type = state.new_usertype<MeshComponent>("MeshComponent");
		meshComponent_type["SetMesh"] = &MeshComponent::SetMesh;

		REGISTER_COMPONENT_WITH_ECS(state, MeshComponent, static_cast<MeshComponent& (entt::registry::*)(const entt::entity)>(&entt::registry::emplace<MeshComponent>));

		sol::usertype<Camera> camera_type = state.new_usertype<Camera>("Camera", sol::constructors<Camera(float, float, float, float), Camera(float, float)>());
		camera_type["fov"] = &Camera::GetFOV;
		camera_type["aspectRatio"] = &Camera::GetAspectRatio;
		camera_type["nearPlane"] = &Camera::GetNear;
		camera_type["farPlane"] = &Camera::GetFar;
        camera_type["SetIsOrthographic"] = &Camera::SetIsOrthographic;
        camera_type["SetNearPlane"] = &Camera::SetNear;
        camera_type["SetFarPlane"] = &Camera::SetFar;
    
        REGISTER_COMPONENT_WITH_ECS(state, Camera, static_cast<Camera& (entt::registry::*)(const entt::entity, const float&, const float&)>(&entt::registry::emplace<Camera, const float&, const float&>));
    
        sol::usertype<Physics3DComponent> physics3DComponent_type = state.new_usertype<Physics3DComponent>("Physics3DComponent", sol::constructors<sol::types<const Ref<RigidBody3D>&>>());
        physics3DComponent_type.set_function("GetRigidBody", &Physics3DComponent::GetRigidBody);
		REGISTER_COMPONENT_WITH_ECS(state, Physics3DComponent, static_cast<Physics3DComponent& (entt::registry::*)(const entt::entity,Ref<RigidBody3D>&)>(&entt::registry::emplace<Physics3DComponent, Ref<RigidBody3D>&>));

		sol::usertype<Physics2DComponent> physics2DComponent_type = state.new_usertype<Physics2DComponent>("Physics2DComponent", sol::constructors<sol::types<const Ref<RigidBody2D>&>>());
		physics2DComponent_type.set_function("GetRigidBody", &Physics2DComponent::GetRigidBody);

		REGISTER_COMPONENT_WITH_ECS(state, Physics2DComponent, static_cast<Physics2DComponent& (entt::registry::*)(const entt::entity, Ref<RigidBody2D>&)>(&entt::registry::emplace<Physics2DComponent, Ref<RigidBody2D>&>));

		REGISTER_COMPONENT_WITH_ECS(state, SoundComponent, static_cast<SoundComponent& (entt::registry::*)(const entt::entity)>(&entt::registry::emplace<SoundComponent>));
		REGISTER_COMPONENT_WITH_ECS(state, MaterialComponent, static_cast<MaterialComponent& (entt::registry::*)(const entt::entity)>(&entt::registry::emplace<MaterialComponent>));

		state.set_function("LoadMesh", &ModelLoader::LoadModel);

		sol::usertype<Graphics::Mesh> mesh_type = state.new_usertype<Graphics::Mesh>("Mesh");

		std::initializer_list<std::pair<sol::string_view, Lumos::Graphics::PrimitiveType>> primitives =
			{
				{"Cube", Lumos::Graphics::PrimitiveType::Cube},
				{"Plane", Lumos::Graphics::PrimitiveType::Plane},
				{"Quad", Lumos::Graphics::PrimitiveType::Quad},
				{"Pyramid", Lumos::Graphics::PrimitiveType::Pyramid},
				{"Sphere", Lumos::Graphics::PrimitiveType::Sphere},
				{"Capsule", Lumos::Graphics::PrimitiveType::Capsule},
				{"Cylinder", Lumos::Graphics::PrimitiveType::Cylinder},

			};

		std::initializer_list<std::pair<sol::string_view, Lumos::Graphics::TextureFilter>> textureFilter =
			{
				{"None", Lumos::Graphics::TextureFilter::NONE},
				{"Linear", Lumos::Graphics::TextureFilter::LINEAR},
				{"Nearest", Lumos::Graphics::TextureFilter::NEAREST}};

		std::initializer_list<std::pair<sol::string_view, Lumos::Graphics::TextureWrap>> textureWrap =
			{
				{"None", Lumos::Graphics::TextureWrap::NONE},
				{"Repeat", Lumos::Graphics::TextureWrap::REPEAT},
				{"Clamp", Lumos::Graphics::TextureWrap::CLAMP},
				{"MorroredRepeat", Lumos::Graphics::TextureWrap::MIRRORED_REPEAT},
				{"ClampToEdge", Lumos::Graphics::TextureWrap::CLAMP_TO_EDGE},
				{"ClampToBorder", Lumos::Graphics::TextureWrap::CLAMP_TO_BORDER}};

		state.new_enum<Lumos::Graphics::PrimitiveType, false>("PrimitiveType", primitives);
		state.set_function("LoadMesh", &CreatePrimative);

		state.new_enum<Lumos::Graphics::TextureWrap, false>("TextureWrap", textureWrap);
		state.new_enum<Lumos::Graphics::TextureFilter, false>("TextureFilter", textureFilter);

		state.set_function("LoadTexture", &LoadTexture);
		state.set_function("LoadTextureWithParams", &LoadTextureWithParams);
	}

	static float LuaRand(float a, float b)
	{
		return RandomNumberGenerator32::Rand(a, b);
	}

	void LuaManager::BindSceneLua(sol::state& state)
	{
		sol::usertype<Scene> scene_type = state.new_usertype<Scene>("Scene");
		scene_type.set_function("GetRegistry", &Scene::GetRegistry);

		sol::usertype<Graphics::Texture2D> texture2D_type = state.new_usertype<Graphics::Texture2D>("Texture2D");
		texture2D_type.set_function("CreateFromFile", &Graphics::Texture2D::CreateFromFile);

		state.set_function("Rand", &LuaRand);
	}

	static void SwitchSceneByIndex(int index)
	{
		Application::Get().GetSceneManager()->SwitchScene(index);
	}

	static void SwitchSceneByName(const std::string& name)
	{
		Application::Get().GetSceneManager()->SwitchScene(name);
	}

	void LuaManager::BindAppLua(sol::state& state)
	{
		sol::usertype<Application> app_type = state.new_usertype<Application>("Application");
		state.set_function("SwitchSceneByIndex", &SwitchSceneByIndex);
		state.set_function("SwitchSceneByName", &SwitchSceneByName);

		state.set_function("GetAppInstance", &Application::Get);
	}
}
