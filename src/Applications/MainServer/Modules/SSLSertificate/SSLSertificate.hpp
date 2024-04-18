#pragma once
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/context.hpp>
#include <cstddef>
#include <memory>
#include <fstream>
#include <string>

int load_server_certificate(boost::asio::ssl::context& ctx, std::string path_to_sertificate, std::string path_to_key);