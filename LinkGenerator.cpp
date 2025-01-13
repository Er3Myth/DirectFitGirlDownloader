//
// Created by Myth on 12/2/2024.
//

#include "LinkGenerator.h"
#include <format>
#include <iomanip>
#include <thread>
#include <chrono>

LinkGenerator::LinkGenerator()
              :curlObj(new CurlHelper()) {
    xmlInitParser();
}

LinkGenerator::~LinkGenerator() {
    delete curlObj;  // clean up memory
    xmlCleanupParser();
}

std::string LinkGenerator::parseUrlPath(const std::string &url) {
    std::vector<std::string> pathVector;
    const std::regex regexPattern(R"((https?)://([a-zA-Z0-9.-]+)(/[^?]*)$)");

    if (std::smatch match; std::regex_match(url, match, regexPattern)) {
        // return the path
        return match[3];
    }

    return "";
}

std::string LinkGenerator::cleanPathSegment(const std::string& utf8_str) {
    std::string latin1_str;
    latin1_str.reserve(utf8_str.length());

    for (size_t i = 0; i < utf8_str.length(); ++i) {
        unsigned char c = utf8_str[i];

        if (c < 0x80) { // ASCII character check
            latin1_str += c;
        }
        else if (c >= 0xC2 && c < 0xE0 && i + 1 < utf8_str.length()) {
            // 2-byte UTF-8 sequence
            unsigned char c2 = utf8_str[++i];
            if ((c2 & 0xC0) == 0x80) {
                unsigned char latin1_char = ((c & 0x1F) << 6) | (c2 & 0x3F);
                latin1_str += latin1_char;
            }
        }
        // Skip characters outside Latin-1 range
    }
    return latin1_str;
}

// HTML Parsing Functions
std::string LinkGenerator::findLink(const xmlNode *node, const std::regex &regexPattern) {
    const xmlNode* currentNode = node;
    while (currentNode != nullptr ) {
        // Check to see if this is a <script> node
        if (currentNode->type == XML_ELEMENT_NODE && xmlStrcmp(currentNode->name, reinterpret_cast<const xmlChar*>("script")) == 0) {
            // Match the content of the script tag to get the direct download link
            xmlChar* content = xmlNodeGetContent(currentNode);
            if (content != nullptr) {
                std::string scriptText(reinterpret_cast<const char*>(content));
                xmlFree(content);

                std::smatch match;
                if (std::regex_search(scriptText, match, regexPattern)) {
                    return match.str();
                }
            }
        }

        // Recursively search children
        if (currentNode->children != nullptr) {
            std::string result = findLink(currentNode->children, regexPattern);
            if (!result.empty()) {
                return result;
            }
        }

        currentNode = currentNode->next;
    }
    return "";
}

void LinkGenerator::updateHeaders(const std::string& type) {
    curlObj->setHeaders({"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36(KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36"});
    if (type == "DataNodes") {
        curlObj->setHeaders({
        "Content-Type: application/x-www-form-urlencoded",
        "Host: datanodes.to",
        "Origin: https://datanodes.to",
        "Referer: https://datanodes.to/download"
        });
    }
}

void LinkGenerator::clearHeaders() const {
    curlObj->clearHeaders();
}


