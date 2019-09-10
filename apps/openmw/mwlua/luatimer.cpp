#include "luatimer.hpp"

#include <algorithm>

#include <components/debug/debuglog.hpp>

#include "luamanager.hpp"

#include "luautil.hpp"

namespace mwse
{
    namespace lua
    {
        bool TimerComparer::operator()(const std::shared_ptr<Timer>& first, double second)
        {
            return first->mTiming < second;
        }

        bool TimerComparer::operator()(const std::shared_ptr<Timer>& first, const std::shared_ptr<Timer>& second)
        {
            return first->mTiming < second->mTiming;
        }

        // Single instance to the comparator used for std::upper_bound.
        TimerComparer comparer;

        TimerController::TimerController() :
            mClock(0.0)
        {

        }

        TimerController::TimerController(double initialClock) :
            mClock(initialClock)
        {

        }

        void TimerController::setClock(double clock)
        {
            mClock = clock;
            update();
        }

        void TimerController::incrementClock(double delta)
        {
            mClock += delta;
            update();
        }

        double TimerController::getClock()
        {
            return mClock;
        }

        std::shared_ptr<Timer> TimerController::createTimer(double duration, sol::protected_function callback, int iterations)
        {
            // Validate parameters.
            if (callback == sol::nil)
            {
                throw std::invalid_argument("Could not create timer: Callback function is nil.");
            }
            else if (duration <= 0.0 || callback.get_type() != sol::type::function)
            {
                return nullptr;
            }
            else if (iterations < 0)
            {
                iterations = 0;
            }

            // Setup the timer structure.
            auto timer = std::make_shared<Timer>();
            timer->mController = this;
            timer->mDuration = duration;
            timer->mTiming = mClock + duration;
            timer->mIterations = iterations;
            timer->mCallback = callback;

            // Find the position in the list to add this timer, and add it.
            insertActiveTimer(timer);

            return timer;
        }

        bool TimerController::pauseTimer(std::shared_ptr<Timer> timer)
        {
            // Validate timer.
            if (timer->mState != TimerState::Active)
            {
                return false;
            }

            // Remove from the active timer list.
            auto result = std::find(mActiveTimers.begin(), mActiveTimers.end(), timer);
            if (result == mActiveTimers.end())
            {
                return false;
            }
            mActiveTimers.erase(result);

            // And add it to the paused list.
            mPausedTimers.insert(timer);

            // Update its state.
            timer->mState = TimerState::Paused;
            timer->mTiming = timer->mTiming - mClock;

            return true;
        }

        bool TimerController::resumeTimer(std::shared_ptr<Timer> timer)
        {
            // Validate timer.
            if (timer->mState != TimerState::Paused)
            {
                return false;
            }

            // Remove from the paused timer list.
            mPausedTimers.erase(timer);

            // Add to the active list.
            insertActiveTimer(timer);

            return true;
        }

        bool TimerController::resetTimer(std::shared_ptr<Timer> timer)
        {
            // Validate timer.
            if (timer->mState != TimerState::Active)
            {
                return false;
            }

            // Change timing.
            timer->mTiming = mClock + timer->mDuration;

            // Move it to the right place in the list.
            repositionTimer(timer);

            return true;
        }

        bool TimerController::cancelTimer(std::shared_ptr<Timer> timer)
        {
            TimerState previousState = timer->mState;
            timer->mState = TimerState::Expired;

            // Remove from the active list.
            if (previousState == TimerState::Active)
            {
                auto position = std::find(mActiveTimers.begin(), mActiveTimers.end(), timer);
                if (position == mActiveTimers.end())
                {
                    return false;
                }
                mActiveTimers.erase(position);
                return true;
            }

            // Remove from the paused timer list.
            else if (previousState == TimerState::Paused)
            {
                return mPausedTimers.erase(timer) == 1;
            }

            return false;
        }

        void TimerController::clearTimers()
        {
            // Mark all timers as expired.
            for (auto itt = mActiveTimers.begin(); itt != mActiveTimers.end(); itt++)
            {
                (*itt)->mState = TimerState::Expired;
            }

            // Free the timers from internal storage.
            mActiveTimers.clear();
            mPausedTimers.clear();
        }

        void TimerController::update()
        {
            // Keep looking at the front timer until it hasn't expired.
            std::shared_ptr<Timer> timer = nullptr;
            while (!mActiveTimers.empty() && (timer = mActiveTimers.front()) && timer->mTiming <= mClock)
            {
                // Build data to send to the callback.
                sol::table data = LuaManager::getInstance().getThreadSafeStateHandle().state.create_table();

                data["timer"] = timer;

                // Invoke the callback.
                sol::protected_function callback = timer->mCallback;
                sol::protected_function_result result = callback(data);
                if (!result.valid())
                {
                    sol::error error = result;
                    Log(Debug::Error) << "Lua error encountered in timer callback:\n" << error.what();

                    // Cancel the timer.
                    cancelTimer(timer);
                    continue;
                }

                // Decrement iterations if the timer uses them.
                if (timer->mIterations > 0)
                {
                    timer->mIterations--;

                    // If we just hit 0 left, cancel the timer.
                    if (timer->mIterations == 0)
                    {
                        cancelTimer(timer);
                        continue;
                    }
                }

                // Update timer and reposition it in the vector.
                timer->mTiming += timer->mDuration;
                repositionTimer(timer);
            }
        }

