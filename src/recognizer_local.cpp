#include <filesystem>

#include <string>

#include "recognizer.hpp"

int main(int argc, char const* argv[])
{
    load_server("CN");
    load_stage_index();
    load_hash_index();
    load_templs();

    std::string dir = "../test_images";
    if (std::filesystem::is_directory(dir))
    {
        for (const auto& f : std::filesystem::directory_iterator(dir))
        {
            Recognizer recognizer {RecognizerMode::RESULT};

            cv::Mat img = cv::imread(f.path().u8string());
            std::cout << "File name: " << f.path().filename() << std::endl;
            recognizer.set_image(img);
            recognizer.recognize();
            std::cout << recognizer.get_report() << std::endl;
            for (int i = 0; i < 100; i++)
            {
                std::cout << "_";
            }
            std::cout << std::endl;
        }
    }
    return 0;
}
