forward OnCreate(ID);
forward OnDestroy(ID);
forward OnSpawn(ID);
forward OnActivate(ID, actor);
forward OnCellChange(ID, cell);
forward OnItemCountChange(ID, count);
forward OnItemConditionChange(ID, Float:condition);
forward OnItemEquippedChange(ID, Bool:equipped);
forward OnActorValueChange(ID, ActorValue:index, Float:value);
forward OnActorBaseValueChange(ID, ActorValue:index, Float:value);
forward OnActorSneak(ID, Bool:sneaking);
forward OnActorDeath(ID, killer, cause);
forward OnActorAttack(victim, attacker, weapon, damage);
forward OnPlayerDisconnect(ID, reason);
forward OnPlayerRequestGame(ID);
forward OnPlayerChat(ID, message{});

forward OnClientAuthenticate(const name{}, const pwd{});
forward OnGameTimeChange(year, month, day, hour);
forward OnServerInit();
forward OnServerExit(Bool:error);

forward OnCharGen(ID);