        std::vector<std::shared_ptr<Timer>>::iterator TimerController::insertActiveTimer(std::shared_ptr<Timer> timer)
        {
            auto position = std::upper_bound(mActiveTimers.begin(), mActiveTimers.end(), timer, comparer);
            return mActiveTimers.insert(position, timer);
        }

        void TimerController::repositionTimer(std::shared_ptr<Timer> timer)
        {
            // Remove from current position.
            auto position = std::find(mActiveTimers.begin(), mActiveTimers.end(), timer);
            if (position == mActiveTimers.end())
            {
                return;
            }
            mActiveTimers.erase(position);

            // Then insert it back in.
            insertActiveTimer(timer);
        }

        //
        // Legacy functions, to help people migrate their code to the new method of performing timers.
        //

        // Create and return a timer given a controller and a table of parameters.
        std::shared_ptr<Timer> startTimer(TimerController* controller, sol::table params)
        {
            // Get the timer variables.
            double duration = getOptionalParam<double>(params, "duration", 0.0);
            sol::function callback = getOptionalParam<sol::function>(params, "callback", sol::nil);
            int iterations = getOptionalParam<int>(params, "iterations", 1);

            // Allow infinite repeat.
            if (iterations <= 0)
            {
                iterations = 0;
            }

            // Create the timer.
            return controller->createTimer(duration, callback, iterations);
        }

        // Create a timer, as above, but get the controller from params.type.
        std::shared_ptr<Timer> startTimerAmbiguous(sol::table params)
        {
            // Get the timer controller we care about.
            unsigned int type = getOptionalParam<unsigned int>(params, "type", (unsigned int)int(TimerType::SimulationTime));
            std::shared_ptr<TimerController> controller = LuaManager::getInstance().getTimerController(static_cast<TimerType>(type));
            if (controller == nullptr)
            {
                return nullptr;
            }

            // Create the timer.
            return startTimer(controller.get(), params);
        }

        // Directly create a real-time timer.
        std::shared_ptr<Timer> startTimerLegacyRealWithIterations(double duration, sol::protected_function callback, int iterations)
        {
            std::shared_ptr<TimerController> controller = LuaManager::getInstance().getTimerController(TimerType::RealTime);
            if (controller == nullptr)
            {
                return nullptr;
            }

            return controller->createTimer(duration, callback, iterations);
        }
        std::shared_ptr<Timer> startTimerLegacyReal(double duration, sol::protected_function callback)
        {
            return startTimerLegacyRealWithIterations(duration, callback, 1);
        }

        // Directly create a simulation-time timer.
        std::shared_ptr<Timer> startTimerLegacySimulationWithIterations(double duration, sol::protected_function callback, int iterations)
        {
            std::shared_ptr<TimerController> controller = LuaManager::getInstance().getTimerController(TimerType::SimulationTime);
            if (controller == nullptr)
            {
                return nullptr;
            }

            return controller->createTimer(duration, callback, iterations);
        }
        std::shared_ptr<Timer> startTimerLegacySimulation(double duration, sol::protected_function callback)
        {
            return startTimerLegacySimulationWithIterations(duration, callback, 1);
        }

        // Function to pause a given timer.
        bool legacyTimerPause(std::shared_ptr<Timer> timer)
        {
            return timer->mController->pauseTimer(timer);
        }

        // Function to resume a given timer.
        bool legacyTimerResume(std::shared_ptr<Timer> timer)
        {
            return timer->mController->resumeTimer(timer);
        }

        // Function to reset a given timer.
        bool legacyTimerReset(std::shared_ptr<Timer> timer)
        {
            return timer->mController->resetTimer(timer);
        }

        // Function to cancel a given timer.
        bool legacyTimerCancel(std::shared_ptr<Timer> timer)
        {
            return timer->mController->cancelTimer(timer);
        }

        // Create a timer that will complete in the next cycle.
        std::shared_ptr<Timer> legacyTimerDelayOneFrame(sol::protected_function callback)
        {
            std::shared_ptr<TimerController> controller = LuaManager::getInstance().getTimerController(TimerType::RealTime);
            if (controller == nullptr)
            {
                return nullptr;
            }
            return controller->createTimer(0.0000001, callback, 1);
        }

