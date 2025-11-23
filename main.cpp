#include <curl/curl.h>

int main(){
    // see https://everything.curl.dev/libcurl/globalinit.html
    CURLcode curlInitResult = curl_global_init(CURL_GLOBAL_DEFAULT);
    return 0;
}
