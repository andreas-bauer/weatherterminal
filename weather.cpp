#include "nlohmann/json.hpp"
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, json* output) {
    size_t totalSize = size * nmemb;

    std::string chunk(static_cast<char*>(contents), totalSize);

    std::vector<json> temp_curl;
    temp_curl.emplace_back(json::parse(chunk));

    for (const auto& jsonObject : temp_curl) {
        *output += jsonObject;
    }

    return totalSize;
}

json APIRequest(const std::string& url) {
    CURL* curl;
    CURLcode res;
    json response;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cout << "Error" << std::endl;
            std::cerr << "curl_easy_perform() failed: "
                      << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }
    return response;
}

json loadCache() {
    std::string homeDir = std::getenv("HOME");
    std::string cache_path = homeDir + "/.config/weather_cache.json";

    std::ifstream cache_file(cache_path);
    json cache_json;
    cache_file >> cache_json;

    cache_file.close();

    return cache_json;
}

void writeToCache(json cache) {
    std::string homeDir = std::getenv("HOME");
    std::string cache_path = homeDir + "/.config/weather_cache.json";

    std::ofstream output_file(cache_path);
    output_file << std::setw(4) << cache << std::endl;

    output_file.close();
}

void updateCacheData(std::string json_key, std::string input_str) {
    std::cout << "updating cache: " << json_key << "with " << input_str
              << std::endl;
    json cache = loadCache();
    cache[json_key] = input_str;
    writeToCache(cache);
}

void createConfig(std::string config_path) {
    json config = json::parse(R"(
                            {
                              "api_key": "",
                              "city": "",
                              "temp_unit": "",
							  "interval_min: 15
                            }
                            )");

    std::ofstream output_file(config_path);
    output_file << std::setw(4) << config << std::endl;

    output_file.close();
}

json loadConfig() {
    std::string homeDir = std::getenv("HOME");
    std::string config_path = homeDir + "/.config/.weather_config.json";

    if (!fs::exists(config_path)) {
        createConfig(config_path);
    }
    std::ifstream inputFile(config_path);
    json input_json;
    inputFile >> input_json;

    inputFile.close();

    return input_json;
}

int calculateTimeDiff(const std::chrono::system_clock::time_point start,
                      const std::chrono::system_clock::time_point end) {
    std::chrono::seconds time_diff =
        std::chrono::duration_cast<std::chrono::seconds>(end - start);
    return time_diff.count();
}

std::string
timeToString(const std::chrono::system_clock::time_point time_point) {
    std::time_t time_value = std::chrono::system_clock::to_time_t(time_point);
    std::stringstream ss;
    ss << time_value;
    return ss.str();
}

std::chrono::system_clock::time_point stringToTime(const std::string time_str) {
    std::time_t time_value = std::stoi(time_str);
    return std::chrono::system_clock::from_time_t(time_value);
}

std::string setWeatherIcon(std::string icon_val) {
    if (icon_val == "01d") {
        return "󰖙";
    } else if (icon_val == "01n") {
        return "";
    } else if (icon_val == "02d") {
        return "";
    } else if (icon_val == "02n") {
        return "";
    } else if (icon_val == "03d" || icon_val == "03n") {
        return "";
    } else if (icon_val == "04d" || icon_val == "04n") {
        return "";
    } else if (icon_val == "09d" || icon_val == "09n") {
        return "";
    } else if (icon_val == "10d") {
        return "";
    } else if (icon_val == "10n") {
        return "";
    } else if (icon_val == "11d") {
        return "";
    } else if (icon_val == "11n") {
        return "";
    } else if (icon_val == "13d" || icon_val == "13n") {
        return "";
    } else if (icon_val == "50d") {
        return "";
    } else if (icon_val == "50n") {
        return "";
    } else {
        std::cout << "Error" << std::endl;
        std::cerr << "Issues retrieving correct icon identifier" << std::endl;
        return "Error";
    }
}

void createCache(std::string cache_path) {
    json cache = json::parse(R"(
                           {
                            "date": "",
                            "city": "",
                            "icon": "",
                            "temp": ""
                           }
                          )");
    std::ofstream output_file(cache_path);
    output_file << std::setw(4) << cache << std::endl;
}

void printCache(bool display_icon) {
    json cache = loadCache();

    if (display_icon) {
        std::cout << std::string(cache["icon"]) << std::endl;
    } else {
        std::cout << std::string(cache["temp"]) << std::endl;
    }
}

