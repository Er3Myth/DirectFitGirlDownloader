#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <map>
#include <windows.h>
#include <oleauto.h> // For CComBSTR and COM utilities
#include <thread>
#include <locale> // test

#include "LinkGenerator.h"
#include "ProgressBar.h"


#include "IDManTypeInfo.h"      // Header for COM interface definitions
#include "IDManTypeInfo_i.c"    // Source for COM object implementations

using namespace std;

// Helper class to manage COM object lifetime
template <typename T>
class ComPtr {
public:
    ComPtr() : ptr(nullptr) {}
    ~ComPtr() { if (ptr) ptr->Release(); }

    T** AddressOf() {
        Reset();
        return &ptr;
    }

    T* operator->() const {
        if (!ptr) throw std::runtime_error("Null COM pointer access");
        return ptr;
    }

    T* Get() const { return ptr; }

    void Reset() {
        if (ptr) {
            ptr->Release();
            ptr = nullptr;
        }
    }
     bool isValid() const { return ptr != nullptr; }

private:
    T* ptr;
    // Prevent copying
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;
};

// RAII wrapper for COM initialization
class ComInitializer {
public:
    ComInitializer() {
        HRESULT hr = CoInitialize(nullptr);
        if (FAILED(hr)) throw std::runtime_error("Failed to initialize COM");
    }
    ~ComInitializer() { CoUninitialize(); }
};

// RAII wrapper for VARIANT
class VariantGuard {
public:
    VariantGuard() { VariantInit(&var); }
    ~VariantGuard() { VariantClear(&var); }
    VARIANT* operator&() { return &var; }
    VARIANT& Get() {return var; }
private:
    VARIANT var;
};

// RAIi wrapper for SAFEARRAY
class SafeArrayGuard {
public:
    SafeArrayGuard(SAFEARRAY* array): sa(array) {}
    ~SafeArrayGuard() { if (sa) SafeArrayDestroy(sa); }
    SAFEARRAY* Get() {return sa; }
    SAFEARRAY** AddressOf() { return &sa; }
    operator SAFEARRAY*() const { return sa; }
private:
    SAFEARRAY* sa;
};

// Helper function to safely put string into SAFEARRAY
HRESULT SafePutArrayString(SAFEARRAY* psa, LONG index[], const std::wstring& str);

std::wstring convertToWString(const std::string& str);

