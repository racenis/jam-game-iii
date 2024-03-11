#ifndef QUEST_H
#define QUEST_H

#include <framework/uid.h>
#include <framework/value.h>
#include <framework/message.h>
#include <vector>

using namespace tram;

// experimental prototype quest abstraction thing

// the end goal is to create a no-code quest thing, similar to what you get
// with rpg maker or other engines

struct TriggerCondition {
	enum Type {
		QUEST_STAGE_IS,
		QUEST_VARIABLE_IS
	};
	
	bool Valid();
	
	Type type;
	name_t quest;
	name_t variable;
	value_t variable_value;
};

struct TriggerAction {
	enum Type {
		QUEST_VARIABLE_SET,
		SHOW_MESSAGE,
		SEND_MESSAGE
	};
	
	void Perform();
	
	Type type;
	name_t quest;
	name_t variable;
	value_t variable_value;
	const char* message;
	Message msg;
};

struct QuestTrigger {
	name_t name;
	
	void Fire();
	
	std::vector<TriggerAction> actions;
	std::vector<TriggerCondition> conditions;
};

struct Quest {
	void FireTrigger(name_t trigger);
	void SetVariable(name_t variable, Value value);
	Value GetVariable(name_t variable);

	static Quest* Find(name_t quest);
	
	void Init();
	
	static void Update();
	
	name_t name;
	std::vector<std::pair<name_t, value_t>> variables;
	std::vector<QuestTrigger> triggers;
};

#endif // QUEST_H