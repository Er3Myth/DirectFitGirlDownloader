#ifndef CURLHELPER_H
#define CURLHELPER_H

#include <curl/curl.h>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <map>

class CurlHelper {
private:
    CURL* curl;
    struct curl_slist* headerList;

    // Static member function for writing data
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

public:
    struct HttpResponse {
        std::string responseBody;
        std::string responseHeaders;
        long statusCode{};
    };

    CurlHelper()
        : curl(curl_easy_init()), headerList(nullptr) {
        if (!curl) {
            throw std::runtime_error("Failed to initialize CURL");
        }
    }

    ~CurlHelper() {
        if (curl) {
            curl_easy_cleanup(curl);
        }
    }

    // Perform GET request
    HttpResponse get(const std::string& url) {
        HttpResponse response;
        response.responseBody.clear();

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.responseBody);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            throw std::runtime_error("Failed to perform request: " +
                                      std::string(curl_easy_strerror(res)));
        }
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.statusCode);

        return response;
    }

    // Perform POST request
    HttpResponse post(const std::string& url, const std::map<std::string, std::string>& data) {
        HttpResponse response;
        std::string encodedData;
        bool first = true;

        // Build the URL-encoded string
        for (const auto& [key, value] : data) {
            if (!first) {
                encodedData += "&";
            }
            first = false;

            char* encodedKey = curl_easy_escape(curl, key.c_str(), key.length()); // Points to the beginning of the string
            char* escapedValue = curl_easy_escape(curl, value.c_str(), value.length());

            if (encodedKey && escapedValue) {
                encodedData += encodedKey;
                encodedData += "=";
                encodedData += escapedValue;
            }

            curl_free(encodedKey);
            curl_free(escapedValue);
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, encodedData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.responseBody);

        // Set header callback
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.responseHeaders);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            throw std::runtime_error("Failed to perform request: " +
                                   std::string(curl_easy_strerror(res)));
        }
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.statusCode);

        // Debugging

            //std::cout << "Status Code: " << std::to_string(response.statusCode) << std::endl;

        return response;
    }

    // Set custom headers
    void setHeaders(const std::vector<std::string>& headers) {

        for (const auto& header : headers) {
            headerList = curl_slist_append(headerList, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }

    // Clear headers
    void clearHeaders() {
        if (headerList) {
            curl_slist_free_all(headerList);
            headerList = nullptr;
        }
    }

    // Clear cookies for the current session
    void clearCookies() {
        curl_easy_setopt(curl, CURLOPT_COOKIELIST, "ALL");
    }

    void reset() {
        if (curl) {
            curl_easy_cleanup(curl);
            curl = nullptr;
        }
        if (headerList) {
            curl_slist_free_all(headerList);
            headerList = nullptr;
        }
        curl = curl_easy_init();
        if (!curl)
            throw std::runtime_error("Failed to reinitialize CURL");
    }

    // Enable verbose output for debugging
    void setVerbose(bool verbose) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, verbose ? 1L : 0L);
    }
};

#endif // CURLHELPER_H