std::vector<std::string> LinkGenerator::getDownloadLinks(const std::string& url, const std::string& type) {
    std::vector<std::string> links;
    curlObj->clearHeaders();
    const std::vector<std::string> headerInfo {
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36(KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36",
        "Accept-Charset: UTF-8"
        };
    curlObj->setHeaders(headerInfo);

    CurlHelper::HttpResponse textResponse = curlObj->get(url);

    if (textResponse.statusCode != 200) {
        std::cerr << "Status code: " << textResponse.statusCode
                  << ". Most likely DDOS guard intervention on FitGirl's website." << std::endl;
        std::cout << "Kindly copy the links of the selected mirror from FitGirl's *paste* website and paste them here:" << std::endl;
        std::cout << "Once completed, type 'END' (without quotes) on a new line (Enter for a new line) and press Enter to finish:" << std::endl;

        std::string allLinks;
        std::string line;

        while (true) {
            std::getline(std::cin, line);
            if (line == "END") {
                break;
            }

            if (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
                line.pop_back();

            links.emplace_back(line);
        }
        return links;
    }



    htmlDocPtr doc = htmlReadMemory(
        textResponse.responseBody.c_str(),
        textResponse.responseBody.length(),
        url.c_str(),
        "UTF-8",
        HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING |HTML_PARSE_RECOVER
    );

    if (!doc) {
        throw std::runtime_error("Failed to parse HTML content.");
    }

    // Create an XPath context
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx) {
        xmlFree(doc);
        throw std::runtime_error("Failed to create XPath context");
    }

    // XPath query to find all 'a' tags with 'href' containing {type}
    std::string formattedXPath = std::format("//li[a[contains(text(), 'Filehoster: {}')]]//div[@class='su-spoiler-content su-u-clearfix su-u-trim']//a/@href", type);
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(
        reinterpret_cast<const xmlChar*>(formattedXPath.c_str()),
        xpathCtx
    );

    if (!xpathObj) {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        throw std::runtime_error("Failed to evaluate XPath expression.");
    }

    // Extract the href attributes
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    if (nodes) {
        for (int i = 0; i < nodes->nodeNr; ++i) {
            xmlChar* href = xmlNodeGetContent(nodes->nodeTab[i]);
            if (href) {
                links.emplace_back(reinterpret_cast<char*>(href));
                xmlFree(href);
            }
        }
    }

    // Cleanup
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return links;
}

// Main Functions
std::string LinkGenerator::getFuckingfastLink(const std::string& downloadURL) {

    // Get the url with debugging
    curlObj->setVerbose(true);
    CurlHelper::HttpResponse textResponse = curlObj->get(downloadURL);

    //std::cout << "Response Body: " << textResponse.responseBody << std::endl; // Debugging

    if (textResponse.statusCode == 429) {
        // Reset Curl Connection for when user still decides to use this after switching VPN/Proxy.
        //curlObj->reset();
        throw toggle_mirrors(std::format("Status code: {}. Most likely a Cloudflare timeout. Switch mirrors.", textResponse.statusCode));
    }

    if (textResponse.statusCode != 200) {
        throw std::logic_error("Response Code - " + std::to_string(textResponse.statusCode) + ". Error: Request failed, unexpected issue.");
    }

    const std::regex regexPattern(R"(https://fuckingfast\.co/dl/[a-zA-Z0-9_-]+)");
    // Parse HTML using libxml2
    htmlDocPtr doc = htmlReadMemory(
        textResponse.responseBody.c_str(),
        textResponse.responseBody.length(),
        downloadURL.c_str(),
        nullptr,
        HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING
    );

    if (!doc) {
        throw std::runtime_error("Failed to parse HTML content");
    }

    std::string result = findLink(xmlDocGetRootElement(doc), regexPattern);

    // Clean up
    xmlFreeDoc(doc);

    return result;
}

std::string LinkGenerator::getDataNodesLink(const std::string &downloadURL) {

        std::string parsedUrl = parseUrlPath(downloadURL);
        if (parsedUrl.empty()) {
            throw std::logic_error("Failed to parse URL: " + downloadURL);
        }

        const std::string delimiter = "/";
        std::vector<std::string> pathSegments;

        size_t pos = 0;
        std::string token;

        while ((pos = parsedUrl.find(delimiter)) != std::string::npos) {
            token = parsedUrl.substr(0, pos);
            pathSegments.push_back(token);
            parsedUrl.erase(0, pos + delimiter.length());
        }
        pathSegments.push_back(parsedUrl);

        std::string fileCode = cleanPathSegment(pathSegments[1]); // Encodes UTF-8 to Latin-1
        std::string fileName = cleanPathSegment(pathSegments[2]); // Encodes UTF-8 to Latin-1

        // Verbose output
        curlObj->setVerbose(true);

        while (true) {
            // Clear cookies since every request uses the same connection
            curlObj->clearCookies();
            curlObj->setHeaders({
                std::format("Cookie: lang=english; file_name={}; file_code={};", fileName, fileCode)
            });

            CurlHelper::HttpResponse response = curlObj->post(downloadURL, {
            {"op", "download2"},
            {"id", fileCode},
            {"rand", ""},
            {"referer", "https://datanodes.to/download"},
            {"method_free", "Free Download >>"},
            {"method_premium", ""},
            {"adblock_detected", ""}
            });

            if (std::to_string(response.statusCode) == "400" || std::to_string(response.statusCode) == "502") {
                std::cerr << "Status Code: "<< response.statusCode
                          << ". Most likely a timeout from DataNodes or an invalid response." << std::endl
                          << "Retrying in 5 seconds..." << std::endl;
                curlObj->reset(); // Most scuffed ass fix 😭.
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }

            if (std::to_string(response.statusCode) == "302" ) {
                // Get the location from the response header
                std::string prefix = "location: ";
                size_t startPos = response.responseHeaders.find(prefix);

                if (startPos != std::string::npos) {
                    startPos += prefix.length(); // Move start position at the end of prefix
                    size_t endPos = response.responseHeaders.find("\r\n", startPos); // End position
                    if (endPos != std::string::npos) {
                        return response.responseHeaders.substr(startPos, endPos - startPos);
                    }
                }
                else {throw std::logic_error("Failed to extract location header from response.");}
            }
            else
                throw toggle_mirrors(std::format("Status code: {}. Error: Request failed. Possibly a 404 (Not Found) or another unexpected issue. Switch mirrors.", response.statusCode));
        }
}

