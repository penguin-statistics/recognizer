#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace std;
using namespace std::filesystem;
using dict = nlohmann::json;

int main(int argc, char const* argv[])
{
    path p = path(argv[0]).parent_path();
    cout << argc << endl;
    cout << argv[0] << endl;
    cout << current_path() << endl;
    cout << is_regular_file(argv[1]) << endl;
    system("pause");

    return 0;
}
