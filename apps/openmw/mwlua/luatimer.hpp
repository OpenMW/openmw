#pragma once

#include <list>
#include <unordered_set>

#include "sol.hpp"

namespace mwse {
	namespace lua {
		struct Timer;

		enum class TimerState {
			Active,
			Paused,
			Expired
		};

		// Comparing structure for use with std::upper_bound.
		struct TimerComparer {
			bool operator() (const std::shared_ptr<Timer>& first, double second);
			bool operator() (const std::shared_ptr<Timer>& first, const std::shared_ptr<Timer>& second);
		};

		// Manager for timers. Responsible for all timer-related logic.
		class TimerController {
		public:
			// Constructor with an initial clock of 0.
			TimerController();

			// Constructor with a defined initial clock.
			TimerController(double initialClock);
			
			// Set time to the new clock.
			void setClock(double clock);

			// Add delta to the current clock.
			void incrementClock(double delta);

			// Get the current clock time.
			double getClock();

			// Create a new timer with fixed data.
			std::shared_ptr<Timer> createTimer(double duration, sol::protected_function callback, int iterations = 1);

			// Move a timer from the active list to the inactive list, and mark it paused.
			bool pauseTimer(std::shared_ptr<Timer> timer);

			// Take a paused timer and move it back to the active timer list.
			bool resumeTimer(std::shared_ptr<Timer> timer);

			// Modify an active timer's completion time to now, plus its duration.
			bool resetTimer(std::shared_ptr<Timer> timer);

			// Cancel a timer, disconnecting it from this controller.
			bool cancelTimer(std::shared_ptr<Timer> timer);

			void clearTimers();

		private:
			// Runs through the active timer list. Triggers and expires/iterates completed timers.
			void update();

			// Places a timer into it the active timer list, based on its completion timing.
			std::vector<std::shared_ptr<Timer>>::iterator insertActiveTimer(std::shared_ptr<Timer> timer);

			// Removes an active timer, then inserts it back into the active timer list based on its timing.
			void repositionTimer(std::shared_ptr<Timer> timer);

			// The current internal clock to compare timers against.
			double m_Clock;

			// A list of active timers, sorted by their completion time.
			std::vector<std::shared_ptr<Timer>> m_ActiveTimers;

			// An unordered collection of paused timers.
			std::unordered_set<std::shared_ptr<Timer>> m_PausedTimers;
		};

		// A logicless structure containing timer data.
		struct Timer {
			// A handle back to the associated controller.
			TimerController * controller;

			// The current state of the timer.
			TimerState state;

			// How long the timer lasts.
			double duration;

			// Either the completion state, or the time left when it was paused.
			double timing;

			// The number of iterations it has left, or -1 if it runs forever.
			int iterations;

			// Callback for timer completion.
			sol::protected_function callback;
		};

		// Create all the necessary lua binding for the timer API and the above data types.
		void bindLuaTimer();
	}
}
