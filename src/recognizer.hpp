#ifndef PENGUIN_RECOGNIZER_HPP_
#define PENGUIN_RECOGNIZER_HPP_

#define PENGUIN_VERSION

#include <opencv2/core/version.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>

#include "core.hpp"
#include "depot.hpp"
#include "result.hpp"

static const std::string version = "3.4.0";
static const std::string opencv_version = CV_VERSION;

cv::Mat decode(uint8_t* buffer, size_t size)
{
    std::vector buf(buffer, buffer + size);
    return cv::imdecode(buf, cv::IMREAD_COLOR);
}

void load_server(std::string server)
{
    penguin::server = server;
}

void load_stage_index(std::string stage_index)
{
    auto& resource = penguin::resource;
    resource.add("stage_index", dict::parse(stage_index));
}

void load_hash_index(std::string hash_index)
{
    auto& resource = penguin::resource;
    resource.add("hash_index", dict::parse(hash_index));
}

void load_templ(char* itemId, uint8_t* buffer, size_t size)
{
    cv::Mat templimg = decode(buffer, size);
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
    return penguin::env_check();
}

class Recognizer
{
public:
    Recognizer()
    {
    }
    void recognize()
    {
        _recognize();
    }

private:
    void _recognize()
    {
    }
};

#endif // PENGUIN_RECOGNIZER_HPP_