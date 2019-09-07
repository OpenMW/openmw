#include "luatimer.hpp"

#include <algorithm>

#include <components/debug/debuglog.hpp>

#include "luamanager.hpp"

#include "luautil.hpp"

namespace mwse {
	namespace lua {
		//
		// TimeComparer
		//

		bool TimerComparer::operator()(const std::shared_ptr<Timer>& first, double second) {
			return first->timing < second;
		}

		bool TimerComparer::operator()(const std::shared_ptr<Timer>& first, const std::shared_ptr<Timer>& second) {
			return first->timing < second->timing;
		}

		// Single instance to the comparator used for std::upper_bound.
		TimerComparer comparer;

		//
		// TimerController
		//

		TimerController::TimerController() :
			m_Clock(0.0)
		{

		}

		TimerController::TimerController(double initialClock) :
			m_Clock(initialClock)
		{

		}

		void TimerController::setClock(double clock) {
			m_Clock = clock;
			update();
		}

		void TimerController::incrementClock(double delta) {
			m_Clock += delta;
			update();
		}

		double TimerController::getClock() {
			return m_Clock;
		}

		std::shared_ptr<Timer> TimerController::createTimer(double duration, sol::protected_function callback, int iterations) {
			// Validate parameters.
			if (callback == sol::nil) {
				throw std::invalid_argument("Could not create timer: Callback function is nil.");
			}
			else if (duration <= 0.0 || callback.get_type() != sol::type::function) {
				return nullptr;
			}
			else if (iterations < 0) {
				iterations = 0;
			}

			// Setup the timer structure.
			auto timer = std::make_shared<Timer>();
			timer->controller = this;
			timer->duration = duration;
			timer->timing = m_Clock + duration;
			timer->iterations = iterations;
			timer->callback = callback;

			// Find the position in the list to add this timer, and add it.
			insertActiveTimer(timer);

			return timer;
		}

		bool TimerController::pauseTimer(std::shared_ptr<Timer> timer) {
			// Validate timer.
			if (timer->state != TimerState::Active) {
				return false;
			}

			// Remove from the active timer list.
			auto result = std::find(m_ActiveTimers.begin(), m_ActiveTimers.end(), timer);
			if (result == m_ActiveTimers.end()) {
				return false;
			}
			m_ActiveTimers.erase(result);

			// And add it to the paused list.
			m_PausedTimers.insert(timer);

			// Update its state.
			timer->state = TimerState::Paused;
			timer->timing = timer->timing - m_Clock;

			return true;
		}

		bool TimerController::resumeTimer(std::shared_ptr<Timer> timer) {
			// Validate timer.
			if (timer->state != TimerState::Paused) {
				return false;
			}

			// Remove from the paused timer list.
			m_PausedTimers.erase(timer);

			// Add to the active list.
			insertActiveTimer(timer);

			return true;
		}

		bool TimerController::resetTimer(std::shared_ptr<Timer> timer) {
			// Validate timer.
			if (timer->state != TimerState::Active) {
				return false;
			}

			// Change timing.
			timer->timing = m_Clock + timer->duration;

			// Move it to the right place in the list.
			repositionTimer(timer);

			return true;
		}

		bool TimerController::cancelTimer(std::shared_ptr<Timer> timer) {
			TimerState previousState = timer->state;
			timer->state = TimerState::Expired;

			// Remove from the active list.
			if (previousState == TimerState::Active) {
				auto position = std::find(m_ActiveTimers.begin(), m_ActiveTimers.end(), timer);
				if (position == m_ActiveTimers.end()) {
					return false;
				}
				m_ActiveTimers.erase(position);
				return true;
			}

			// Remove from the paused timer list.
			else if (previousState == TimerState::Paused) {
				return m_PausedTimers.erase(timer) == 1;
			}

			return false;
		}

