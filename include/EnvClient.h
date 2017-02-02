#ifndef RL_GLUE_ENV_CLIENT_H
#define RL_GLUE_ENV_CLIENT_H

#include "RLGlue++.h"

namespace RLGlue {

  class EnvClient {
  public:
    EnvClient(boost::asio::io_service& io_service,
              const std::string &host,
              const std::string &service) :
      socket_(io_service) {

      boost::asio::ip::tcp::resolver resolver(io_service);
      boost::asio::ip::tcp::resolver::query query(host, service);
      boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

      LOG(INFO) << "Attempting connection to " << host << " " << service;

      boost::asio::connect(socket_, endpoint_iterator);

      LOG(INFO) << "Connected";

    }


    boost::asio::ip::tcp::socket& getSocket() {
      return socket_;
    }

    void init() {

      // Initialize the environment
      RLGlue::EnvironmentCommand initCmd;
      initCmd.set_type(RLGlue::EnvironmentCommand_Type_ENV_INIT);

      RLGlue::writeMessage(socket_, initCmd);

    }

    StateDesc start() {

      // Start the environment
      EnvironmentCommand startCmd;
      startCmd.set_type(EnvironmentCommand_Type_ENV_START);

      writeMessage(socket_, startCmd);

      return readMessage<StateDesc>(socket_);

    }

    RewardStateTerminal step(const RLGlue::ActionDesc &action) {
    
      // Write a step command
      EnvironmentCommand cmd;

      cmd.set_type(RLGlue::EnvironmentCommand_Type_ENV_STEP);
      cmd.mutable_stepcommand()->mutable_action();

      ActionDesc actionCopy(action);

      EnvironmentCommand_StepCommand *stepCmd = cmd.mutable_stepcommand();
      *(stepCmd->mutable_action()) = actionCopy;

      writeMessage(socket_, cmd);

      return readMessage<RewardStateTerminal>(socket_);


    }

    void cleanup() {

      // Cleanup the environment
      RLGlue::EnvironmentCommand cleanupCmd;
      cleanupCmd.set_type(RLGlue::EnvironmentCommand_Type_ENV_CLEANUP);

      RLGlue::writeMessage(socket_, cleanupCmd);

    }


    boost::asio::ip::tcp::socket socket_;

  };

}

#endif // RL_GLUE_ENV_CLIENT_H
