#pragma once

#include <iostream>
#include <string>
#include <vector>

struct contentData
{
	std::string	name;
	std::string	filename;
	std::string	content;
};

class MultipartFormData
{
	public:
		MultipartFormData(){};
		MultipartFormData(std::string contentType, std::string rawContent);
		MultipartFormData(const MultipartFormData &other){};
		~MultipartFormData(){};

		std::string					boundary;
		std::vector<contentData>	content;
};
