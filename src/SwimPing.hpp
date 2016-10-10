// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/8/16.



#ifndef BRICK_SWIMPING_HPP
#define BRICK_SWIMPING_HPP

#include "swim.pb.h"

using swim::Server;
using swim::SwimStatus;

class SwimPing {

  Server server_;

public:

  /**
   * Builds a status update message to be sent to the `server`.
   *
   * @param server where to send the message to.
   */
  SwimPing(const Server& server) : server_(server)  {}

  /**
   * Sends the status update.
   *
   * @param msg this server's status.
   */
  void send(const SwimStatus &msg);
};


#endif //BRICK_SWIMPING_HPP
