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
#include <render/api.h>
#include <physics/physics.h>
#include <entities/player.h>
#include <entities/staticworldobject.h>
#include <entities/light.h>
#include <entities/crate.h>
#include <entities/marker.h>
#include <entities/sound.h>
#include <entities/decoration.h>
#include <entities/trigger.h>
#include <components/player.h>
#include <components/controller.h>
#include <components/render.h>
#include <extensions/camera/camera.h>
#include <extensions/menu/menu.h>
#include <extensions/kitchensink/design.h>
#include <extensions/kitchensink/entities.h>

#include <platform/time.h>

#include <cstring>

#include "quest.h"

using namespace tram;
using namespace tram::UI;
using namespace tram::Render;
using namespace tram::Physics;
using namespace tram::GUI;

const int INTRO_LENGTH = 240;
bool skip_intro = false;
Player* player = nullptr;
Quest* froggy_quest = nullptr;

void main_loop();

int main(int argc, const char** argv) {
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0) {
			std::cout << "game [flags]";
			std::cout << "\n\t--help display this message";
			std::cout << "\n\t-nointro skip intro" << std::endl;
		}
		if (strcmp(argv[i], "-nointro") == 0)  {
			std::cout << "Skipping intro..." << std::endl;
			skip_intro = true;
		}
	}
	
	SetSystemLoggingSeverity(System::SYSTEM_PLATFORM, SEVERITY_WARNING);
	
	StaticWorldObject::Register();
	Light::Register();
	Crate::Register();
	Marker::Register();
	Sound::Register();
	Decoration::Register();
	Trigger::Register();
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

	// I forgot to make the engine load animations automatically
	Animation::Find("froggy-idle")->Load();
	Animation::Find("froggy-wave")->Load();
	Animation::Find("aberration")->Load();
	Animation::Find("cat-waiting")->Load();
	Animation::Find("cat-disappointed")->Load();
	Animation::Find("cat-hand")->Load();
	
	Render::SetSunDirection(glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f)));
	Render::SetSunColor(glm::vec3(250.0f, 214.0f, 165.0f) / 256.0f * 0.8f);
	Render::SetAmbientColor((glm::vec3(250.0f, 214.0f, 165.0f) / 256.0f * 0.8f) * 0.7f);
	Render::SetScreenClear(glm::vec3(250.0f, 214.0f, 165.0f) / 256.0f * 0.8f, true);
	
	// TODO: fix segfault in engine AABBtree RemoveLeaf and then split up worldcells again
	// I hope that keeping everything loaded all the time won't lag the game too much
	WorldCell* house = WorldCell::Make("house");
	//WorldCell* tunnel = WorldCell::Make("tunnel");
	//WorldCell* crevasse = WorldCell::Make("crevasse");
	house->LoadFromDisk();
	//tunnel->LoadFromDisk();
	//crevasse->LoadFromDisk();

	player = new Player;
	//player->SetLocation(vec3(0.0f, (1.85f/2.0f) + 0.05f, 0.0f));
	//player->SetLocation(vec3(0.0f, (1.85f/2.0f) + 10.05f, 0.0f));
	player->SetLocation(Entity::Find("player-start-2")->GetLocation());
	//player->SetLocation(vec3(0.0f, (1.85f/2.0f) + 10.05f, 0.0f));
	player->Load();

	player->controllercomponent->SetWalkSpeed(0.05f);
	player->controllercomponent->SetCrouchSpeed(0.01f);
	player->controllercomponent->SetRunSpeed(0.1f);
	
	Ext::Camera::Camera* camera = new Ext::Camera::Camera;
	camera->SetMouselook(true);
	camera->SetRotateFollowing(true);
	camera->SetFollowingOffset({0.0f, 0.5f, 0.0f});
	camera->SetFollowing(player);

	camera->SetBobbingDistance(0.0f);
	
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
		Text("Press\b[E]\bto\bactivate.", 4, TEXT_CENTER);
		GUI::EndFrame();
    });
	
	//Physics::DRAW_PHYSICS_DEBUG = true;
	
	// in the end this would be loaded in from a file, but since this is only a
	// prototype, we can skip that and just hard-code everything
	froggy_quest = Quest::Find("froggy-quest");
	froggy_quest->name = "froggy-quest";
	froggy_quest->variables = {
		{"has-cake", false},
		{"frogs-fed", 0},
		{"lock-player", false},
		{"outro", false},
	};
	froggy_quest->triggers = {
		
		// if has-cake is false, set has-cake to true.
		// this will be triggered when the cake is activated
		{"pick-up-cake", {
			{.type = TriggerAction::QUEST_VARIABLE_SET, .quest = "froggy-quest", 
				.variable = "has-cake", .variable_value = true
			},
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Got\bcake!"
			}
		}, {
			{.type = TriggerCondition::QUEST_VARIABLE_IS, .quest = "froggy-quest", 
				.variable = "has-cake", .variable_value = false
			}
		}},
		
		// this will be fired when you feed the frogs.
		// for the final version of the quest system thing, it would be a good
		// idea to add some kind of a "increment variable" operation 
		{"feed-frog-3", {
			// perform
			{.type = TriggerAction::QUEST_VARIABLE_SET, .quest = "froggy-quest", 
				.variable = "frogs-fed", .variable_value = 3
			},
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Fed\bcake\bto\bfrog\b(3/3)"
			},
			{.type = TriggerAction::SEND_MESSAGE,
				.msg = {
					.type = Message::UNLOCK,
					.sender = 0,
					.receiver = Entity::Find("trigger-start-bath")->GetID()
				}
			}
		}, {
			// if
			{.type = TriggerCondition::QUEST_VARIABLE_IS, .quest = "froggy-quest", 
				.variable = "frogs-fed", .variable_value = 2
			}
		}},
		{"feed-frog-2", {
			// perform
			{.type = TriggerAction::QUEST_VARIABLE_SET, .quest = "froggy-quest", 
				.variable = "frogs-fed", .variable_value = 2
			},
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Fed\bcake\bto\bfrog\b(2/3)"
			}
		}, {
			// if
			{.type = TriggerCondition::QUEST_VARIABLE_IS, .quest = "froggy-quest", 
				.variable = "frogs-fed", .variable_value = 1
			}
		}},
		{"feed-frog-1", {
			// perform
			{.type = TriggerAction::QUEST_VARIABLE_SET, .quest = "froggy-quest", 
				.variable = "frogs-fed", .variable_value = 1
			},
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Fed\bcake\bto\bfrog\b(1/3)"
			}
		}, {
			// if
			{.type = TriggerCondition::QUEST_VARIABLE_IS, .quest = "froggy-quest", 
				.variable = "frogs-fed", .variable_value = 0
			}
		}},
		
		{"bath-faucet-locked", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "A\bmysterious\bforce\bprevents\byou\bfrom sequence-breaking..."
			}
		}, {}},
		
		{"feed-frogs", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Feed\bthe\bfrogs."
			}
		}, {}},
		
		{"get-cake", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Check\bthe\bfridge\bfor\bfood."
			}
		}, {}},
		
		// triggered when going into the house after feeding the frogs
		{"start-bath", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "It\bis\btime\bto\bgive\bthe\bfrogs\ba\bbath!"
			}
		}, {}},
		
		// triggered when filling the bath
		{"get-frogs", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Get\bthe\bfrogs."
			}
		}, {}},
		
		// triggered when coming out of the house after filling bath
		{"where-frogs-1", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Where\bare\bthe\bfrogs?"
			}
		}, {}},
		{"where-frogs-2", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "What\bdid\byou\bdo?"
			}
		}, {}},
		{"where-frogs-3", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Why\bdo\byou\balways\bhave\bto\bruin\beverything..."
			}
		}, {}},
		{"where-frogs-4", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Go\bfind\bthem!"
			}
		}, {}},
		
		// triggered when triggered
		{"aberration-warning", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Do\bnot\btouch\bthe\baberration,\bit\bis\bdangerous!"
			}
		}, {}},
		{"aberration-1", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "That\bwas\breally\bdumb...\byou\bcould\bhave\bdied..."
			}
		}, {}},
		{"clumsy", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Why\bare\byou\bso\bclumsy?"
			}
		}, {}},
		{"toilet-warning", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Do\bnot\btouch\bmy\bspecial\btoilet!!!"
			}
		}, {}},
		{"toilet-toilet", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Was\bit\breally\bso\bhard\bnot\bto\bdo\bit?"
			}
		}, {}},
		
		{"trigger-end-1", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "I\bfound\ball\bthe\bfrogs\bmyself."
			},
			{.type = TriggerAction::QUEST_VARIABLE_SET, .quest = "froggy-quest", 
				.variable = "lock-player", .variable_value = true
			}
		}, {}},
		{"trigger-end-2", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Without\byour\bhelp."
			}
		}, {}},
		{"trigger-end-3", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "Give\bhand."
			}
		}, {}},
		{"trigger-end-4", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "We\bare\bwalking\boff\binto\bthe\bsunset."
			}
		}, {}},
		{"trigger-end-5", {
			{.type = TriggerAction::SHOW_MESSAGE,
				.message = "THE\bEND"
			},
			{.type = TriggerAction::QUEST_VARIABLE_SET, .quest = "froggy-quest", 
				.variable = "outro", .variable_value = true
			}
		}, {}},
		
		
	};
	
	froggy_quest->Init();
	
	
	#ifdef __EMSCRIPTEN__
		UI::SetWebMainLoop(main_loop);
	#else
		while (!EXIT) {
			main_loop();
		}

		Async::Yeet();
		Audio::Uninit();
		UI::Uninit();
	#endif
}

