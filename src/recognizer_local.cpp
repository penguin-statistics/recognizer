#include <filesystem>
#include <string>

#include <opencv2/highgui.hpp>
#include "recognizer.hpp"

void show_img(cv::Mat src)
{
    if (src.rows > 600)
    {
        double fx = 600.0 / src.rows;
        cv::resize(src, src, cv::Size(), fx, fx, cv::INTER_AREA);
    }
    cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);
    cv::imshow("Display window", src);
    cv::waitKey(0);
    cv::destroyWindow("Display window");
}

int main(int argc, char const* argv[])
{
    std::filesystem::path p = std::filesystem::path(argv[0]).parent_path();
    std::filesystem::current_path(p);
    load_server("CN");
    load_stage_index();
    load_hash_index();
    load_templs();

    std::string dir = "../new";
    if (std::filesystem::is_directory(dir))
    {
        for (const auto& f : std::filesystem::directory_iterator(dir))
        {
            Recognizer recognizer {"RESULT"};

            cv::Mat img = cv::imread(f.path().u8string());
            std::cout << "File name: " << f.path().filename() << std::endl;
            auto report = recognizer.recognize(img);
            std::cout << report.dump(4) << std::endl;
            for (int i = 0; i < 100; i++)
            {
                std::cout << "_";
            }
            std::cout << std::endl;
            auto debug_img = recognizer.get_debug_img();
            show_img(debug_img);
        }
    }
    return 0;
}
