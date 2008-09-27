module bullet.bullet;

import bullet.bindings;

void initBullet()
{
  if(cpp_initBullet())
    throw new Exception("Bullet setup failed");
}

void cleanupBullet() { cpp_cleanupBullet(); }
