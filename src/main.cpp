#include <framework/core.h>
#include <framework/logging.h>
#include <framework/ui.h>
#include <framework/gui.h>
#include <framework/async.h>
#include <framework/event.h>
#include <framework/message.h>
#include <framework/system.h>
#include <framework/worldcell.h>
#include <framework/language.h>
#include <audio/audio.h>
#include <render/render.h>
#include <render/material.h>
#include <physics/physics.h>
#include <entities/player.h>
#include <entities/staticworldobject.h>
#include <entities/light.h>
#include <entities/crate.h>
#include <entities/marker.h>
#include <entities/sound.h>
#include <entities/decoration.h>
#include <components/player.h>
#include <components/controller.h>
#include <components/render.h>
#include <extensions/camera/camera.h>
#include <extensions/menu/menu.h>
#include <extensions/kitchensink/design.h>
#include <extensions/kitchensink/entities.h>

using namespace tram;
using namespace tram::UI;
using namespace tram::Render;
using namespace tram::Physics;
using namespace tram::GUI;

int main() {
	SetSystemLoggingSeverity(System::SYSTEM_PLATFORM, SEVERITY_WARNING);
	
	StaticWorldObject::Register();
	Light::Register();
	Crate::Register();
	Marker::Register();
	Sound::Register();
	Decoration::Register();
	Ext::Design::Button::Register();

	Core::Init();
	UI::Init();
	Render::Init();
	Physics::Init();
	Async::Init();
	Audio::Init();
	GUI::Init();

	Ext::Menu::Init();
	Ext::Camera::Init();
	Ext::Design::Init();

	Material::LoadMaterialInfo("material");
	Language::Load("english");


	Animation::Find("froggy-idle")->Load();
	Animation::Find("froggy-wave")->Load();
	Animation::Find("NodHead")->Load();
	Animation::Find("Flip")->Load();
	
	Render::SetSunDirection(glm::normalize(glm::vec3(0.0f, 1.0f, 0.5f)));
	Render::SetSunColor(glm::vec3(250.0f, 214.0f, 165.0f) / 256.0f * 0.8f);
	Render::SetAmbientColor((glm::vec3(250.0f, 214.0f, 165.0f) / 256.0f * 0.8f) * 0.7f);

	WorldCell* house = WorldCell::Make("house");
	house->LoadFromDisk();

	Player* player = new Player;
	//player->SetLocation(vec3(0.0f, (1.85f/2.0f) + 0.05f, 0.0f));
	//player->SetLocation(vec3(0.0f, (1.85f/2.0f) + 10.05f, 0.0f));
	//player->SetLocation(Entity::Find("player-start")->GetLocation());
	player->SetLocation(vec3(0.0f, (1.85f/2.0f) + 10.05f, 0.0f));
	player->Load();

	Ext::Camera::Camera* camera = new Ext::Camera::Camera;
	camera->SetMouselook(true);
	camera->SetRotateFollowing(true);
	camera->SetFollowingOffset({0.0f, 0.5f, 0.0f});
	camera->SetFollowing(player);

	Ext::Camera::SetCamera(camera);

	// when you hold down the E key
	Event::AddListener(Event::KEYPRESS, [](Event& event) {
        if (event.subtype != KEY_ACTION_ACTIVATE) return;
        
        vec3 start = Render::GetCameraPosition();
        vec3 direction = Render::GetCameraRotation() * DIRECTION_FORWARD;
        
        auto result = Physics::Raycast(start, start + 2.0f * direction, -1 ^ Physics::COLL_TRIGGER);
        
        if (result.collider) {
            Message::Send({Message::ACTIVATE, 0, result.collider->GetParent()->GetID(), 0});
        }
    });
    
	// when you press the E key
    Event::AddListener(Event::KEYDOWN, [](Event& event) {
        if (event.subtype != KEY_ACTION_ACTIVATE) return;
        
        vec3 start = Render::GetCameraPosition();
        vec3 direction = Render::GetCameraRotation() * DIRECTION_FORWARD;
        
        auto result = Physics::Raycast(start, start + 2.0f * direction, -1 ^ Physics::COLL_TRIGGER);
        
        if (result.collider) {
            Message::Send({Message::ACTIVATE_ONCE, 0, result.collider->GetParent()->GetID(), 0});
        }
    });
	
	// selects stuff
	Event::AddListener(Event::TICK, [](Event& event) {
        vec3 start = Render::GetCameraPosition();
        vec3 direction = Render::GetCameraRotation() * DIRECTION_FORWARD;
        
        auto result = Physics::Raycast(start, start + 2.0f * direction, -1 ^ Physics::COLL_TRIGGER);

        if (result.collider) {
            Message::Send({Message::SELECT, 0, result.collider->GetParent()->GetID(), 0});
        }
    });
    
	// shows the select
    Event::AddListener(Event::SELECTED, [](Event& event) {
		GUI::Frame(FRAME_BOTTOM, 100);
		Text("Press [E] to activate.", 4, TEXT_CENTER);
		GUI::EndFrame();
    });
	
	//Physics::DRAW_PHYSICS_DEBUG = true;
	
	while (!EXIT) {
		Core::Update();
		UI::Update();
		Physics::Update();
		
		Ext::Camera::Update();
		
		GUI::Begin();
		Ext::Menu::DebugMenu();
		Ext::Menu::EscapeMenu();

		GUI::End();
		GUI::Update();
		
		Async::ResourceLoader2ndStage();
		Async::FinishResource();
		
		Event::Dispatch();
		Message::Dispatch();
		
		Entity::UpdateFromList();

		Loader::Update();

		ControllerComponent::Update();
		AnimationComponent::Update();
		
		//ControllerComponent::Update();
		
		Render::Render();
		UI::EndFrame();
	}
	
	Async::Yeet();
}