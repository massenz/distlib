// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/17/17.


#include <google/protobuf/util/json_util.h>
#include "ApiServer.hpp"

namespace swim {
namespace rest {

int ApiServer::ConnectCallback(void *cls, struct MHD_Connection *connection,
                           const char *url,
                           const char *method,
                           const char *version,
                           const char *upload_data,
                           size_t *upload_data_size,
                           void **con_cls) {
  struct MHD_Response *response;
  int ret;

  auto gfd = static_cast<const swim::GossipFailureDetector *>(cls);

  auto report = gfd->gossip_server().PrepareReport();
  std::string json_body;

  ::google::protobuf::util::MessageToJsonString(report, &json_body);

  response = MHD_create_response_from_buffer(json_body.size(),
                                             (void *) json_body.c_str(),
                                             MHD_RESPMEM_MUST_COPY);
  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);

  return ret;
}

} // namespace rest
} // namespace swim
