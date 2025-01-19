#include <iostream>
#include <string>
#include <vector>
#include "CurlHelper.h"
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <regex>

#ifndef LINKGENERATOR_H
#define LINKGENERATOR_H


class LinkGenerator {
private:
    std::vector<std::string> directDownloadLinks;
    CurlHelper* curlObj;
    static std::string cleanPathSegment(const std::string& utf8_str);
    std::string findLink(const xmlNode* node, const std::regex& regexPattern);

public:
    LinkGenerator();
    ~LinkGenerator();
    std::string parseUrlPath(const std::string &url);
    void updateHeaders(const std::string& type);
    void clearHeaders() const;
    void setVerbose(bool verbose);
    std::vector<std::string> getDownloadLinks(const std::string& url, const std::string& type);
    std::string getFuckingfastLink(const std::string& downloadURL);
    std::string getDataNodesLink(const std::string& downloadURL);
};

class toggle_mirrors : public std::exception {
private:
    std::string message;
public:
    explicit toggle_mirrors(const std::string& msg) : message(msg) {}

    // Override the what() function
    const char* what() const noexcept override {
        return message.c_str();
    }
};




#endif //LINKGENERATOR_H
