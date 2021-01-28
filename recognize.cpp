#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
using namespace std;
using namespace std::filesystem;
using namespace cv;
using dict = nlohmann::json;

#define TOP 0
#define BOTTOM 1
#define LEFT 2
#define RIGHT 3

#define BEGIN 0
#define END 1

vector<Mat> templs;
dict stage_index, item_index, hash_index;
dict hsv_index = {
    { "51", "LMB" },
    { "201", "FIRST" },
    { "0", "NORMAL_DROP" },
    { "360", "NORMAL_DROP" },
    { "25", "SPECIAL_DROP" },
    { "63", "EXTRA_DROP" },
    { "24", "FURNITURE" }
};

//**********DEBUG TOOLS**********

void show_img(Mat& src)
{
    namedWindow("Display window", WINDOW_AUTOSIZE);
    imshow("Display window", src);
    waitKey(0);
}

void preload()
{
    ifstream f;
    f.open("src\\stage_index.json");
    f >> stage_index;
    f.close();
    f.clear();
    f.open("src\\item_index.json");
    f >> item_index;
    f.close();
    f.clear();
    f.open("src\\hash_index.json");
    f >> hash_index;
    f.close();
    f.clear();

    int count = 0;
    for (auto& item : item_index.items()) {
        templs.push_back(imread("src\\items\\" + item.key() + ".jpg"));
        item_index[item.key()]["img"] = count;
        count++;
    }
}

//**********DEBUG TOOLS**********

// int find_edge(Mat src)
// {
//     if (src.channels() != 1)
//         throw "Require image in graystyle";

//     int edge = -1;
//     for (int co = 0; co < src.cols; co++) {
//         uchar* pix = src.data + co;
//         for (int ro = 0; ro < src.rows; ro++) {
//             if ((bool)*pix) {
//                 edge = co;
//                 break;
//             }
//             pix = pix + src.step;
//         }
//         if (edge >= 0)
//             break;
//     }
//     return edge;
// }

vector<int> separate(Mat src_bin, int direc, int n = 0, bool roi = false)
{
    if (src_bin.empty())
        throw invalid_argument("separate(): Image is empty");
    if (src_bin.channels() != 1)
        throw invalid_argument("separate(): Require image in graystyle");
    if (direc < 0 || direc > 3)
        throw invalid_argument("separate(): Invalid direction");

    int offset = 0;
    if (roi) {
        Size wholesize;
        Point upperleft;
        src_bin.locateROI(wholesize, upperleft);
        if (direc >= 2)
            offset = upperleft.x;
        else
            offset = upperleft.y;
    }

    vector<int> sp;
    bool isodd = false;
    if (direc == TOP) {
        for (int ro = 0; ro < src_bin.rows; ro++) {
            uchar* pix = src_bin.data + ro * src_bin.step;
            int co = 0;
            for (; co < src_bin.cols; co++) {
                if ((bool)*pix && !isodd) {
                    sp.push_back(ro + offset);
                    isodd = !isodd;
                    break;
                } else if ((bool)*pix && isodd)
                    break;
                pix++;
            }
            if (co == src_bin.cols && isodd) {
                sp.push_back(ro + offset);
                isodd = !isodd;
                if (sp.size() == 2 * n)
                    break;
            }
        }
        if (isodd)
            sp.push_back(src_bin.rows + offset);
    } else if (direc == BOTTOM) {
        for (int ro = src_bin.rows - 1; ro >= 0; ro--) {
            uchar* pix = src_bin.data + ro * src_bin.step;
            int co = 0;
            for (; co < src_bin.cols; co++) {
                if ((bool)*pix && !isodd) {
                    sp.push_back(ro + 1 + offset);
                    isodd = !isodd;
                    break;
                } else if ((bool)*pix && isodd)
                    break;
                pix++;
            }
            if (co == src_bin.cols && isodd) {
                sp.insert(sp.end() - 1, ro + 1 + offset);
                isodd = !isodd;
                if (sp.size() == 2 * n)
                    break;
            }
        }
        if (isodd)
            sp.insert(sp.end() - 1, 0 + offset);
    } else if (direc == LEFT) {
        for (int co = 0; co < src_bin.cols; co++) {
            uchar* pix = src_bin.data + co;
            int ro = 0;
            for (; ro < src_bin.rows; ro++) {
                if ((bool)*pix && !isodd) {
                    sp.push_back(co + offset);
                    isodd = !isodd;
                    break;
                } else if ((bool)*pix && isodd)
                    break;
                pix = pix + src_bin.step;
            }
            if (ro == src_bin.rows && isodd) {
                sp.push_back(co + offset);
                isodd = !isodd;
                if (sp.size() == 2 * n)
                    break;
            }
        }
        if (isodd)
            sp.push_back(src_bin.cols + offset);
    } else if (direc == RIGHT) {
        for (int co = src_bin.cols - 1; co >= 0; co--) {
            uchar* pix = src_bin.data + co;
            int ro = 0;
            for (; ro < src_bin.rows; ro++) {
                if ((bool)*pix && !isodd) {
                    sp.push_back(co + 1 + offset);
                    isodd = !isodd;
                    break;
                } else if ((bool)*pix && isodd)
                    break;
                pix = pix + src_bin.step;
            }
            if (ro == src_bin.rows && isodd) {
                sp.insert(sp.end() - 1, co + 1 + offset);
                isodd = !isodd;
                if (sp.size() == 2 * n)
                    break;
            }
        }
        if (isodd)
            sp.insert(sp.end() - 1, 0 + offset);
    }
    return sp;
}

