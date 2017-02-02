#include <glog/logging.h>

#include "Experiment.h"


namespace RLGlue {

  bool Experiment::runEpisode(const int stepLimit) {

    StateDesc state = envClient_.start();
    ActionDesc action = agentClient_.start(state);

    // Step the episode
    for (int i=0; i<stepLimit; i++) {

      // Step the environment
      RewardStateTerminal rewardStateTerminal = envClient_.step(action);

      // Create the RewardState
      RewardState rewardState;

      rewardState.set_reward(rewardStateTerminal.reward());
      *(rewardState.mutable_state()) = rewardStateTerminal.state();

      // End the episode on a terminal state, or at the step limit
      if (rewardStateTerminal.terminal()) {
        agentClient_.end(rewardStateTerminal.reward());
        return true;
      }

      // Step the agent
      action = agentClient_.step(rewardState);

    }

    return false;

  }

}