void main_loop() {
	Core::Update();
	UI::Update();
	
	Physics::Update();
	
	GUI::Begin();
	Ext::Menu::DebugMenu();
	Ext::Menu::EscapeMenu();

	Quest::Update();
	
	GUI::End();
	GUI::Update();
	
	Async::ResourceLoader2ndStage();
	Async::FinishResource();
	
	Event::Dispatch();
	Message::Dispatch();
	
	Entity::UpdateFromList();

	Loader::Update();

	
	// intro and outro animations
	if ((GetTick() > INTRO_LENGTH || skip_intro) && !froggy_quest->GetVariable("lock-player")) {
		ControllerComponent::Update();
		Ext::Camera::Update();
	} else if (GetTick() <= INTRO_LENGTH) {
		vec3 begin_pos = Entity::Find("player-start-1")->GetLocation();
		vec3 end_pos = Entity::Find("player-start-2")->GetLocation();
		end_pos.y += 0.5f;
		quat begin_rot = vec3(1.57f, 1.57f, 0.0f);
		quat end_rot = vec3(0.0f, 0.0f, 0.0f);
		
		float camera_mix = (float)GetTick()/(float)INTRO_LENGTH;
		
		vec3 camera_pos = glm::mix(begin_pos, end_pos, camera_mix);
		quat camera_rot = glm::mix(begin_rot, end_rot, camera_mix);
		
		Render::SetCameraPosition(camera_pos);
		Render::SetCameraRotation(camera_rot);
	} else {
		vec3 from = Render::GetCameraPosition();
		vec3 to = Entity::Find("catter")->GetLocation() + vec3(0.0f, 1.0f, 0.0f);
		vec3 dir = glm::normalize(to - from);
		
		quat camera_target = glm::quatLookAt(dir, DIRECTION_UP);
		quat camera_current = Render::GetCameraRotation();
		
		if (froggy_quest->GetVariable("outro")) {
			camera_target = vec3(1.57f, 0.0f, 0.0f);
		}
		
		quat camera_rot = glm::mix(camera_target, camera_current, 0.99f);
		
		Render::SetCameraRotation(camera_rot);
	}
	
	// this will place a player in a safe place, if they fall through the
	// ground
	if (vec3 player_pos = player->GetLocation(); player_pos.y < -5.0f) {
		std::cout << "Player fell out of the world!" << std::endl;
		vec3 recovery_1 = Entity::Find("recovery-1")->GetLocation();
		vec3 recovery_2 = Entity::Find("recovery-2")->GetLocation();
		vec3 recovery_3 = Entity::Find("recovery-3")->GetLocation();
		
		vec3 nearest = recovery_1;
		if (glm::distance(player_pos, recovery_2) < glm::distance(player_pos, nearest)) nearest = recovery_2;
		if (glm::distance(player_pos, recovery_3) < glm::distance(player_pos, nearest)) nearest = recovery_3;
		
		player->SetLocation(nearest);
	}
	
	AnimationComponent::Update();
	
	Render::Render();
	UI::EndFrame();
}