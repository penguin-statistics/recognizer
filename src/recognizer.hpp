#ifndef PENGUIN_RECOGNIZER_HPP_
#define PENGUIN_RECOGNIZER_HPP_

#define PENGUIN_VERSION

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include <opencv2/core/version.hpp>
#include <opencv2/imgcodecs.hpp>

#include "core.hpp"
#include "depot.hpp"
#include "result.hpp"

static const std::string version = "3.4.0";
static const std::string opencv_version = CV_VERSION;

enum class RecognizerMode
{
    UNDEFINED = 0,
    RESULT = 1,
    DEPOT = 2
};

cv::Mat decode(std::string JSarrayBuffer)
{
    std::vector<uint8_t> buf(JSarrayBuffer.cbegin(), JSarrayBuffer.cend());
    return cv::imdecode(buf, cv::IMREAD_COLOR);
}

cv::Mat decode(uint8_t* buffer, size_t size)
{
    std::vector buf(buffer, buffer + size);
    return cv::imdecode(buf, cv::IMREAD_COLOR);
}

void load_server(std::string server)
{
    penguin::server = server;
}

void load_stage_index() // local
{
    std::ifstream f;
    dict stage_index;
    f.open("../resources/json/stage_index.json");
    f >> stage_index;
    f.close();
    penguin::resource.add("stage_index", stage_index);
}

void load_stage_index(std::string stage_index) // wasm
{
    auto& resource = penguin::resource;
    resource.add("stage_index", dict::parse(stage_index));
}

void load_hash_index() // local
{
    std::ifstream f;
    dict hash_index;
    f.open("../resources/json/hash_index.json");
    f >> hash_index;
    f.close();
    penguin::resource.add("hash_index", hash_index);
}

void load_hash_index(std::string hash_index) // wasm
{
    auto& resource = penguin::resource;
    resource.add("hash_index", dict::parse(hash_index));
}

void load_templs() // local
{
    std::map<std::string, cv::Mat> item_templs;
    for (auto& templ : std::filesystem::directory_iterator(
             "../resources/icon/items"))
    {
        std::string itemId = templ.path().stem().string();
        cv::Mat templimg = cv::imread(templ.path().string());
        item_templs[itemId] = templimg;
    }
    penguin::resource.add("item_templs", item_templs);
}

void load_templs(std::string itemId, std::string JSarrayBuffer) // wasm
{
    cv::Mat templimg = decode(JSarrayBuffer);
    auto& resource = penguin::resource;
    if (!resource.contains<std::map<std::string, cv::Mat>>("item_templs"))
    {
        resource.add("item_templs", std::map<std::string, cv::Mat>());
    }
    auto& item_templs =
        resource.get<std::map<std::string, cv::Mat>>("item_templs");
    item_templs[itemId] = templimg;
}

const bool env_check()
{
    return penguin::env_check(); // to review
}

class Recognizer
{
public:
    Recognizer(RecognizerMode mode)
    {
        _mode = mode;
    }
    Recognizer(std::string mode)
    {
        if (mode == "RESULT")
        {
            _mode = RecognizerMode::RESULT;
        }
        else if (mode == "DEPOT")
        {
            _mode = RecognizerMode::DEPOT;
        }
        else
        {
            _mode = RecognizerMode::UNDEFINED;
        }
    }

    void set_image(cv::Mat img)
    {
        _img = img;
    }
    void set_image(std::string JSarrayBuffer)
    {
        int64 start = cv::getTickCount();
        _img = decode(JSarrayBuffer);
        int64 end = cv::getTickCount();
        _report["cost"]["decode"] =
            std::to_string((end - start) / cv::getTickFrequency() * 1000) + "ms";
    }

    std::string get_report()
    {
        return _report.dump();
    }

    void recognize()
    {
        switch (_mode)
        {
        case RecognizerMode::RESULT:
        {
            int64 start = cv::getTickCount();
            penguin::Result result {_img};
            result.analyze();
            int64 end = cv::getTickCount();
            _report.merge_patch(result.report());
            _report["md5"] = result.get_md5();
            _report["cost"]["recognize"] =
                std::to_string((end - start) / cv::getTickFrequency() * 1000) + "ms";
            break;
        }
        default:
        {
            _report = "[ERROR] Invalid Recognizer Mode";
            break;
        }
        }
    }

private:
    RecognizerMode _mode;
    cv::Mat _img;
    dict _report;
};

#endif // PENGUIN_RECOGNIZER_HPP_