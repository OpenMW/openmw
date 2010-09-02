#include "sndmanager.hpp"

#include "../misc/list.hpp"
#include <boost/weak_ptr.hpp>

using namespace OEngine::Sound;
using namespace Mangle::Sound;

/** This is our own internal implementation of the
    Mangle::Sound::Sound interface. This class links a SoundPtr to
    itself and prevents itself from being deleted as long as the sound
    is playing.
 */
struct OEngine::Sound::ManagedSound : SoundFilter
{
private:
  /** Who's your daddy? This is set if and only if we are listed
      internally in the given SoundManager.

      It may be NULL if the manager has been deleted but the user
      keeps their own SoundPtrs to the object.
  */
  SoundManager *mgr;

  /** Keep a weak pointer to ourselves, which we convert into a
      'strong' pointer when we are playing. When 'self' is pointing to
      ourselves, the object will never be deleted.

      This is used to make sure the sound is not deleted while
      playing, unless it is explicitly ordered to do so by the
      manager.

      TODO: This kind of construct is useful. If we need it elsewhere
      later, template it. It would be generally useful in any system
      where we poll to check if a resource is still needed, but where
      manual references are allowed.
  */
  WSoundPtr weak;
  SoundPtr self;

  // Keep this object from being deleted
  void lock()
  {
    self = SoundPtr(weak);
  }

  // Release the lock. This may or may not delete the object. Never do
  // anything after calling unlock()!
  void unlock()
  {
    self.reset();
  }

public:
  // Used for putting ourselves in linked lists
  ManagedSound *next, *prev;

  /** Detach this sound from its manager. This means that the manager
      will no longer know we exist. Typically only called when either
      the sound or the manager is about to get deleted.

      Since this means update() will no longer be called, we also have
      to unlock the sound manually since it will no longer be able to
      do that itself. This means that the sound may be deleted, even
      if it is still playing, when the manager is deleted.

      However, you are still allowed to keep and manage your own
      SoundPtr references, but the lock/unlock system is disabled
      after the manager is gone.
  */
  void detach()
  {
    if(mgr)
      {
        mgr->detach(this);
        mgr = NULL;
      }

    // Unlock must be last command. Object may get deleted at this
    // point.
    unlock();
  }

  ManagedSound(SoundPtr snd, SoundManager *mg)
    : SoundFilter(snd), mgr(mg)
  {}
  ~ManagedSound() { detach(); }

  // Needed to set up the weak pointer
  void setup(SoundPtr self)
  {
    weak = WSoundPtr(self);
  }

  // Override play() to mark the object as locked
  void play()
  {
    SoundFilter::play();

    // Lock the object so that it is not deleted while playing. Only
    // do this if we have a manager, otherwise the object will never
    // get unlocked.
    if(mgr) lock();
  }

  // Called regularly by the manager
  void update()
  {
    // If we're no longer playing, don't force object retention.
    if(!isPlaying())
      unlock();

    // unlock() may delete the object, so don't do anything below this
    // point.
  }

  SoundPtr clone()
  {
    // Cloning only works when we have a manager.
    assert(mgr);
    return mgr->wrap(client->clone());
  }
};

struct SoundManager::SoundManagerList
{
private:
  // A linked list of ManagedSound objects.
  typedef Misc::List<ManagedSound> SoundList;
  SoundList list;

public:
  // Add a new sound to the list
  void addNew(ManagedSound* snd)
  {
    list.insert(snd);
  }

  // Remove a sound from the list
  void remove(ManagedSound *snd)
  {
    list.remove(snd);
  }

  // Number of sounds in the list
  int numSounds() { return list.getNum(); }

  // Update all sounds
  void updateAll()
  {
    ManagedSound *s = list.getHead();
    while(s)
      {
        ManagedSound *cur = s;
        // Propagate first, since update() may delete object
        s = s->next;
        cur->update();
      }
  }

  // Detach and unlock all sounds
  void detachAll()
  {
    ManagedSound *s = list.getHead();
    while(s)
      {
        ManagedSound *cur = s;
        s = s->next;
        cur->detach();
      }
  }
};

SoundManager::SoundManager(SoundFactoryPtr fact)
  : FactoryFilter(fact)
{
  needsUpdate = true;
  list = new SoundManagerList;
}

SoundManager::~SoundManager()
{
  // Detach all sounds
  list->detachAll();
}

SoundPtr SoundManager::wrap(SoundPtr client)
{
  // Create and set up the sound wrapper
  ManagedSound *snd = new ManagedSound(client,this);
  SoundPtr ptr(snd);
  snd->setup(ptr);

  // Add ourselves to the list of all sounds
  list->addNew(snd);

  return ptr;
}

// Remove the sound from this manager.
void SoundManager::detach(ManagedSound *sound)
{
  list->remove(sound);
}

int SoundManager::numSounds()
{
  return list->numSounds();
}

void SoundManager::update()
{
  // Update all the sounds we own
  list->updateAll();

  // Update the source if it needs it
  if(client->needsUpdate)
    client->update();
}
