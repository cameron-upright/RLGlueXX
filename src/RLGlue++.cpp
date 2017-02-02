#include <string>
#include <vector>

#include <iostream>

#include "RLGlue++.h"

using namespace std;


namespace RLGlue {

  void writeMessage(boost::asio::ip::tcp::socket &socket, const ::google::protobuf::Message &msg) {

    std::string msgData;

    msg.SerializeToString(&msgData);

    write(socket, boost::asio::buffer(std::vector<size_t>{msgData.size()}));
    write(socket, boost::asio::buffer(msgData));
  }

}
