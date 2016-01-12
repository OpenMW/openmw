

public OnServerInit()
{

}

public OnServerExit(Bool:error)
{

}

public OnGameTimeChange(year, month, day, hour, minute)
{

}

public OnClientAuthenticate(const name{}, const pwd{})
{
	return true;
}


public OnPlayerDisconnect(ID, reason)
{

}


public OnPlayerRequestGame(ID)
{
	//return 0x00000000;
}

public OnPlayerChat(ID, message{})
{
	return 1;
}

public OnSpawn(ID)
{
	if (IsPlayer(ID))
	{
		new message {MAX_MESSAGE_LENGTH};
		GetBaseName(ID, message);

		strformat(message, sizeof(message), true, "Hello, %s!", message);
		UIMessage(ID, message);
	}
}

public OnActivate(ID, actor)
{

}

public OnCellChange(ID, cell)
{

}

blic OnItemCountChange(ID, count)
{

}

public OnItemConditionChange(ID, Float:condition)
{

}

public OnItemEquippedChange(ID, Bool:equipped)
{

}

public OnItemPickup(ID, actor)
{
	return 1;
}


public OnActorSneak(ID, Bool:sneaking)
{

}

public OnActorDeath(ID, killer, Death:cause)
{

}

public OnActorAttack(victim, attacker, weapon, damage)
{

}