void squarize(Mat& src_bin)
{
    if (src_bin.empty())
        throw invalid_argument("squarize(): Image is empty");
    if (src_bin.channels() != 1)
        throw invalid_argument("squarize(): Require image in graystyle");

    int h = src_bin.rows, w = src_bin.cols;
    if (h > w) {
        int d = h - w;
        hconcat(Mat(h, d / 2, CV_8UC1, Scalar(0)), src_bin, src_bin);
        hconcat(src_bin, Mat(h, d - d / 2, CV_8UC1, Scalar(0)), src_bin);
    }
    if (w > h) {
        int d = w - h;
        vconcat(Mat(d / 2, w, CV_8UC1, Scalar(0)), src_bin, src_bin);
        vconcat(src_bin, Mat(d - d / 2, w, CV_8UC1, Scalar(0)), src_bin);
    }
}

string shash16(Mat src_bin)
{
    if (src_bin.empty())
        throw invalid_argument("shash16(): Image is empty");
    if (src_bin.channels() != 1)
        throw invalid_argument("shash16(): Require image in graystyle");

    resize(src_bin, src_bin, Size(16, 16));
    stringstream hash_value;
    uchar* pix = src_bin.data;
    int tmp_dec = 0;
    for (int ro = 0; ro < 256; ro++) {
        tmp_dec = tmp_dec << 1;
        if ((bool)*pix)
            tmp_dec++;
        if (ro % 4 == 3) {
            hash_value << hex << tmp_dec;
            tmp_dec = 0;
        }
        pix++;
    }
    return hash_value.str();
}

int hamming(string hash1_str, string hash2_str)
{
    stringstream hash1, hash2;
    hash1 << setw(64) << setfill('0') << hash1_str;
    hash2 << setw(64) << setfill('0') << hash2_str;
    hash1_str = hash1.str();
    hash2_str = hash2.str();
    int diff = 0;
    for (int i = 0; i < 64; i = i + 7) {
        int x = stoi(hash1_str.substr(i, 7), NULL, 16)
            ^ stoi(hash2_str.substr(i, 7), NULL, 16);
        while (x) {
            diff++;
            x = x & (x - 1);
        }
    }
    return diff;
}

class Result {
public:
    static auto analayse(Mat img, Result& result, bool ex);
    static Rect get_baseline_v(Mat img_bin);
    static Rect get_baseline_h(Mat img_bin, Rect baseline_v);
    static Rect get_baseline_h_ex(Mat img_bin, Rect baseline_v);
    static bool is_result(Mat img_bin, Rect baseline_v);
    static bool is_3stars(Mat img_bin, Rect baseline_v);

    // private:
    bool ex;
    bool valid;
    Mat img;
    Mat img_gray;
    Mat img_bin127;
    Mat img_bin230;
    Mat img_autobin;
    Rect baseline_v;
    Rect baseline_h;
    int item_diameter;
    string stage_code;
    dict droptypes;
    vector<dict> drops;
    dict drops_display;
    void get_stage();
    void get_droptypes();
    void get_drops();
};

