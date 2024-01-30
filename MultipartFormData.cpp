#include "MultipartFormData.hpp"
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cstdlib>

MultipartFormData::MultipartFormData(std::string contentType, std::string rawContent)
{
    boundary = contentType.substr(contentType.find("boundary=") + 9);
    std::string delimiter = "--" + boundary;
    std::string delimiterEnd;
	size_t returnPos = boundary.find('\r');
	if (returnPos != std::string::npos) {
		// Insérer les tirets avant le caractère de retour chariot
		delimiterEnd = boundary;
		delimiterEnd.insert(returnPos, "--");
	} else {
		// Si pas de retour chariot, ajouter les tirets à la fin normalement
		delimiterEnd = boundary + "--";
	}

	delimiterEnd = "--" + delimiterEnd;

    std::istringstream rawStream(rawContent);
    std::string line;
    contentData data;
    
    while (getline(rawStream, line, '\n'))
    {
        std::cout << "Read line: " << line << std::endl;

        if (line == delimiter)
        {
            std::cout << "Delimiter found" << std::endl;
            if (data.name != "")
            {
                if (data.content.size() >= 4)
                    data.content = data.content.substr(2, data.content.size() - 4);
                std::cout << "Processed data - Name: " << data.name << ", Filename: " << data.filename << ", Content: " << data.content << std::endl;
                content.push_back(data);
                data = contentData(); // Reset data for next content block
            }
            continue;
        }
        else if (!line.find(delimiterEnd))
        {
            std::cout << "Delimiter end found" << std::endl;
            if (data.name != "")
            {
                if (data.content.size() >= 4)
                    data.content = data.content.substr(2, data.content.size() - 4);
                std::cout << "Processed last data - Name: " << data.name << ", Filename: " << data.filename << ", Content: " << data.content << std::endl;
                content.push_back(data);
            }
            std::cout << "End of processing" << std::endl;
            break;
        }
        else if (line.find("Content-Disposition: form-data; name=") != std::string::npos)
        {
            std::cout << "Extracting content data" << std::endl;
            data.name = line.substr(line.find("name=") + 6);
            data.name = data.name.substr(0, data.name.find("\""));
            data.filename = line.substr(line.find("filename=") + 10);
            data.filename = data.filename.substr(0, data.filename.find("\""));
            getline(rawStream, line, '\n'); // Skip the next line (Content-Type or empty)
        }
        else
        {
            std::cout << "Adding line to content" << std::endl;
            data.content += line + "\n";
        }
    }
}