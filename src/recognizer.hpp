#ifndef PENGUIN_RECOGNIZER_HPP_
#define PENGUIN_RECOGNIZER_HPP_

#define PENGUIN_VERSION

#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <opencv2/core/version.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "core.hpp"
#include "depot.hpp"
#include "result.hpp"

static const std::string version = "3.4.1";
static const std::string opencv_version = CV_VERSION;

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
    penguin::resource.add("stage_index", stage_index);
}

void wload_stage_index(std::string stage_index) // wasm
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
    penguin::resource.add("hash_index", hash_index);
}

void wload_hash_index(std::string hash_index) // wasm
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

void wload_templs(std::string itemId, std::string JSarrayBuffer) // wasm
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
    Recognizer(std::string mode)
    {
        _mode = mode;
    }

    void set_img(cv::Mat img) // local
    {
        _img = img;
    }
    void wset_img(std::string JSarrayBuffer) // wasm
    {
        int64 start = cv::getTickCount();
        _img = decode(JSarrayBuffer);
        int64 end = cv::getTickCount();
        _decode_time = (end - start) / cv::getTickFrequency() * 1000;
    }

    void recognize()
    {
        if (_mode == "RESULT")
        {
            int64 start = cv::getTickCount();
            _result.set_img(_img);
            _result.analyze();
            int64 end = cv::getTickCount();
            _md5 = _result.get_md5();
            _recognize_time = (end - start) / cv::getTickFrequency() * 1000;
        }
    }
    void wrecognize()
    {
        recognize();
    }

    dict get_report(bool detail = false)
    {
        dict report;
        if (_mode == "RESULT")
        {
            if (detail)
            {
                report.merge_patch(_result.report(true));
            }
            else
            {
                report.merge_patch(_result.report());
            }
            report["md5"] = _md5;
            if (const auto& decode_time = _decode_time; decode_time)
            {
                report["cost"]["decode"] = decode_time;
            }
            report["cost"]["recognize"] = _recognize_time;
        }
        return report;
    }
    std::string wget_report(bool detail = false, bool pretty_print = false)
    {
        if (pretty_print)
        {
            return get_report(detail).dump(4);
        }
        else
        {
            return get_report(detail).dump();
        }
    }

    cv::Mat get_debug_img()
    {
        auto get_rect = [](dict arr) {
            return cv::Rect(arr[0], arr[1], arr[2], arr[3]);
        };
        cv::Mat img = _img.clone();
        if (const auto& report = get_report();
            report.contains("exceptions"))
        {
            for (const auto& exp : report["exceptions"])
            {
                if (exp["type"] == "ERROR" &&
                    exp["detail"].contains("rect") &&
                    exp["detail"]["rect"] != "empty")
                {
                    cv::Rect rect = get_rect(exp["detail"]["rect"]);
                    cv::rectangle(img, rect, cv::Scalar(0, 0, 255));
                }
            }
        }
        return img;
    }
#ifdef PENGUIN_RECOGNIZER_WASM_CPP_
#include <emscripten.h>
#include <emscripten/val.h>
    emscripten::val wget_debug_img()
    {
        cv::Mat img = get_debug_img();
        std::vector<uint8_t> buf;
        cv::imencode(".png", img, buf);
        return emscripten::val(
            emscripten::typed_memory_view(buf.size(), buf.data()));
    }
#endif // PENGUIN_RECOGNIZER_WASM_CPP_

private:
    std::string _mode;
    cv::Mat _img;
    penguin::Result _result;
    penguin::Depot _depot;
    dict _report;
    std::string _md5;
    double _decode_time = 0;
    double _recognize_time = 0;
};

#endif // PENGUIN_RECOGNIZER_HPP_