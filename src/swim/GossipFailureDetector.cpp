// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 2/12/17.



#include "GossipFailureDetector.hpp"

swim::GossipFailureDetector::GossipFailureDetector(const long &update_round_interval,
                                                   const long &grace_period,
                                                   const long &ping_timeout,
                                                   const long &min_ping_interval)
    : update_round_interval_(update_round_interval), grace_period_(grace_period),
      ping_timeout_(ping_timeout), min_ping_interval_(min_ping_interval) {}