auto Result::analayse(Mat img, Result& result, bool ex = false)
{
    if (img.empty())
        throw invalid_argument("analyse(): Image is empty");
    if (img.channels() != 3)
        throw invalid_argument("analyse(): Require image in BGR");

    Mat img_gray, img_bin127, img_autobin;
    cvtColor(img, img_gray, COLOR_BGR2GRAY);
    threshold(img_gray, img_bin127, 127, 255, THRESH_BINARY);
    Rect baseline_v = Result::get_baseline_v(img_bin127);
    adaptiveThreshold(img_gray, img_autobin, 255, ADAPTIVE_THRESH_MEAN_C,
        THRESH_BINARY, baseline_v.height / 2 * 2 + 1, 0);
    if (Result::is_result(img_bin127, baseline_v)) {
        if (Result::is_3stars(img_bin127, baseline_v)) {
            Rect baseline_h;
            if (ex)
                baseline_h = Result::get_baseline_h_ex(img_autobin, baseline_v);
            else
                baseline_h = Result::get_baseline_h(img_bin127, baseline_v);
            if (!baseline_h.empty()) {
                result.valid = true;
                result.ex = ex;
                result.img = img;
                result.img_gray = img_gray;
                result.img_bin127 = img_bin127;
                threshold(img_gray, result.img_bin230, 230, 255, THRESH_BINARY);
                result.img_autobin = img_autobin;
                result.baseline_v = baseline_v;
                result.baseline_h = baseline_h;
                result.item_diameter = round(baseline_v.height * 0.5238);

                result.get_stage();
                result.get_droptypes();
                result.get_drops();
                dict data = {
                    { "drops", result.drops },
                    { "stageId", stage_index[result.stage_code]["stageId"] }
                };
                auto display = tuple(result.stage_code, result.drops_display);
                return tuple(data, display);
            } else {
                result.valid = false;
                return tuple(dict(), tuple(string(), dict()));
            }
        } else {
            result.valid = false;
            return tuple(dict(), tuple(string(), dict()));
        }
    } else {
        result.valid = false;
        return tuple(dict(), tuple(string(), dict()));
    }
}

Rect Result::get_baseline_v(Mat img_bin)
{
    if (img_bin.empty())
        throw invalid_argument("get_baseline_v(): Image is empty");
    if (img_bin.channels() != 1)
        throw invalid_argument("get_baseline_v(): Require image in graystyle");

    // rotate(img_bin, img_bin, ROTATE_90_CLOCKWISE);

    int column = 0;
    int top = 0, bottom = 0;
    for (int co = img_bin.cols / 4; co < img_bin.cols / 2; co++) {
        uchar* pix = img_bin.data + (img_bin.rows - 1) * img_bin.step + co;
        int begin = 0, end = 0;
        bool prober[2] = { false, false };
        for (int ro = img_bin.rows; ro > img_bin.rows / 2; ro--) {
            prober[0] = prober[1];
            prober[1] = (bool)*pix;
            if (prober[0] == false && prober[1] == true)
                end = ro;
            if (prober[0] == true && prober[1] == false)
                begin = ro;
            if (begin && end)
                break;
            pix = pix - img_bin.step;
        }
        if ((begin && end) && (end - begin > bottom - top)) {
            column = co;
            top = begin;
            bottom = end;
        }
    }
    return Rect(column, top, 1, bottom - top);
}

Rect Result::get_baseline_h(Mat img_bin, Rect baseline_v)
{
    if (img_bin.empty())
        throw invalid_argument("get_baseline_h(): Image is empty");
    if (img_bin.channels() != 1)
        throw invalid_argument("get_baseline_h(): Require image in graystyle");

    int min_d = baseline_v.height / 2;
    int row = 0;
    int left = 0, right = 0;
    for (int ro = baseline_v.y + baseline_v.height - 1;
         ro > baseline_v.y + baseline_v.height / 2;
         ro--) {
        uchar* pix = img_bin.data + baseline_v.x + 5 + ro * img_bin.step;
        int begin = 0, end = 0;
        bool prober[2] = { false, false };
        for (int co = baseline_v.x + 5; co < img_bin.cols; co++) {
            prober[0] = prober[1];
            prober[1] = (bool)*pix;
            if (prober[0] == false && prober[1] == true)
                begin = co;
            if (prober[0] == true && prober[1] == false)
                end = co;
            if (begin && end)
                break;
            pix++;
        }
        if ((begin && end) && (end - begin > right - left)) {
            row = ro;
            left = begin;
            right = end;
        }
        if (prober[0] == false && prober[1] == false && right - left > min_d)
            break;
    }
    return Rect(left, row, right - left, 1);
}

