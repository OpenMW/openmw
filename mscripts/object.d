module mscripts.object;

import monster.monster;
import std.stdio;
import std.date;

// Set up the base Monster classes we need in OpenMW
void initMonsterScripts()
{
  // Add the script directory
  MonsterClass.addPath("mscripts/");

  // Make sure the Object class is loaded
  auto mc = new MonsterClass("Object", "object.mn");

  // Bind various functions
  mc.bind("print", { print(); });
  mc.bind("sleep", new IdleSleep);

  // Load and run the test script
  mc = new MonsterClass("Test");
  mc.createObject().call("test");
}

// Write a message to screen
void print()
{
  AIndex[] args = stack.popAArray();

  foreach(AIndex ind; args)
    writef("%s ", arrays.getRef(ind).carr);
  writefln();
}

// Sleep a given amount of time. Currently uses the system clock, but
// will later be optimized to use the already existing timing
// information from OGRE.
class IdleSleep : IdleFunction
{
  long getLong(MonsterObject *mo)
    { return *(cast(long*)mo.extra); }
  void setLong(MonsterObject *mo, long l)
    { *(cast(long*)mo.extra) = l; }

 override:
  bool initiate(MonsterObject *mo)
    {
      // Get the parameter
      double secs = stack.popFloat;

      // Get current time
      long newTime = getUTCtime();

      // Calculate when we should return
      newTime += secs*TicksPerSecond;

      // Store it
      if(mo.extra == null) mo.extra = new long;
      setLong(mo, newTime);

      // Schedule us
      return true;
    }

  bool hasFinished(MonsterObject *mo)
    {
      // Is it time?
      return getUTCtime() >= getLong(mo);
    }
}
