const Sex: {
	Male = 0,
	Female = 1,
}

const Race: {
	Argoninan = "Argonian",
	Breton = "Breton",
	DarkElf = "Dark Elf",
	HighElf = "High Elf",
	Imperial = "Imperial",
	Khajiit = "Khajiit",
	Nord = "Nord",
	Orc = "Orc",
	Redguard = "Redguard",
	WoodElf = "Wood Elf"
}

native Bool:Kick(ID);
native SetRespawnTime(interval);
native GetRespawnTime();
native SetSpawnCell(cell);
native GetSpawnCell();

native Bool:IsPlayer(ID);

native GetBaseName(ID, message);
native SetBaseName(ID);

native CharGen(ID);
