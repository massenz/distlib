// Copyright (c) 2017 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 10/17/17.


#include <google/protobuf/util/json_util.h>
#include "../../../include/swim/rest/ApiServer.hpp"

namespace swim {
namespace rest {

// Mark: CONSTANTS
const char *const kApiVersionPrefix = "/api/v1";
const char *const kApplicationJson = "application/json";
const char *const kApplicationProtobuf = "application/x-protobuf";

// Mark: ERROR CONSTANTS
const char *const kNoApiUrl = "Unknown API endpoint; should start with /api/v1/";
const char *const kIllegalRequest = "Cannot parse JSON into valid PB";
const char *const kInvalidResource = "Not a valid resource";

int getReport(struct MHD_Connection *connection, const swim::GossipFailureDetector *gfd) {
  auto report = gfd->gossip_server().PrepareReport();
  std::string json_body;

  ::google::protobuf::util::MessageToJsonString(report, &json_body);

  auto response = MHD_create_response_from_buffer(json_body.size(),
                                                  (void *) json_body.c_str(),
                                                  MHD_RESPMEM_MUST_COPY);
  MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, kApplicationJson);
  int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);

  return ret;

}

bool postServer(swim::GossipFailureDetector *gfd, const std::string &jsonBody) {

  Server neighbor;
  auto status = ::google::protobuf::util::JsonStringToMessage(jsonBody, &neighbor);
  if (!status.ok()) {
    LOG(ERROR) << "Not valid JSON: " << jsonBody;
    return false;
  }
  gfd->AddNeighbor(neighbor);
  VLOG(2) << "Added neighbor " << neighbor;
  return true;
}

struct post_info {
  swim::GossipFailureDetector *detector;
  std::string resource;
  bool success;
};

int ApiServer::ConnectCallback(void *cls,
                               struct MHD_Connection *connection,
                               const char *url,
                               const char *method,
                               const char *version,
                               const char *upload_data,
                               size_t *upload_data_size,
                               void **con_cls) {
  auto detector = static_cast<swim::GossipFailureDetector *>(cls);
  std::string path{url};

  VLOG(2) << method << " Request: " << path;
  if (path.find(kApiVersionPrefix) != 0) {
    auto response = MHD_create_response_from_buffer(strlen(kNoApiUrl),
                                                    (void *) kNoApiUrl,
                                                    MHD_RESPMEM_PERSISTENT);
    LOG(ERROR) << "Not a valid API request: " << path;
    MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return MHD_YES;
  }

  std::string resource = path.substr(path.rfind('/') + 1);
  VLOG(2) << "Resource: " << resource;

  // TODO: probably a more general request routing approach would be desirable.
  if (strcmp(method, "GET") == 0) {
    if (resource == "report") {
      return getReport(connection, detector);
    }
  } else if (strcmp(method, "POST") == 0) {
    if (*con_cls == nullptr) {
      auto con_info = new post_info{detector, resource, false};
      *con_cls = con_info;
      return MHD_YES;
    }
    auto con_info = static_cast<post_info *>(*con_cls);
    if (*upload_data_size != 0) {
      std::string body{upload_data, *upload_data_size};
      VLOG(2) << "Data (" << *upload_data_size << "): " << body;
      if (con_info->resource == "server") {
        con_info->success = postServer(detector, body);
        *upload_data_size = 0;
      }
      return MHD_YES;
    }
    VLOG(2) << "POST processing completed";
    auto response = MHD_create_response_from_buffer(2, (void *) "OK", MHD_RESPMEM_PERSISTENT);
    int ret;
    if (con_info->success) {
      ret = MHD_queue_response(connection, MHD_HTTP_CREATED, response);
    } else {
      ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
    }
    MHD_destroy_response(response);
    return ret;
  }
  auto response = MHD_create_response_from_buffer(strlen(kInvalidResource),
                                                  (void *) kInvalidResource,
                                                  MHD_RESPMEM_PERSISTENT);
  LOG(ERROR) << "Not a valid REST entity: " << resource;
  MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
  MHD_destroy_response(response);
  return MHD_NO;
}


} // namespace rest
} // namespace swim