		void TimerController::clearTimers() {
			// Mark all timers as expired.
			for (auto itt = m_ActiveTimers.begin(); itt != m_ActiveTimers.end(); itt++) {
				(*itt)->state = TimerState::Expired;
			}

			// Free the timers from internal storage.
			m_ActiveTimers.clear();
			m_PausedTimers.clear();
		}

		void TimerController::update() {
			// Keep looking at the front timer until it hasn't expired.
			std::shared_ptr<Timer> timer = nullptr;
			while (!m_ActiveTimers.empty() && (timer = m_ActiveTimers.front()) && timer->timing <= m_Clock) {
				// Build data to send to the callback.
				sol::table data = LuaManager::getInstance().getThreadSafeStateHandle().state.create_table();

				data["timer"] = timer;

				// Invoke the callback.
				sol::protected_function callback = timer->callback;
				sol::protected_function_result result = callback(data);
				if (!result.valid()) {
					sol::error error = result;
					Log(Debug::Error) << "Lua error encountered in timer callback:\n" << error.what();

					// Cancel the timer.
					cancelTimer(timer);
					continue;
				}

				// Decrement iterations if the timer uses them.
				if (timer->iterations > 0) {
					timer->iterations--;

					// If we just hit 0 left, cancel the timer.
					if (timer->iterations == 0) {
						cancelTimer(timer);
						continue;
					}
				}

				// Update timer and reposition it in the vector.
				timer->timing += timer->duration;
				repositionTimer(timer);
			}
		}

		std::vector<std::shared_ptr<Timer>>::iterator TimerController::insertActiveTimer(std::shared_ptr<Timer> timer) {
			auto position = std::upper_bound(m_ActiveTimers.begin(), m_ActiveTimers.end(), timer, comparer);
			return m_ActiveTimers.insert(position, timer);
		}

		void TimerController::repositionTimer(std::shared_ptr<Timer> timer) {
			// Remove from current position.
			auto position = std::find(m_ActiveTimers.begin(), m_ActiveTimers.end(), timer);
			if (position == m_ActiveTimers.end()) {
				return;
			}
			m_ActiveTimers.erase(position);

			// Then insert it back in.
			insertActiveTimer(timer);
		}

		//
		// Legacy functions, to help people migrate their code to the new method of performing timers.
		//

		// Create and return a timer given a controller and a table of parameters.
		std::shared_ptr<Timer> startTimer(TimerController* controller, sol::table params) {
			// Get the timer variables.
			double duration = getOptionalParam<double>(params, "duration", 0.0);
			sol::function callback = getOptionalParam<sol::function>(params, "callback", sol::nil);
			int iterations = getOptionalParam<int>(params, "iterations", 1);

			// Allow infinite repeat.
			if (iterations <= 0) {
				iterations = 0;
			}

			// Create the timer.
			return controller->createTimer(duration, callback, iterations);
		}

		// Create a timer, as above, but get the controller from params.type.
		std::shared_ptr<Timer> startTimerAmbiguous(sol::table params) {
			// Get the timer controller we care about.
			unsigned int type = getOptionalParam<unsigned int>(params, "type", (unsigned int)int(TimerType::SimulationTime));
			std::shared_ptr<TimerController> controller = LuaManager::getInstance().getTimerController(static_cast<TimerType>(type));
			if (controller == nullptr) {
				return nullptr;
			}

			// Create the timer.
			return startTimer(controller.get(), params);
		}

		// Directly create a real-time timer.
		std::shared_ptr<Timer> startTimerLegacyRealWithIterations(double duration, sol::protected_function callback, int iterations) {
			std::shared_ptr<TimerController> controller = LuaManager::getInstance().getTimerController(TimerType::RealTime);
			if (controller == nullptr) {
				return nullptr;
			}

			return controller->createTimer(duration, callback, iterations);
		}
		std::shared_ptr<Timer> startTimerLegacyReal(double duration, sol::protected_function callback) {
			return startTimerLegacyRealWithIterations(duration, callback, 1);
		}

