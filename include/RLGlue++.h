#ifndef RL_GLUE_XX_H
#define RL_GLUE_XX_H

#include <google/protobuf/message.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include <iostream>

#include "proto/RLGlue.pb.h"


using namespace std;


namespace RLGlue {

  void writeMessage(boost::asio::ip::tcp::socket &socket, const ::google::protobuf::Message &msg);

  template <class MessageType>
  MessageType readMessage(boost::asio::ip::tcp::socket &socket) {

    // Read the header
    std::vector<size_t> headerReadBuffer(1);
    boost::asio::read(socket, boost::asio::buffer(headerReadBuffer));

    // Read the body
    std::vector<char> bodyReadBuffer(headerReadBuffer[0]);
    boost::asio::read(socket, boost::asio::buffer(bodyReadBuffer));

    // Convert to string and then MessageType
    std::string msgData(bodyReadBuffer.begin(), bodyReadBuffer.end());

    MessageType msg;
    msg.ParseFromString(msgData);

    return msg;

  }


  template <typename WriteHandler>
  class AsyncWriteMessage :
    public boost::enable_shared_from_this<AsyncWriteMessage<WriteHandler> >,
    public boost::noncopyable {
 
  public:

    typedef boost::shared_ptr<AsyncWriteMessage<WriteHandler> > pointer;

    static pointer create(boost::asio::ip::tcp::socket &socket,
                          BOOST_ASIO_MOVE_ARG(WriteHandler) handler) {

      return pointer(new AsyncWriteMessage<WriteHandler>(socket, std::move(handler)));

    }


    void start(std::shared_ptr<::google::protobuf::Message> &msg) {

      // Serialize the response
      std::shared_ptr<std::string> messageWriteBuffer(new std::string());
      msg->SerializeToString(messageWriteBuffer.get());

      // Setup the header (size)
      std::shared_ptr<std::vector<size_t> > headerWriteBuffer(new std::vector<size_t>({messageWriteBuffer->size()}));

      // Write the header size first
      boost::asio::async_write(socket_, boost::asio::buffer(*headerWriteBuffer),
                               boost::bind(&AsyncWriteMessage<WriteHandler>::handleWriteResponseHeader, this->shared_from_this(),
                                           messageWriteBuffer,
                                           headerWriteBuffer,
                                           boost::asio::placeholders::error,
                                           boost::asio::placeholders::bytes_transferred));

    }

    ~AsyncWriteMessage() {}

  private:

    // Serializes the message, then starts an async write
    AsyncWriteMessage(boost::asio::ip::tcp::socket &socket,
                      BOOST_ASIO_MOVE_ARG(WriteHandler) handler) :
      socket_(socket), handler_(handler) {}



    void handleWriteResponseHeader(std::shared_ptr<std::string> &messageWriteBuffer,
                                   std::shared_ptr<std::vector<size_t> > &headerWriteBuffer,
                                   const boost::system::error_code& error, size_t num_bytes) {

      if (error)
        return;

      // Setup the body buffer
      std::shared_ptr<vector<char> > bodyWriteBuffer(new std::vector<char>(messageWriteBuffer->begin(), messageWriteBuffer->end()));

      // Write the body, then when done read the next command
      boost::asio::async_write(socket_, boost::asio::buffer(*bodyWriteBuffer),
                               boost::bind(&AsyncWriteMessage<WriteHandler>::handleWriteResponseBody, this->shared_from_this(),
                                           bodyWriteBuffer,
                                           boost::asio::placeholders::error,
                                           boost::asio::placeholders::bytes_transferred));
    }


    void handleWriteResponseBody(std::shared_ptr<std::vector<char> > &bodyWriteBuffer,
                                 const boost::system::error_code& error, size_t num_bytes) {

      handler_(error, num_bytes);

    }


    boost::asio::ip::tcp::socket &socket_;  

    WriteHandler handler_;


  };



  template <typename WriteHandler>
  void asyncWriteMessage(boost::asio::ip::tcp::socket &socket,
                         shared_ptr<::google::protobuf::Message> &msg,
                         BOOST_ASIO_MOVE_ARG(WriteHandler) handler) {

    AsyncWriteMessage<WriteHandler>::create(socket, std::move(handler))->start(msg);

  }




  class Env {
  public:

    virtual void init() {}

    virtual StateDesc start() = 0;

    virtual RewardStateTerminal step(const ActionDesc &action) = 0;

    virtual void cleanup() {}

  };


  class Agent {
  public:

    virtual void init() {}

    virtual ActionDesc start(const StateDesc &state) = 0;

    virtual ActionDesc step(const RewardState &rewardState) = 0;

    virtual void end(const float &reward) = 0;

    virtual void cleanup() {}

  };







}


#endif // RL_GLUE_CXX_H