Rect Result::get_baseline_h_ex(Mat img_bin, Rect baseline_v)
{
    if (img_bin.empty())
        throw invalid_argument("get_baseline_h(): Image is empty");
    if (img_bin.channels() != 1)
        throw invalid_argument("get_baseline_h(): Require image in graystyle");

    int min_d = baseline_v.height / 2;
    int row = 0;
    int count, maxcount = 0;
    for (int ro = baseline_v.y + baseline_v.height - 1;
         ro > baseline_v.y + 3 * baseline_v.height / 4;
         ro--) {
        uchar* pix = img_bin.data + baseline_v.x + 5 + ro * img_bin.step;
        count = 0;
        for (int co = baseline_v.x + 5; co < img_bin.cols; co++) {
            if ((bool)*pix)
                count++;
            pix++;
        }
        if (count > maxcount) {
            row = ro;
            maxcount = count;
        }
    }
    return Rect(baseline_v.x + 5, row, min_d, 1);
}

bool Result::is_result(Mat img_bin, Rect baseline_v)
{
    if (img_bin.empty())
        throw invalid_argument("is_result(): Image is empty");
    if (img_bin.channels() != 1)
        throw invalid_argument("is_result(): Require image in graystyle");
    Mat resultimg = img_bin(Rect(
        0, baseline_v.y + baseline_v.height / 2,
        baseline_v.x - 5, baseline_v.height / 2));
    resultimg = resultimg(boundingRect(resultimg));
    string hash = shash16(resultimg);
    int dist = hamming(hash, hash_index["result"]["zh"]);
    if (dist <= 25)
        return true;
    else
        return false;
}

bool Result::is_3stars(Mat img_bin, Rect baseline_v)
{
    if (img_bin.empty())
        throw invalid_argument("is_3stars(): Image is empty");
    if (img_bin.channels() != 1)
        throw invalid_argument("is_3stats(): Require image in graystyle");

    img_bin = img_bin(Rect(0, baseline_v.y, baseline_v.x - 5, baseline_v.height));
    vector<int> sp = separate(img_bin, RIGHT, 1, true);
    Mat star3 = img_bin(Rect(sp[0], 0, sp[1] - sp[0], baseline_v.height / 2));
    for (int ro = 0; ro < star3.rows; ro++) {
        uchar* pix = star3.data + ro * star3.step;
        for (int co = 0; co < star3.cols; co++) {
            if ((bool)*pix)
                return true;
            pix++;
        }
    }
    return false;
}

void Result::get_stage()
{
    auto get_char = [](Mat charimg) {
        if (charimg.empty())
            throw invalid_argument("get_stage(): Image is empty");

        string charhash = shash16(charimg);
        string chr;
        int diff = 64;
        for (auto& [kchar, vhash] : hash_index["stage"].items()) {
            int d = hamming(charhash, vhash);
            if (d < diff) {
                if (d == 0)
                    return kchar;
                chr = kchar;
                diff = d;
            }
        }
        return chr;
    };

    string stage = "";
    Mat stageimg = img_bin230(Rect(
        0, baseline_v.y,
        baseline_v.x - 5, baseline_v.height / 4));
    stageimg = stageimg(boundingRect(stageimg));
    vector sp = separate(stageimg, LEFT);
    if (!sp.empty()) {
        for (int i = 0; i < sp.size(); i = i + 2) {
            Mat charimg = stageimg(Rect(
                sp[i], 0, sp[i + 1] - sp[i], stageimg.rows));
            charimg = charimg(boundingRect(charimg));
            squarize(charimg);
            stage = stage + get_char(charimg);
        }
    }
    stage_code = stage;
}

