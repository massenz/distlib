// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/17/17.


#include <google/protobuf/util/json_util.h>
#include "ApiServer.hpp"

namespace swim {
namespace rest {

// Mark: CONSTANTS
const char *const kApiVersionPrefix = "/api/v1";
const char *const kNoApiUrl = "Unknown API endpoint; should start with /api/v1/";
const char *const kApplicationJson = "application/json";
const char *const kApplicationProtobuf = "application/x-protobuf";


int ApiServer::ConnectCallback(void *cls, struct MHD_Connection *connection,
                               const char *url,
                               const char *method,
                               const char *version,
                               const char *upload_data,
                               size_t *upload_data_size,
                               void **con_cls) {
  struct MHD_Response *response;
  int ret;

  std::string path{url};
  LOG(INFO) << "Request: " << path;
  if (path.find(kApiVersionPrefix) != 0) {
    response = MHD_create_response_from_buffer(strlen(kNoApiUrl),
                                               (void *) kNoApiUrl,
                                               MHD_RESPMEM_PERSISTENT);
    LOG(ERROR) << "Not a valid API request";
    ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;
  }

  std::string resource = path.substr(path.rfind('/') + 1);
  LOG(INFO) << "Resource: " << resource;

  auto detector = static_cast<const swim::GossipFailureDetector *>(cls);

  auto report = detector->gossip_server().PrepareReport();
  std::string json_body;

  ::google::protobuf::util::MessageToJsonString(report, &json_body);

  response = MHD_create_response_from_buffer(json_body.size(),
                                             (void *) json_body.c_str(),
                                             MHD_RESPMEM_MUST_COPY);
  MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, kApplicationJson);
  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);

  return ret;
}

} // namespace rest
} // namespace swim
