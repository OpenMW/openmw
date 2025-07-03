Physics Settings
################

.. omw-setting::
   :title: async num threads
   :type: int
   :range: ≥ 0
   :default: 1
   :location: :bdg-success:`Launcher > Settings > Gameplay`

   Number of threads spawned for physics updates (processing actor movement) in the background.
   A value of 0 means physics update runs in the main thread.
   Values > 1 require Bullet physics library compiled with multithreading support.
   If multithreading is unsupported, a warning is logged and value 1 is used instead.

.. omw-setting::
   :title: lineofsight keep inactive cache
   :type: int
   :range: ≥ -1
   :default: 0

   Duration (in frames) to keep line-of-sight request results cached.
   Line of sight determines if two actors can see each other, used by AI and scripts.
   0 means cache only for current frame (multiple requests in same frame hit cache).
   Values > 0 keep cache warm for given frame count even without repeated requests.
   If async num threads is 0, this setting is forced to 0.
   If Bullet is compiled without multithreading support, uncached requests block async thread, hurting performance.
   If Bullet has multithreading, requests are non-blocking, so setting this to 0 is preferable.