        // Create a timer that will complete in the next cycle, defaulting to simulation time.
        std::shared_ptr<Timer> legacyTimerDelayOneFrameSpecified(sol::protected_function callback, sol::optional<int> type)
        {
            std::shared_ptr<TimerController> controller = LuaManager::getInstance().getTimerController(static_cast<TimerType>(type.value_or((int)TimerType::SimulationTime)));
            if (controller == nullptr)
            {
                return nullptr;
            }
            return controller->createTimer(0.0000001, callback, 1);
        }

        //
        // Lua binding for new data types and functions.
        //

        void bindLuaTimer()
        {
            // Get our lua state.
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            // Bind TimerController.
            {
                // Start our usertype. We must finish this with state.set_usertype.
                auto usertypeDefinition = state.create_simple_usertype<TimerController>();
                usertypeDefinition.set("new", sol::constructors<TimerController(), TimerController(double)>());

                // Basic property binding.
                usertypeDefinition.set("clock", sol::property(&TimerController::getClock, &TimerController::setClock));

                // Allow creating timers.
                usertypeDefinition.set("create", [](TimerController& self, sol::table params)
                {
                    return startTimer(&self, params);
                });

                // Finish up our usertype.
                state.set_usertype("timerController", usertypeDefinition);
            }

            // Bind Timer.
            {
                // Start our usertype. We must finish this with state.set_usertype.
                auto usertypeDefinition = state.create_simple_usertype<Timer>();
                usertypeDefinition.set("new", sol::no_constructor);

                // Basic property binding.
                usertypeDefinition.set("duration", sol::readonly_property(&Timer::mDuration));
                usertypeDefinition.set("iterations", sol::readonly_property(&Timer::mIterations));
                usertypeDefinition.set("state", sol::readonly_property(&Timer::mState));
                usertypeDefinition.set("timing", sol::readonly_property(&Timer::mTiming));
                usertypeDefinition.set("callback", sol::readonly_property(&Timer::mCallback));
                usertypeDefinition.set("timeLeft", sol::readonly_property([](Timer& self) -> sol::optional<double>
                {
                    if (self.mState == TimerState::Active)
                    {
                        return self.mTiming - self.mController->getClock();
                    }
                    else if (self.mState == TimerState::Paused)
                    {
                        return self.mTiming;
                    }

                    return sol::optional<double>();
                }));

                // Legacy value binding.
                usertypeDefinition.set("t", &Timer::mDuration);
                usertypeDefinition.set("c", &Timer::mCallback);
                usertypeDefinition.set("i", &Timer::mIterations);
                usertypeDefinition.set("f", &Timer::mTiming);

                // Allow creating timers.
                usertypeDefinition.set("pause", [](std::shared_ptr<Timer> self)
                {
                    return self->mController->pauseTimer(self);
                });
                usertypeDefinition.set("resume", [](std::shared_ptr<Timer> self)
                {
                    return self->mController->resumeTimer(self);
                });
                usertypeDefinition.set("reset", [](std::shared_ptr<Timer> self)
                {
                    return self->mController->resetTimer(self);
                });
                usertypeDefinition.set("cancel", [](std::shared_ptr<Timer> self)
                {
                    return self->mController->cancelTimer(self);
                });

                // Finish up our usertype.
                state.set_usertype("timer", usertypeDefinition);
            }

            // Create our timer library.
            state["timer"] = state.create_table();

            // Expose timer types.
            state["timer"]["real"] = TimerType::RealTime;
            state["timer"]["simulate"] = TimerType::SimulationTime;
            state["timer"]["game"] = TimerType::GameTime;

            // Expose timer states.
            state["timer"]["active"] = TimerState::Active;
            state["timer"]["paused"] = TimerState::Paused;
            state["timer"]["expired"] = TimerState::Expired;

            // Bind the legacy and new start functions.
            state["timer"]["start"] = sol::overload(&startTimerAmbiguous, &startTimerLegacySimulation, &startTimerLegacySimulationWithIterations);

            // Legacy support for frame timers.
            state["timer"]["frame"] = state.create_table();
            state["timer"]["frame"]["start"] = sol::overload(&startTimerLegacyReal, &startTimerLegacyRealWithIterations);

            // Legacy support for functions.
            state["timer"]["pause"] = &legacyTimerPause;
            state["timer"]["frame"]["pause"] = &legacyTimerPause;
            state["timer"]["resume"] = &legacyTimerResume;
            state["timer"]["frame"]["resume"] = &legacyTimerResume;
            state["timer"]["reset"] = &legacyTimerReset;
            state["timer"]["frame"]["reset"] = &legacyTimerReset;
            state["timer"]["cancel"] = &legacyTimerCancel;
            state["timer"]["frame"]["cancel"] = &legacyTimerCancel;
            state["timer"]["delayOneFrame"] = &legacyTimerDelayOneFrameSpecified;
            state["timer"]["frame"]["delayOneFrame"] = &legacyTimerDelayOneFrame;

            // Let new TimerControllers get made.
            state["timer"]["createController"] = []()
            {
                return std::make_shared<TimerController>();
            };
        }
    }
}
