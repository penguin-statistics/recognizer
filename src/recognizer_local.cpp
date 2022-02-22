#include <filesystem>
#include <string>

#include <opencv2/highgui.hpp>
#include "recognizer.hpp"

int main(int argc, char const* argv[])
{
    std::filesystem::path p = std::filesystem::path(argv[0]).parent_path();
    std::filesystem::current_path(p);
    load_server("CN");
    load_stage_index();
    load_hash_index();
    load_templs();

    std::string dir = "../test_images";
    if (std::filesystem::is_directory(dir))
    {
        for (const auto& f : std::filesystem::directory_iterator(dir))
        {
            Recognizer recognizer {"RESULT"};

            cv::Mat img = cv::imread(f.path().u8string());
            std::cout << "File name: " << f.path().filename() << std::endl;
            recognizer.set_img(img);
            recognizer.recognize();
            std::cout << recognizer.get_report(true).dump(4) << std::endl;
            for (int i = 0; i < 100; i++)
            {
                std::cout << "_";
            }
            std::cout << std::endl;
            auto debug_img = recognizer.get_debug_img();
            cv::imshow("", debug_img);
            cv::waitKey();
        }
    }
    return 0;
}
