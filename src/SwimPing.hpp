// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/8/16.



#ifndef BRICK_SWIMPING_HPP
#define BRICK_SWIMPING_HPP

#include "swim.pb.h"

using swim::Forwarder;
using swim::SwimStatus;

class SwimPing {

  Forwarder forwarder_;

public:
  SwimPing(const Forwarder& fwd) : forwarder_(fwd)  {}
  void send(const SwimStatus &msg);

};


#endif //BRICK_SWIMPING_HPP