void Result::get_droptypes()
{
    auto get_h = [](Scalar bgra) {
        double b = (bgra[0]) / 255;
        double g = (bgra[1]) / 255;
        double r = (bgra[2]) / 255;
        double cmax = std::max({ b, g, r });
        double cmin = std::min({ b, g, r });
        double delta = cmax - cmin;
        double h;
        if (delta < 1e-15)
            h = 0;
        else if (cmax == r)
            h = 60 * ((g - b) / delta);
        else if (cmax == g)
            h = 60 * ((b - r) / delta) + 120;
        else if (cmax = b)
            h = 60 * ((r - g) / delta) + 240;
        if (h < 0)
            h = h + 360;
        return round(h);
    };

    auto get_type = [this, get_h](Mat droplineimg) {
        string droptype;
        Scalar_<int> bgra = mean(droplineimg);
        int h = get_h(bgra);
        {
            int dist = 360;
            for (auto& [kh, vtype] : hsv_index.items()) {
                int d = abs(stoi(kh) - h);
                if (d < dist) {
                    dist = d;
                    droptype = vtype;
                }
            }
        }
        if (droptype == "SPECIAL_DROP" || droptype == "FURNITURE") {
            Size wholesize;
            Point upperleft;
            droplineimg.locateROI(wholesize, upperleft);
            Mat droptypeimg = img_gray(Rect(
                upperleft.x, upperleft.y + 1, droplineimg.cols / 2,
                baseline_v.y + baseline_v.height - upperleft.y));
            cvtColor(droplineimg, droplineimg, COLOR_BGR2GRAY);
            double minval, maxval;
            Point minloc, maxloc;
            minMaxLoc(droplineimg, &minval, &maxval, &minloc, &maxloc);
            threshold(droptypeimg, droptypeimg, maxval / 2, 255, THRESH_BINARY);
            droptypeimg = droptypeimg(boundingRect(droptypeimg));

            string hash_value = shash16(droptypeimg);
            int dist_special = hamming(hash_value,
                hash_index["droptype"]["zh"]["SPECIAL_DROP"]);
            int dist_furni = hamming(hash_value,
                hash_index["droptype"]["zh"]["FURNITURE"]);
            if (dist_special < dist_furni)
                droptype = "SPECIAL_DROP";
            else if (dist_furni < dist_special)
                droptype = "FURNITURE";
            else
                droptype = "UNDEFINED";
        }
        return droptype;
    };

    Mat img_bin;
    if (ex)
        img_bin = img_autobin;
    else
        img_bin = img_bin127;
    vector sp = separate(
        img_bin(Rect(baseline_v.x + 5, baseline_h.y,
            img.cols - (baseline_v.x + 5), 1)),
        LEFT, 0, true);
    { //fill the gap if it is smaller than threshold
        int i = 1;
        while (i + 1 < sp.size()) {
            if (sp[i + 1] - sp[i] < 5) {
                sp.erase(sp.begin() + i, sp.begin() + i + 2);
            } else
                i = i + 2;
        }
    }
    { //delete the line if it is smaller than threshold
        int i = 0;
        while (i < sp.size()) {
            if (sp[i + 1] - sp[i] < item_diameter) {
                sp.erase(sp.begin() + i, sp.begin() + i + 2);
            } else
                i = i + 2;
        }
    }

    int nodes = sp.size();
    if (nodes) {
        for (int i = 0; i < nodes; i = i + 2) {
            Mat droplineimg = img(Rect(
                sp[i], baseline_h.y, sp[i + 1] - sp[i], 1));
            string droptype = get_type(droplineimg);
            if (droptype == "FIRST") {
                valid = false;
                return;
            }
            droptypes[droptype] = { sp[i], sp[i + 1] };
        }
    }
}