int main() {

    // Set the console to use UTF-8 encoding
    system("chcp 65001");
    try {
        // Initialize COM
        ComInitializer comInit;

        // Create an instance of ICIDMLinkTransmitter2
        ComPtr<ICIDMLinkTransmitter2> pIDM;
        HRESULT hr = CoCreateInstance(
            CLSID_CIDMLinkTransmitter,
            nullptr,
            CLSCTX_LOCAL_SERVER,
            IID_ICIDMLinkTransmitter2,
            reinterpret_cast<void**>(pIDM.AddressOf())
            );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create IDM COM object. HRESULT: 0x" );
        }

        std::vector<std::wstring> links;
        std::string url, type;

        // Get FitGirl URL
        std::cout << "Enter the FitGirl webpage URL:" ;
        std::getline(std::cin, url);

        while (true) {
            int choice;
            std::cout << "Please select the source for download mirrors:\n"
                      << "1. DataNodes (recommended)\n"
                      << "2. FuckingFast\n"
                      << "\nEnter your choice (1 or 2): ";
            std::cin >> choice;
            if ( choice == 1 || choice == 2 ) {
                type = (choice == 1) ? "DataNodes" : "FuckingFast";
                std::cout << "\nYou selected " << type << " for download mirrors.\n";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                break;
            }

            std::cout << "\nInvalid choice, please try again.\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }

        const std::string DATANODES_TYPE = "DataNodes";
        const std::string FUCKINGFAST_TYPE = "FuckingFast";

        // Generate links.
        auto fitGirl = LinkGenerator();
        fitGirl.setVerbose(false); // Debugging
        fitGirl.clearHeaders();
        fitGirl.updateHeaders(type);
        std::vector<std::string> linksFuckingFast = fitGirl.getDownloadLinks(url,FUCKINGFAST_TYPE);
        std::vector<std::string> linksDataNodes = fitGirl.getDownloadLinks(url,DATANODES_TYPE);
        std::vector<std::string> linksFitGirl = (type == FUCKINGFAST_TYPE) ? linksFuckingFast : linksDataNodes;

        // Debugging.
        for (string& link : linksFitGirl) {
            cout << link << endl;
        }

        std::string currentType = type;// Track consecutive failures for each mirror type
        std::map<std::string, int> consecutiveFailures = {
            {DATANODES_TYPE, 0},
            {FUCKINGFAST_TYPE, 0}
        };

        const std::string CHOICE_PROMPT =
            "Please select a choice to continue: \n"
            "1. Switch to a different mirror \n"
            "2. Retry the same mirror (ONLY AFTER SWITCHING A VPN/Proxy ) \n"
            "\nEnter your choice (1 or 2): ";

        ProgressBar pb(linksFitGirl.size(),50);
        pb.setPrefix("Testing loading:");
        pb.setSuffix(" Please wait...");

        for (std::string& link : linksFitGirl) {
            bool success = false;

            while (!success) {
                try {
                    links.emplace_back(convertToWString(
                        currentType == DATANODES_TYPE ? fitGirl.getDataNodesLink(link) : fitGirl.getFuckingfastLink(link)
                    ));
                    // Reset the failure counter on success.
                    consecutiveFailures[currentType] = 0;
                    pb.incrememnt();
                    success = true; // Exit loop on success
                }
                catch (const toggle_mirrors& e) {
                    std::cout << e.what() << std::endl;
                    consecutiveFailures[currentType]++;

                    if(consecutiveFailures[FUCKINGFAST_TYPE] >=2 && consecutiveFailures[DATANODES_TYPE] >=2 )
                        throw std::logic_error("No mirrors are working right now. Please try again later.");

                    if (currentType == FUCKINGFAST_TYPE) {
                        bool validInput = false;
                        int retryCount = 0;
                        const int maxRetries = 3;

                        while (!validInput && retryCount < maxRetries) {
                            int choice;
                            std::cout << CHOICE_PROMPT;

                            if (std::cin >> choice) {
                                switch (choice) {
                                    case 1:
                                        currentType = DATANODES_TYPE; // Switch to another mirror
                                        linksFitGirl = linksDataNodes;
                                        fitGirl.updateHeaders(currentType); // Set default Headers
                                        validInput = true;
                                        break;
                                    case 2:
                                        std::cout << "Retrying the same mirror: " << currentType << "\n";
                                        validInput = true;
                                        break;
                                    default:
                                        ++retryCount;
                                        std::cout << "Invalid choice, please try again.\n";
                                }
                            } else {
                                ++retryCount;
                                std::cout << "Invalid input, please enter a number.\n";
                                std::cin.clear();
                                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                            }
                        }

                        if (retryCount >= maxRetries) {
                            throw std::logic_error("Too many invalid inputs. Exiting...");
                        }
                    }
                    else {
                        // Toggle currentType.
                        currentType = FUCKINGFAST_TYPE;
                        linksFitGirl = linksFuckingFast;
                        fitGirl.updateHeaders(currentType); // Set default Headers
                    }
                }
            }
        }
        pb.complete(); // endl
        if (links.empty()) {
            throw std::runtime_error("No links were generated");
        }
        std::cout << "Links were generated, transferring them to IDM now.\n";

        // Create SAFEARRAY
        SAFEARRAYBOUND bounds[2] = {
            {static_cast<ULONG>(links.size()), 0},
            {4, 0}  // href, cookie, description, user agent
        };

        SafeArrayGuard pSA(SafeArrayCreate(VT_BSTR, 2, bounds));
        if (!pSA) {
            throw std::runtime_error("Failed to create SAFEARRAY");
        }

        // Fill SAFEARRAY
        const std::wstring userAgent = L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36";

        for (LONG i = 0; i < static_cast<LONG>(links.size()); ++i) {
            LONG index[2] = {i, 0};

            // URL
            hr = SafePutArrayString(pSA, index, links[i]);
            if (FAILED(hr)) throw std::runtime_error("Failed to add URL to array");

            // Cookie (empty)
            index[1] = 1;
            hr = SafePutArrayString(pSA, index, L"");
            if (FAILED(hr)) throw std::runtime_error("Failed to add cookie to array");

            // Description
            index[1] = 2;
            hr = SafePutArrayString(pSA, index, std::wstring(type.begin(), type.end()));
            if (FAILED(hr)) throw std::runtime_error("Failed to add description to array");

            // User-Agent
            index[1] = 3;
            hr = SafePutArrayString(pSA, index, i == 0 ? userAgent : L"");
            if (FAILED(hr)) throw std::runtime_error("Failed to add user agent to array");
        }

        // Prepare VARIANT and send to IDM
        VariantGuard varLinks;
        varLinks.Get().vt = VT_ARRAY | VT_BSTR;
        varLinks.Get().parray = pSA;

        BSTR bstrReferer = SysAllocString(L"");
        if (!bstrReferer) {
            throw std::runtime_error("Failed to allocate referer string");
        }

        hr = pIDM->SendLinksArray(bstrReferer, &varLinks.Get());
        SysFreeString(bstrReferer);

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to send links to IDM queue");
        }

        std::cout << "Links added to IDM queue successfully!" << std::endl;
        return 0;
    }

    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    catch (const std::logic_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    catch (const std::exception& e) {
        std::cerr << "Don't know wtf happened here: " << e.what() << std::endl;
    }

}

HRESULT SafePutArrayString(SAFEARRAY* psa, LONG index[], const std::wstring& str) {
    BSTR bstr = SysAllocString(str.c_str());
    if (!bstr) return E_OUTOFMEMORY;
    HRESULT hr = SafeArrayPutElement(psa, index, bstr);
    SysFreeString(bstr);
    return hr;
}

std::wstring convertToWString(const std::string& str) {
   return std::wstring(str.begin(), str.end());
}