		// Directly create a simulation-time timer.
		std::shared_ptr<Timer> startTimerLegacySimulationWithIterations(double duration, sol::protected_function callback, int iterations) {
			std::shared_ptr<TimerController> controller = LuaManager::getInstance().getTimerController(TimerType::SimulationTime);
			if (controller == nullptr) {
				return nullptr;
			}

			return controller->createTimer(duration, callback, iterations);
		}
		std::shared_ptr<Timer> startTimerLegacySimulation(double duration, sol::protected_function callback) {
			return startTimerLegacySimulationWithIterations(duration, callback, 1);
		}

		// Function to pause a given timer.
		bool legacyTimerPause(std::shared_ptr<Timer> timer) {
			return timer->controller->pauseTimer(timer);
		}

		// Function to resume a given timer.
		bool legacyTimerResume(std::shared_ptr<Timer> timer) {
			return timer->controller->resumeTimer(timer);
		}

		// Function to reset a given timer.
		bool legacyTimerReset(std::shared_ptr<Timer> timer) {
			return timer->controller->resetTimer(timer);
		}

		// Function to cancel a given timer.
		bool legacyTimerCancel(std::shared_ptr<Timer> timer) {
			return timer->controller->cancelTimer(timer);
		}

		// Create a timer that will complete in the next cycle.
		std::shared_ptr<Timer> legacyTimerDelayOneFrame(sol::protected_function callback) {
			std::shared_ptr<TimerController> controller = LuaManager::getInstance().getTimerController(TimerType::RealTime);
			if (controller == nullptr) {
				return nullptr;
			}
			return controller->createTimer(0.0000001, callback, 1);
		}

		// Create a timer that will complete in the next cycle, defaulting to simulation time.
		std::shared_ptr<Timer> legacyTimerDelayOneFrameSpecified(sol::protected_function callback, sol::optional<int> type) {
			std::shared_ptr<TimerController> controller = LuaManager::getInstance().getTimerController(static_cast<TimerType>(type.value_or((int)TimerType::SimulationTime)));
			if (controller == nullptr) {
				return nullptr;
			}
			return controller->createTimer(0.0000001, callback, 1);
		}

		//
		// Lua binding for new data types and functions.
		//

		void bindLuaTimer() {
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
				usertypeDefinition.set("create", [](TimerController& self, sol::table params) {
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
				usertypeDefinition.set("duration", sol::readonly_property(&Timer::duration));
				usertypeDefinition.set("iterations", sol::readonly_property(&Timer::iterations));
				usertypeDefinition.set("state", sol::readonly_property(&Timer::state));
				usertypeDefinition.set("timing", sol::readonly_property(&Timer::timing));
				usertypeDefinition.set("callback", sol::readonly_property(&Timer::callback));
				usertypeDefinition.set("timeLeft", sol::readonly_property([](Timer& self) -> sol::optional<double> {
					if (self.state == TimerState::Active) {
						return self.timing - self.controller->getClock();
					}
					else if (self.state == TimerState::Paused) {
						return self.timing;
					}

					return sol::optional<double>();
				}));

				// Legacy value binding.
				usertypeDefinition.set("t", &Timer::duration);
				usertypeDefinition.set("c", &Timer::callback);
				usertypeDefinition.set("i", &Timer::iterations);
				usertypeDefinition.set("f", &Timer::timing);

				// Allow creating timers.
				usertypeDefinition.set("pause", [](std::shared_ptr<Timer> self) {
					return self->controller->pauseTimer(self);
				});
				usertypeDefinition.set("resume", [](std::shared_ptr<Timer> self) {
					return self->controller->resumeTimer(self);
				});
				usertypeDefinition.set("reset", [](std::shared_ptr<Timer> self) {
					return self->controller->resetTimer(self);
				});
				usertypeDefinition.set("cancel", [](std::shared_ptr<Timer> self) {
					return self->controller->cancelTimer(self);
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
			state["timer"]["createController"] = []() {
				return std::make_shared<TimerController>();
			};
		}
	}
}
