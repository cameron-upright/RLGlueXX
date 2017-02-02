#ifndef RL_GLUE_AGENT_SERVER_H
#define RL_GLUE_AGENT_SERVER_H

#include "RLGlue++.h"

namespace RLGlue {

  class AgentServerConnection : public boost::enable_shared_from_this<AgentServerConnection> {

  public:
    typedef boost::shared_ptr<AgentServerConnection> pointer;

    static pointer create(boost::asio::io_service& io_service, Agent &agent) {
      return pointer(new AgentServerConnection(io_service, agent));
    }

    boost::asio::ip::tcp::socket& socket() {
      return socket_;
    }

    void start() {
      readCommand();
    }

    ~AgentServerConnection() {}

  private:
    AgentServerConnection(boost::asio::io_service& io_service, Agent &agent)
      : agent_(agent), socket_(io_service) {}

    void readCommand() {

      headerReadBuffer_.resize(1);

      boost::asio::async_read(socket_, boost::asio::buffer(headerReadBuffer_),
                              boost::bind(&AgentServerConnection::handleReadCommand, shared_from_this(),
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));

    }


    // TODO : Refactor to an asyncReadCommand function
    void handleReadCommand(const boost::system::error_code& error, size_t num_bytes) {

      if (error)
        return;

      bodyReadBuffer_.resize(headerReadBuffer_[0]);

      boost::asio::async_read(socket_, boost::asio::buffer(bodyReadBuffer_),
                              boost::bind(&AgentServerConnection::handleReadCommandBody, shared_from_this(),
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));

    }



    void handleReadCommandBody(const boost::system::error_code& error, size_t num_bytes) {

      if (error)
        return;

      std::string cmdBodyStr(bodyReadBuffer_.begin(), bodyReadBuffer_.end());

      AgentCommand cmd;


      cmd.ParseFromString(cmdBodyStr);


      switch (cmd.type()) {


      case AgentCommand_Type_AGENT_INIT:

        // Init and then wait for the next command
        agent_.init();

        readCommand();

        break;


      case AgentCommand_Type_AGENT_START:

        {

          // Get the state
          const StateDesc &state = cmd.startcommand().state();

          // TODO : Fix this
          std::shared_ptr<::google::protobuf::Message> actionDesc(new ActionDesc(agent_.start(state)));

          // Start a new episode, and send the action back
          asyncWriteMessage(socket_, actionDesc,
                            boost::bind(&AgentServerConnection::handleWriteResponse, shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));

          break;
        }

      case AgentCommand_Type_AGENT_STEP:

        {

          // Get the reward state
          const RewardState &rewardState = cmd.stepcommand().rewardstate();

          // TODO : Fix this
          std::shared_ptr<::google::protobuf::Message> actionDesc(new ActionDesc(agent_.step(rewardState)));

          // Step the agent, and send the resulting RewardStateTerminal message
          asyncWriteMessage(socket_, actionDesc,
                            boost::bind(&AgentServerConnection::handleWriteResponse, shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));

          break;
        }

      case AgentCommand_Type_AGENT_END:

        {

          // Get the reward
          const float &reward = cmd.endcommand().reward();

          // End and then wait for the next command
          agent_.end(reward);

          readCommand();

          break;
        }


      case AgentCommand_Type_AGENT_CLEANUP:

        // Cleanup the agent, and then do nothing (ending the ASIO run loop)
        agent_.cleanup();

        break;


      default:
        break;

      }

    }



    void handleWriteResponse(const boost::system::error_code& error, size_t num_bytes) {

      if (error)
        return;

      readCommand();

    }







    Agent &agent_;

    boost::asio::ip::tcp::socket socket_;

    std::vector<size_t> headerReadBuffer_;
    std::vector<char> bodyReadBuffer_;

  };






  class AgentServer {
  public:
    AgentServer(boost::asio::io_service& io_service, Agent &agent)
      : acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 1338)),
        agent_(agent) {
      start_accept();
    }

  private:
    void start_accept() {
      AgentServerConnection::pointer new_connection =
        AgentServerConnection::create(acceptor_.get_io_service(), agent_);

      acceptor_.async_accept(new_connection->socket(),
                             boost::bind(&AgentServer::handle_accept, this, new_connection,
                                         boost::asio::placeholders::error));
    }

    void handle_accept(AgentServerConnection::pointer new_connection,
                       const boost::system::error_code& error) {
      if (!error)
        new_connection->start();
    }

    boost::asio::ip::tcp::acceptor acceptor_;

    Agent &agent_;

  };


}

#endif // RL_GLUE_AGENT_SERVER_H
