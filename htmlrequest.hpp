#pragma once

#include <string>
#include <map>
#include "MultipartFormData.hpp"

class HttpRequest
{
	public:
		std::string							method;
		std::string							uri;
		std::string							httpVersion;
		std::map<std::string, std::string>	headers;
		std::string							rawBody;
		MultipartFormData					formattedBody;
		int									client_fd;
		
		HttpRequest(){};
		HttpRequest(const std::string& requestString, int client_fd);
		HttpRequest(const HttpRequest &other) {};
		~HttpRequest();

		bool								HandleRequest(std::map<int, std::string>& clientDataToSend, int client_fd);
};
