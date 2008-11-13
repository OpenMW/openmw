module scene.gamesettings;

import monster.monster;
import esm.records : gameSettings;
import esm.defs : VarType;
import std.stdio;
import std.string;

MonsterObject *gmstObj;

void loadGameSettings()
{
  // Load the GameSettings Monster class, and create an instance.
  MonsterClass mc = new MonsterClass("GameSettings");
  MonsterObject *mo = mc.createObject;
  gmstObj = mo;

  foreach(a, b; gameSettings.names)
    {
      assert(a == b.id);
      assert(a[0] == 'i' || a[0] == 'f' || a[0] == 's');

      // There's three cases of variable names containing
      // spaces. Since these are so seldom, it makes more sense to
      // make special workarounds for them instead of searching every
      // string.
      char[] name = a;
      if(name.length > 13 && (name[6] == ' ' || name[5] == ' '))
        {
          name = name.replace(" ", "_");
          // Make sure we don't change the original string!
          assert(name != a);
        }

      if(mc.sc.findVar(name) is null)
        {
          writefln("WARNING: GMST %s not supported!", name);
          continue;
        }

      if(b.type == VarType.Int) mo.setInt(name, b.i);
      else if(b.type == VarType.Float) mo.setFloat(name, b.f);
      // TODO: At some point we will probably translate strings into
      // UTF32 at load time, so string8 will not be needed here.
      else if(b.type == VarType.String) mo.setString8(name, b.str);
    }

  // Call the test function
  mo.call("test");
}
