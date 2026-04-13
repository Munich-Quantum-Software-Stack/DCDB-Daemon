#include <iostream>
#include <string>
#include <curl/curl.h>

// Helper for writing API
static size_t WriteCb(void* p, size_t sz, size_t nm, std::string* out) {
    out->append(static_cast<char*>(p), sz*nm);
    return sz*nm;
}

int main() {
    // Token: apiv3_LeYjAH9jT6nVx57THHOke-nrU15jd2m8a-ksoHgW4dFgemHi0RCrdFZ-ursmxrEn-wXlkfu-f_G1UmaMwml1wQ
    // HTTP Requests Header: Authorization: 
    // Bearer apiv3_LeYjAH9jT6nVx57THHOke-nrU15jd2m8a-ksoHgW4dFgemHi0RCrdFZ-ursmxrEn-wXlkfu-f_G1UmaMwml1wQ
    const std::string host = "http://host.docker.internal:8181";
    const std::string token = "apiv3_LeYjAH9jT6nVx57THHOke-nrU15jd2m8a-ksoHgW4dFgemHi0RCrdFZ-ursmxrEn-wXlkfu-f_G1UmaMwml1wQ";
    const std::string dbname = "temperature_db";

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return 1;
    }

    // Write something to the database
    /*
    {
        const std::string url  = host + "/api/v3/write_lp?db=" + dbname + "&precision=s";
        const std::string data = "coldlab_temp,sensor_id=42 value=21.7";

        struct curl_slist* hdr = nullptr;
        hdr = curl_slist_append(hdr, ("Authorization: Bearer " + token).c_str());
        hdr = curl_slist_append(hdr, "Content-Type: text/plain");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdr);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);
        long code = 0; curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        if (res != CURLE_OK || code != 204) {
            std::cerr << "Write failed: " << curl_easy_strerror(res)
                      << ", HTTP " << code << "\n";
        } else {
            std::cout << "Write OK\n";
        }
        curl_slist_free_all(hdr);
    } */

    // Query via /api/v3/query_sql
    {
        const std::string url  = host + "/api/v3/query_sql?format=json";
        const std::string body = std::string(R"({"db":")") + dbname +
                                 R"(","q":"SELECT * FROM home ORDER BY time DESC LIMIT  5"})";

        struct curl_slist* hdr = nullptr;
        hdr = curl_slist_append(hdr, ("Authorization: Bearer " + token).c_str());
        hdr = curl_slist_append(hdr, "Content-Type: application/json");

        std::string resp;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdr);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);
        long code = 0; curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        if (res != CURLE_OK || code != 200) {
            std::cerr << "Query failed: " << curl_easy_strerror(res)
                      << ", HTTP " << code << "\n";
        } else {
            std::cout << "Query OK. Response:\n" << resp << "\n";
        }
        curl_slist_free_all(hdr);
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}
