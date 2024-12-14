//
// Created by Marco Massenzio on 11/3/24.
//

#include <glog/logging.h>

int main(int argc, char* argv[]) {
    // Initialize Google logging.
    FLAGS_log_dir = "./logs";
    google::InitGoogleLogging(argv[0]);

    // Log an info message.
    LOG(INFO) << "Hello, this is an info message from glog!";

    // Shutdown Google logging.
    google::ShutdownGoogleLogging();
    return 0;
}
