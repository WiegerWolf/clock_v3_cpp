#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using namespace cpr;
using namespace std;
using json = nlohmann::json;

struct Image {
    string fullUrl;
    string date;
};
// https://json.nlohmann.me/features/arbitrary_types/#simplify-your-life-with-macros
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Image, fullUrl, date)

string getBgImageUrl(){
    // https://docs.libcpr.dev/introduction.html#get-requests
    Response response = Get(Url{"https://peapix.com/bing/feed?country=us"});

    // https://json.nlohmann.me/features/parsing/json_lines/
    json response_json = json::parse(response.text);
    auto first_image = response_json.get<vector<Image>>()[0];
    return first_image.fullUrl;
}

int main(){
    string bg_image_url = getBgImageUrl();
    Response response = Get(Url{bg_image_url}, ReserveSize{1024*1024}); // Increase reserve size to 1MB
    return 0;
}
