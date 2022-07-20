-------------------------------------------------------------------------------
-- Coroutine Manipulation.
-- The operations related to coroutines comprise a sub-library of the basic library 
-- and come inside the table coroutine.
-- See http://www.lua.org/manual/5.1/manual.html#2.11 for a general description of coroutines.
-- @module coroutine



-------------------------------------------------------------------------------
-- Creates a new coroutine, with body `f`. `f` must be a Lua
-- function. Returns this new coroutine, an object with type `"thread"`.
-- @function [parent=#coroutine] create
-- @param f a function used as coroutine body.  
-- @return #thread a new coroutine.

-------------------------------------------------------------------------------
-- Starts or continues the execution of coroutine `co`. The first time
-- you resume a coroutine, it starts running its body. The values `val1`,
-- ... are passed as the arguments to the body function. If the coroutine
-- has yielded, `resume` restarts it; the values `val1`, ... are passed
-- as the results from the yield.
-- 
-- If the coroutine runs without any errors, `resume` returns true plus any
-- values passed to `yield` (if the coroutine yields) or any values returned
-- by the body function (if the coroutine terminates). If there is any error,
-- `resume` returns false plus the error message.
-- @function [parent=#coroutine] resume
-- @param #thread co coroutine to start or resume.
-- @param ... arguments passed to the body function or as result of yield call.
-- @return #boolean true plus any values passed to `yield` (if the coroutine yields) or any values returned
-- by the body function (if the coroutine terminates)
-- @return #boolean false plus an error message.

-------------------------------------------------------------------------------
-- Returns the running coroutine, or nil when called by the main thread.
-- @function [parent=#coroutine] running
-- @return #thread the running coroutine, or nil when called by the main thread.

-------------------------------------------------------------------------------
-- Returns the status of coroutine `co`, as a string: `"running"`, if
-- the coroutine is running (that is, it called `status`); `"suspended"`, if
-- the coroutine is suspended in a call to `yield`, or if it has not started
-- running yet; `"normal"` if the coroutine is active but not running (that
-- is, it has resumed another coroutine); and `"dead"` if the coroutine has
-- finished its body function, or if it has stopped with an error.
-- @function [parent=#coroutine] status
-- @param #thread co a coroutine
-- @return #string the status : `"running"`, `"suspended"`, `"normal"` or `"dead"`.

-------------------------------------------------------------------------------
-- Creates a new coroutine, with body `f`. `f` must be a Lua
-- function. Returns a function that resumes the coroutine each time it is
-- called. Any arguments passed to the function behave as the extra arguments to
-- `resume`. Returns the same values returned by `resume`, except the first
-- boolean. In case of error, propagates the error.
-- @function [parent=#coroutine] wrap
-- @param f a function used as coroutine body. 
-- @param ... arguments passed to the body function or as result of yield call.
-- @return Any values passed to `yield` (if the coroutine yields) or any values returned
-- by the body function (if the coroutine terminates).

-------------------------------------------------------------------------------
-- Suspends the execution of the calling coroutine. The coroutine cannot
-- be running a C function, a metamethod, or an iterator. Any arguments to
-- `yield` are passed as extra results to `resume`.
-- @function [parent=#coroutine] yield
-- @param ... arguments passed as extra results to `resume` function.
-- @return Any values passed to the `resume` function.

return nil