bool isCacheValid(int interval) {
    std::string homeDir = std::getenv("HOME");
    std::string cache_path = homeDir + "/.config/weather_cache.json";

    if (!fs::exists(cache_path)) {
        createCache(cache_path);
        return false;
    }
    json cache = loadCache();
    json config = loadConfig();

    std::string cached_date_str = cache["date"].template get<std::string>();
    std::chrono::system_clock::time_point cached_date =
        stringToTime(cached_date_str);

    std::string cached_city = cache["city"].template get<std::string>();
    std::string config_city = config["city"].template get<std::string>();

    std::chrono::system_clock::time_point current_time =
        std::chrono::system_clock::now();

    int diff_in_sec = calculateTimeDiff(cached_date, current_time);

    if ((interval * 60) > diff_in_sec && cached_city == config_city) {
        return true;
    } else {
        return false;
    }
}

double convertToCelsius(double kelvin_temp) { return kelvin_temp - 273.15; }

double convertToFahrenheit(double kelvin_temp) {
    return (kelvin_temp - 273.15) * 9 / 5 + 32;
}

std::string lowerCaseString(std::string str) {
    for (char& ch : str) {
        ch = std::tolower(ch);
    }
    return str;
}

int main(int argc, char* argv[]) {
    json config = loadConfig();
    bool display_icon = false;

    if (argc > 1 && std::string(argv[1]) == "icon") {
        display_icon = true;
    }

    std::string api_key;
    std::string city;
    std::string temp_unit;
    int interval_min = -1;

    try {
        api_key = config["api_key"].template get<std::string>();
        city = config["city"].template get<std::string>();
        temp_unit = config["temp_unit"].template get<std::string>();
        interval_min = config["interval_min"].template get<int>();
    } catch (const std::exception&) {
        std::cout << "Error" << std::endl;
        std::cerr << "Incomplete config setup" << std::endl;
        return 0;
    }

    if (api_key.empty()) {
        std::cout << "Error" << std::endl;
        std::cerr << "Missing api key in config" << std::endl;
        return 0;
    }

    if (city.empty()) {
        std::cout << "Error" << std::endl;
        std::cerr << "Missing city in config" << std::endl;
        return 0;
    }

    if (temp_unit.empty()) {
        std::cout << "Error" << std::endl;
        std::cerr << "Missing temp unit in config" << std::endl;
        return 0;
    }

    if (interval_min == -1) {
        std::cout << "Error" << std::endl;
        std::cerr << "Missing interval for API requests in config" << std::endl;
        return 0;
    }

    bool use_cache = isCacheValid(interval_min);

    if (use_cache) {
        printCache(display_icon);
        return 0;
    } else {
        updateCacheData("date", timeToString(std::chrono::system_clock::now()));
    }

    std::string city_url =
        "http://api.openweathermap.org/geo/1.0/direct?q=" + city +
        "&appid=" + api_key;

    json cityInfo = APIRequest(city_url)[0][0];

    double lat;
    double lon;

    try {
        lat = cityInfo["lat"].template get<double>();
        lon = cityInfo["lon"].template get<double>();
    } catch (std::exception&) {
        std::cout << "Error" << std::endl;
        std::cerr << "API Request failed" << std::endl;
        return 0;
    }

    std::string weather_url =
        "https://api.openweathermap.org/data/2.5/weather?lat=" +
        std::to_string(lat) + "&lon=" + std::to_string(lon) +
        "&appid=" + api_key;

    json weather_info = APIRequest(weather_url)[0];

    double temp_kelvin;
    std::string weather_id;

    try {
        temp_kelvin = weather_info["main"]["temp"].template get<double>();
        weather_id =
            weather_info["weather"][0]["icon"].template get<std::string>();
    } catch (std::exception&) {
        std::cout << "Error" << std::endl;
        std::cerr << "API request failed" << std::endl;
        return 0;
    }

    temp_unit = lowerCaseString(temp_unit);
    std::string temp_print;

    if (temp_unit == "celsius") {
        double temp_celsius = convertToCelsius(temp_kelvin);
        temp_print = std::to_string(int(temp_celsius)) + "°";
    } else if (temp_unit == "fahrenheit") {
        double temp_fahrenheit = convertToFahrenheit(temp_kelvin);
        temp_print = std::to_string(int(temp_fahrenheit)) + "°F";
    } else if (temp_unit == "kelvin") {
        temp_print = std::to_string(int(temp_kelvin)) + "K";
    } else {
        std::cout << "Error" << std::endl;
        std::cerr << "temp unit not recognized. The following are supported: "
                     "celsius, fahrenheit and kelvin"
                  << std::endl;
        return 0;
    }

    std::string icon = setWeatherIcon(weather_id);
    updateCacheData("icon", icon);
    updateCacheData("temp", temp_print);
    updateCacheData("city", city);

    if (display_icon) {
        std::cout << icon << std::endl;
    } else {
        std::cout << temp_print << std::endl;
    }
}
