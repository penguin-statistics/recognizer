#include "json.hpp"
#include <fstream>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
using namespace std;
using namespace cv;
using dict = nlohmann::json;

std::vector<uchar> load_file(std::string const& filepath)
{
    std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);

    if (!ifs)
        throw std::runtime_error(filepath + ": " + std::strerror(errno));

    auto end = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    auto size = std::size_t(end - ifs.tellg());

    if (size == 0) // avoid undefined behavior
        return {};

    std::vector<uchar> buffer(size);

    if (!ifs.read((char*)buffer.data(), buffer.size()))
        throw std::runtime_error(filepath + ": " + std::strerror(errno));

    return buffer;
}

void show_img(Mat& src)
{
    namedWindow("Display window", WINDOW_AUTOSIZE);
    imshow("Display window", src);
    waitKey(0);
}

int main(int argc, char const* argv[])
{
    auto rawdata = load_file("err.png");
    Mat img;
    img = imdecode(rawdata, 1);
    show_img(img);
}