void Result::get_drops()
{
    auto detect_item = [this](Mat& dropimg) {
        auto get_mask = [](Mat src) {
            Mat src_gray, mask;
            cvtColor(src, src_gray, COLOR_BGR2GRAY);
            threshold(src_gray, mask, 0, 255, THRESH_BINARY);
            cvtColor(mask, mask, COLOR_GRAY2BGR);
            return mask;
        };

        string itemId;
        double similarity = 0;
        Point upperleft;
        for (auto& item : stage_index[stage_code]["drops"].items()) {
            string itemid = item.value();

            Mat templ = templs[item_index[itemid]["img"]];
            double fx = (double)item_diameter / 163;
            resize(templ, templ, Size(), fx, fx);
            Mat mask = get_mask(templ);
            Mat resimg;
            matchTemplate(dropimg, templ, resimg, TM_CCORR_NORMED, mask);

            double minval, maxval;
            Point minloc, maxloc;
            minMaxLoc(resimg, &minval, &maxval, &minloc, &maxloc);
            if (maxval > similarity) {
                itemId = itemid;
                similarity = maxval;
                upperleft = maxloc;
            }
        }
        int w = round(183 * ((double)item_diameter / 163));
        dropimg = dropimg(Rect(upperleft.x, upperleft.y, w, w));
        return tuple(itemId, similarity);
    };
    auto detect_quantity = [](Mat dropimg) {
        auto get_char = [](Mat charimg) {
            if (charimg.empty())
                throw invalid_argument("detect_quantity(): Image is empty");

            string charhash = shash16(charimg);
            string chr;
            int diff = 64;
            for (auto& [kchar, vhash] : hash_index["dropimg"]["zh"].items()) {
                int d = hamming(charhash, vhash);
                if (d < diff) {
                    if (d == 0)
                        return kchar;
                    chr = kchar;
                    diff = d;
                }
            }
            return chr;
        };

        int height = dropimg.rows;
        int width = dropimg.cols;
        Mat qimg = dropimg(Rect(
            0, round(height * 0.7),
            round(width * 0.82), round(height * 0.15)));

        int digits;
        vector sp = separate(qimg, RIGHT);
        int edge = qimg.cols - sp[1];
        for (int i = 3; i < sp.size(); i = i + 2) {
            if (abs(sp[i] - sp[i - 3]) >= edge) {
                digits = i / 2;
                break;
            }
        }

        string quantity = "";
        for (int i = (digits - 1) * 2; i >= 0; i = i - 2) {
            Mat charimg = qimg(Rect(
                sp[i], 0, sp[i + 1] - sp[i], qimg.rows));
            squarize(charimg);
            quantity = quantity + get_char(charimg);
        }
        return quantity;
    };

    if (!droptypes.empty()) {
        for (auto& [ktype, vrange] : droptypes.items()) {
            if (ktype == "LMB")
                continue;
            else if (ktype == "FURNITURE") {
                drops.push_back({ { "dropType", ktype },
                    { "itemId", "furni" },
                    { "quantity", 1 } });
                drops_display[ktype] = { true };
                continue;
            } else
                drops_display[ktype] = {};

            int typerange[] = { (int)vrange[BEGIN], (int)vrange[END] };
            int typerange_len = typerange[END] - typerange[BEGIN];
            int count = typerange_len / item_diameter;
            for (int i = 0; i < count; i++) {
                int range[] = {
                    typerange[BEGIN] + i * typerange_len / count,
                    typerange[BEGIN] + (i + 1) * typerange_len / count
                };
                int offset = (range[END] - range[BEGIN] - item_diameter) / 3;
                Mat dropimg = img(
                    Range(baseline_v.y + baseline_v.height / 4, baseline_h.y),
                    Range(range[BEGIN], range[END] - offset));
                auto [itemId, similarity] = detect_item(dropimg);
                Mat dropimg_bin;
                cvtColor(dropimg, dropimg_bin, COLOR_BGR2GRAY);
                threshold(dropimg_bin, dropimg_bin, 127, 255, THRESH_BINARY);
                string quantity = detect_quantity(dropimg_bin);
                if (similarity > 0.9) {
                    drops.push_back({ { "dropType", ktype },
                        { "itemId", itemId },
                        { "quantity", quantity } });
                    string name = item_index[itemId]["name_i18n"]["zh"];
                    drops_display[ktype][name] = quantity;
                }
            }
        }
    }
}

void get_img(uint8_t* buffer, size_t size)
{
    Mat raw_data = Mat(1, size, CV_8UC1, buffer);
    cout << raw_data.empty() << endl;
    // imdecode(src, IMREAD_COLOR)
}

int main(int argc, char** argv)
{
    system("chcp 65001");
    preload();

    double T = 0;
    for (const auto& png : directory_iterator("D:\\Code\\arknights\\adb\\test")) {
        Mat img = imread(png.path().u8string());
        // Mat img = imread("D:\\Code\\arknights\\adb\\test\\hyda.jpg");
        Result result;
        auto s = getTickCount();
        auto [data, display] = Result::analayse(img, result);
        auto e = getTickCount();
        auto [stage, drops] = display;
        cout << stage << endl;
        for (auto& [ktype, vdrops] : drops.items()) {
            cout << ktype << "\t" << vdrops << endl;
        }
        double t = (e - s) / getTickFrequency() * 1000;
        T = T + t;
        cout << t << endl;
        cout << endl;
    }
    cout << T << endl;

    return 0;
}