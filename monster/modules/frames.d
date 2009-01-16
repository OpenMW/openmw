
// Provides some simple numbers and functions regarding the rendering
// frames of the application. It's up to the user to some degree to
// provide this information, though. We rely on vm.frame to be called
// each frame.
module monster.modules.frames;

import monster.monster;
import monster.vm.mclass;
import monster.vm.idlefunction;
import monster.vm.thread;

const char[] moduleDef =
"module frames;

float time;      // Time since last frame
float totalTime; // Time since rendering started

ulong counter;   // Number of frames since program startup

// Sleep a given number of frames
idle sleep(int frameNum);
"; //"

// Keep local copies of these, since we don't want Monster code to
// overwrite them (we'll be able to explicitly forbid this later.)
ulong frames = 0;
float totTime = 0;

ulong *counter_ptr;
float *time_ptr;
float *totalTime_ptr;

// Add the given time and number of frames to the counters
void updateFrames(float time, int frmCount = 1)
{
  // Add up to the totals
  frames += frmCount;
  totTime += time;

  // Set the Monster variables
  *counter_ptr = frames;
  *time_ptr = time;
  *totalTime_ptr = totTime;

  // TODO: A similar priority queue like we're planning for timer
  // would also be applicable here. However I'm guessing frameSleep()
  // will be used a lot less than sleep() though, so this is really
  // not high up on the priority list.
}

// Idle function that sleeps a given number of frames before
// returning.
class IdleFrameSleep : IdleFunction
{
 override:
  bool initiate(Thread* cn)
    {
      // Calculate the return frame
      cn.idleData.l = frames + stack.popInt;

      // Schedule us
      return true;
    }

  bool hasFinished(Thread* cn)
    {
      // Are we at (or past) the correct frame?
      return frames >= cn.idleData.l;
    }
}

void initFramesModule()
{
  static MonsterClass mc;
  if(mc !is null) return;

  mc = new MonsterClass(MC.String, moduleDef, "frames");

  // Bind the idle
  mc.bind("sleep", new IdleFrameSleep);

  // Get pointers to the variables so we can write to them easily.
  auto mo = mc.getSing();
  counter_ptr   = mo.getUlongPtr("counter");
  time_ptr      = mo.getFloatPtr("time");
  totalTime_ptr = mo.getFloatPtr("totalTime");
}
