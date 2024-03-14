#include "quest.h"

#include <framework/entity.h>
#include <framework/gui.h>
#include <templates/pool.h>

#include <cstring>

// quest methods and stuff

static const char* message_displayed = nullptr;
static size_t message_progress = 0;

bool TriggerCondition::Valid() {
	switch (type) {
		case QUEST_STAGE_IS:
			std::cout << "not implemetn" << std::endl; abort();
			break;
		case QUEST_VARIABLE_IS:
			return Quest::Find(quest)->GetVariable(variable) == variable_value;
			break;
		
	}
	return false;
}

void TriggerAction::Perform() {
	switch (type) {
		case QUEST_VARIABLE_SET:
			Quest::Find(quest)->SetVariable(variable, variable_value);
			std::cout << "Setting " << quest << " variable " << variable << std::endl;
			break;
		case SHOW_MESSAGE:
			std::cout << "Showing message: " << message << std::endl;
			message_displayed = message;
			message_progress = 0;
			break;
		case SEND_MESSAGE:
			std::cout << "Sending message to entity " << Message::GetName(msg.type) << std::endl;
			Message::Send(msg);
			break;
	}
}

void QuestTrigger::Fire() {
	for (auto& cond : conditions) {
		if (!cond.Valid()) return;
	}
	
	std::cout << "Firing quest trigger "<< name << std::endl;
	
	for (auto& act : actions) {
		act.Perform();
	}
}

void Quest::FireTrigger(name_t trigger) {
	for (auto& tri : triggers) {
		if (tri.name == trigger) {
			tri.Fire();
		}
	}
}

void Quest::SetVariable(name_t variable, Value value) {
	for (auto& var : variables) {
		if (var.first == variable) {
			var.second = value;
			return;
		}
	}
}

Value Quest::GetVariable(name_t variable) {
	for (auto& var : variables) {
		if (var.first == variable) {
			return var.second;
		}
	}
	
	std::cout << "Can't find " << variable << " in " << name << std::endl;
	abort();
}

template<> Pool<Quest> PoolProxy<Quest>::pool("qyuespool", 10);
Quest* Quest::Find(name_t quest) {
	for (auto& q : PoolProxy<Quest>::GetPool()) {
		if (q.name == quest) return &q;
	}
	
	return PoolProxy<Quest>::New();
}

void Quest::Update() {
	using namespace tram::GUI;
	
	if (message_displayed) {
		char display_buf[200];
		strncpy(display_buf, message_displayed, ++message_progress);
		display_buf[message_progress] = '\0';
		
		Frame(FRAME_BOTTOM, 300);
		Text(display_buf, 4, TEXT_CENTER);
		EndFrame();
		
		if (message_progress >= 120) {
			message_displayed = nullptr;
		}
	}
	

}

// quest entity stuff

// we need quest to have an entity, since in this engine only entities can 
// receive messages. we need to receive messages, since that is how buttons
// and stuff work and we need to trigger quest stuff from buttons and stuff

class QuestEntity : public Entity {
public:
	QuestEntity(name_t name) : Entity(name) {}
    void UpdateParameters() {}
    void SetParameters() {}
    void Load() {}
    void Unload() {}
    void Serialize() {}
    void MessageHandler(Message& msg) {
		name_t trigger = *(Value*)msg.data;
		std::cout << name << " triggered " << trigger << std::endl;
		Quest::Find(name)->FireTrigger(trigger);
	}
};

void Quest::Init() {
	new QuestEntity(name);
}