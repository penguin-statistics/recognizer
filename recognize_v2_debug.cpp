#include "json.hpp"
#include "md5.hpp"
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

#define X 0
#define Y 1
#define WIDTH 2
#define HEIGHT 3

#define BEGIN 0
#define END 1

string server;
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

dict droptype_order = {
    { "FIRST", 0 },
    { "LMB", 1 },
    { "FURNITURE", 2 },
    { "NORMAL_DROP", 3 },
    { "SPECIAL_DROP", 4 },
    { "EXTRA_DROP", 5 }
};

//**********DEBUG TOOLS**********

void show_img(Mat src)
{
    if (src.rows > 600) {
        double fx = 600.0 / src.rows;
        resize(src, src,
            Size(), fx, fx, INTER_AREA);
    }
    namedWindow("Display window", WINDOW_AUTOSIZE);
    imshow("Display window", src);
    waitKey(0);
    destroyWindow("Display window");
}

Rect get_rect(dict box)
{
    return Rect(box[X], box[Y], box[WIDTH], box[HEIGHT]);
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

vector<int> separate(Mat src_bin, int direc, int n = 0, bool roi = false)
{
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
    resize(src_bin, src_bin, Size(16, 16), 0, 0);
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

int hamming(std::string hash1, std::string hash2)
{
    hash1.insert(hash1.begin(), 64 - hash1.size(), '0');
    hash2.insert(hash2.begin(), 64 - hash2.size(), '0');
    int dist = 0;
    for (int i = 0; i < 64; i = i + 16) {
        size_t x = strtoull(hash1.substr(i, 16).c_str(), NULL, 16)
            ^ strtoull(hash2.substr(i, 16).c_str(), NULL, 16);
        while (x) {
            dist++;
            x = x & (x - 1);
        }
    }
    return dist;
}

string md5(Mat img)
{
    MD5 hash;
    return hash(img.data, img.step * img.rows);
}

class AnalyzeContext {
    friend class Analyzer;

private:
    bool valid = true;
    Mat img_debug;
    Mat img;
    Mat img_small;
    double coeff;
    Mat img_gray;
    int maxgraypix = 255;
    Mat img_bin127;
    Mat img_bin230;
    Mat img_autobin;
    Rect baseline_v;
    Rect baseline_h;
    int item_diameter;

    string stage_code = "";
    string fingerprint = "";
    dict droptypes;
    vector<dict> drops;
    dict rectangles;
    dict validation = {
        { "warnings", dict::array() },
        { "errors", dict::array() }
    };
};

class AnalyzeResult {
    friend class Analyzer;

public:
    Mat img_debug;
    string report();

private:
    string hash_md5 = "";
    string stage_code = "";
    string fingerprint = "";
    vector<dict> drops;
    dict validation = {
        { "warnings", dict::array() },
        { "errors", dict::array() }
    };
};

string AnalyzeResult::report()
{
    string stageId = "";
    if (stage_index.contains(stage_code))
        stageId = stage_index[stage_code]["stageId"];
    dict report = {
        { "drops", drops },
        { "stageId", stageId },
        { "fingerprint", fingerprint },
        { "md5", hash_md5 },
        { "errors", validation["errors"] },
        { "warnings", validation["warnings"] }
    };
    return report.dump();
}

class Analyzer {
public:
    AnalyzeResult analyze(Mat img, bool fallback);
    stringstream output;

private:
    AnalyzeContext context;
    Rect get_baseline_v();
    Rect get_baseline_h();
    bool is_result();
    bool is_3stars();
    string get_stage();
    void get_droptypes();
    void get_drops();
    void get_fingerprint();
};

AnalyzeResult Analyzer::analyze(Mat img, bool fallback = false)
{
    AnalyzeResult result;
    auto make_result = [this, &result]() {
        result.img_debug = context.img_debug;
        result.stage_code = context.stage_code;
        result.fingerprint = context.fingerprint;
        result.drops = context.drops;
        result.validation = context.validation;
    };

    context.img = img;

    output << "Image resolution: " << img.size << endl;
    output << "Analysing..." << endl;

    context.img_debug = img.clone();

    // img_gray
    cvtColor(img, context.img_gray, COLOR_BGR2GRAY);
    // img_bin127
    threshold(context.img_gray, context.img_bin127, 127, 255, THRESH_BINARY);

    // baseline_v
    output << "Getting baseline_v...";
    context.baseline_v = get_baseline_v();
    output << "\t" << context.baseline_v << endl;
    context.item_diameter = round(context.baseline_v.height * 0.5238);
    int k;
    if (fallback) {
        k = 1;
        context.coeff = 1.0;
        context.img_small = img;
    } else {
        k = round(context.baseline_v.height / 80.0);
        if (k != 0) {
            context.coeff = 1.0 / k;
            resize(img, context.img_small, Size(), context.coeff, context.coeff, INTER_AREA);
        }
    }
    output << "Shrink coefficient: " << k << endl;

    output << "Is Result...";
    if (is_result()) {
        // img_bin230
        threshold(context.img_gray, context.img_bin230,
            context.maxgraypix * 0.9, 255, THRESH_BINARY);
        // get_stage
        output << "Getting stage...";
        context.stage_code = get_stage();
        {
            // ********** EXPERIMENTAL **********
            // Fix some wrong results like "O-7"
            // May add regex
            if (context.stage_code.substr(0, 2) == "O-") {
                string stage_code_origin = context.stage_code;
                string stage_code_fixed = stage_code_origin.replace(0, 1, "0");
                output << "\t" << stage_code_fixed << endl;
                output << "\t[**WARNING**: Stage::NotFound] "
                       << "Automatically fixed: "
                       << "[" << context.stage_code << "] -> "
                       << "[" << stage_code_fixed << "]" << endl;
                context.validation["warnings"].push_back(
                    { { "type", "Stage::NotFound" },
                        { "original", context.stage_code },
                        { "corrected", stage_code_fixed } });
                context.stage_code = stage_code_fixed;
            }
            // **********************************
            else
                output << "\t" << context.stage_code << endl;
        }
        if (!stage_index.contains(context.stage_code)) {
            output << "\t[**ERROR**: Stage::NotFound]";
            context.valid = false;
            context.validation["errors"].push_back(
                { { "type", "Stage::NotFound" } });
            make_result();
            return result;
        }

        output << "Is 3stars...";
        if (is_3stars()) {
            output << "\t\tYES" << endl;
            // img_autobin
            adaptiveThreshold(context.img_gray, context.img_autobin,
                255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY,
                context.baseline_v.height / 2 * 2 + 1, -20);

            output << "Getting baseline_h...";
            context.baseline_h = get_baseline_h();
            if (!(context.baseline_h.y == 0)) {
                output << "\t" << context.baseline_h << endl;
                context.item_diameter = round(context.baseline_v.height * 0.5238);

                output << "Getting droptypes..." << endl;
                get_droptypes();
                if (!context.valid) {
                    make_result();
                    return result;
                }

                output << "Getting drops...";
                get_drops();
                result.drops = context.drops;
            } else {
                output << "\tFAIL" << endl;
                context.valid = false;
                context.validation["errors"].push_back(
                    { { "type", "Baseline_h::NotFound" } });
                make_result();
                return result;
            }
        } else {
            output << "\t\tNO" << endl;
            context.valid = false;
            context.validation["errors"].push_back(
                { { "type", "3stars::False" } });
            make_result();
            return result;
        }
    } else {
        context.valid = false;
        context.validation["errors"].push_back(
            { { "type", "Result::False" } });
        make_result();
        return result;
    }

    get_fingerprint();
    result.hash_md5 = md5(context.img_small);
    make_result();
    return result;
}

Rect Analyzer::get_baseline_v()
{
    Mat img_bin = context.img_bin127;
    int column = 0;
    int top = 0, bottom = 0;

    int _start = img_bin.cols / 4;
    for (int co = 0; co < img_bin.cols / 2; co++) {
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
    context.rectangles["baseline_v"] = { column, top, 1, bottom - top };
    rectangle(context.img_debug, get_rect(context.rectangles["baseline_v"]),
        Scalar(0, 0, 255), 2);
    return Rect(column, top, 1, bottom - top);
}

Rect Analyzer::get_baseline_h()
{
    Mat img_bin = context.img_autobin;
    Rect baseline_v = context.baseline_v;

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

bool Analyzer::is_result()
{
    Mat img_gray = context.img_gray;
    Mat img_bin = context.img_bin127;
    Rect baseline_v = context.baseline_v;
    int result_thresh = 25;

    Rect rect_result = Rect(
        0, baseline_v.y + baseline_v.height / 2,
        baseline_v.x - 5, baseline_v.height / 2);

    double minval, maxval;
    Point minloc, maxloc;
    minMaxLoc(img_gray, &minval, &maxval, &minloc, &maxloc);
    context.maxgraypix = maxval;

    Mat resultimg = img_bin(rect_result);

    rect_result = boundingRect(resultimg);
    context.rectangles["result"] = {
        rect_result.x, rect_result.y + baseline_v.y + baseline_v.height / 2,
        rect_result.width, rect_result.height
    };
    rectangle(context.img_debug, get_rect(context.rectangles["result"]),
        Scalar(0, 0, 255), 2);

    resultimg = resultimg(rect_result);
    string hash_value;
    if (!resultimg.empty())
        hash_value = shash16(resultimg);
    else
        return false;
    int dist = hamming(hash_value, hash_index["result"][server]);
    if (dist <= result_thresh)
        output << "\t\tYES" << endl;
    else
        output << "\t\tNO" << endl;
    output << "\tHamming distance: " << dist << endl;
    output << "\tHash value: " << hash_value << endl;
    if (dist <= result_thresh)
        return true;
    else
        return false;
}

bool Analyzer::is_3stars()
{
    Mat img_bin = context.img_bin127;
    Rect baseline_v = context.baseline_v;
    dict rectangles = context.rectangles;

    int stage_right_edge
        = (int)rectangles["stage"][X] + (int)rectangles["stage"][WIDTH] + 5;
    Mat starsimg = img_bin(Rect(
        stage_right_edge, baseline_v.y,
        baseline_v.x - 5 - stage_right_edge, baseline_v.height / 4));
    vector sp = separate(starsimg, LEFT);

    Rect rect_stars = boundingRect(starsimg);
    rect_stars.x += stage_right_edge;
    rect_stars.y += baseline_v.y;
    rect_stars.height += baseline_v.height / 4;
    starsimg = img_bin(rect_stars);
    starsimg = starsimg(boundingRect(starsimg));

    Size wholesize;
    Point upperleft;
    starsimg.locateROI(wholesize, upperleft);
    context.rectangles["stars"] = {
        upperleft.x, upperleft.y,
        starsimg.cols, starsimg.rows
    };
    rectangle(context.img_debug, get_rect(context.rectangles["stars"]),
        Scalar(0, 0, 255), 2);

    if (sp.size() == 6)
        return true;
    else
        return false;
}

string Analyzer::get_stage()
{
    Mat img_bin230 = context.img_bin230;
    Rect baseline_v = context.baseline_v;
    int right_edge = baseline_v.x / 2;
    auto get_char = [](Mat charimg) {
        string charhash = shash16(charimg);
        string chr;
        int dist = 256;
        for (auto& [kchar, vhash] : hash_index["stage"].items()) {
            int d = hamming(charhash, vhash);
            if (d < dist) {
                if (d == 0)
                    return kchar;
                chr = kchar;
                dist = d;
            }
        }
        return chr;
    };

    string stage = "";
    Mat stageimg = img_bin230(Rect(
        0, baseline_v.y,
        right_edge, baseline_v.height / 4));

    Rect rect_stage = boundingRect(stageimg);
    context.rectangles["stage"] = {
        rect_stage.x, rect_stage.y + baseline_v.y,
        rect_stage.width, rect_stage.height
    };
    stageimg = stageimg(rect_stage);
    rectangle(context.img_debug, get_rect(context.rectangles["stage"]),
        Scalar(0, 0, 255), 2);
    rect_stage.y--;

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
    context.stage_code = stage;
    return stage;
}

void Analyzer::get_droptypes()
{
    auto get_hsv = [](Scalar bgra) {
        double b = (bgra[0]) / 255;
        double g = (bgra[1]) / 255;
        double r = (bgra[2]) / 255;
        double cmax = std::max({ b, g, r });
        double cmin = std::min({ b, g, r });
        double delta = cmax - cmin;
        double h, s, v;
        if (delta < 1e-15)
            h = 0;
        else if (cmax == r)
            h = 60 * ((g - b) / delta);
        else if (cmax == g)
            h = 60 * ((b - r) / delta) + 120;
        else if (cmax == b)
            h = 60 * ((r - g) / delta) + 240;
        if (h < 0)
            h = h + 360;
        if (cmax == 0)
            s = 0;
        else
            s = delta / cmax;
        v = cmax;
        return tuple(h, s, v);
    };

    auto get_type = [this, get_hsv](Mat droplineimg) {
        Rect baseline_v = context.baseline_v;
        string droptype;
        Scalar_<int> bgra = mean(droplineimg);

        auto [h, s, v] = get_hsv(bgra);
        if (s < 0.10)
            droptype = "NORMAL_DROP";
        else {
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
            Mat droptypeimg;
            if (server == "CN" || server == "KR") {
                droptypeimg
                    = context.img_gray(
                                 Rect(upperleft.x, upperleft.y + 1, droplineimg.cols / 2,
                                     baseline_v.y + baseline_v.height - upperleft.y))
                          .clone();
            } else {
                droptypeimg
                    = context.img_gray(
                                 Rect(upperleft.x, upperleft.y + 1, droplineimg.cols,
                                     baseline_v.y + baseline_v.height - upperleft.y))
                          .clone();
            }
            cvtColor(droplineimg, droplineimg, COLOR_BGR2GRAY);
            double minval, maxval;
            Point minloc, maxloc;
            minMaxLoc(droplineimg, &minval, &maxval, &minloc, &maxloc);
            threshold(droptypeimg, droptypeimg, maxval / 2, 255, THRESH_BINARY);
            bool overline = true;
            while (overline) {
                uchar* pix = droptypeimg.data;
                int count = 0;
                for (int co = 0; co < droptypeimg.cols; co++) {
                    if ((bool)*pix) {
                        count++;
                    }
                    pix++;
                }
                if (count > droptypeimg.cols / 2) {
                    droptypeimg = droptypeimg(Rect(
                        0, 1, droptypeimg.cols, droptypeimg.rows - 1));
                } else
                    overline = false;
            }
            droptypeimg = droptypeimg(boundingRect(droptypeimg));

            string hash_value = shash16(droptypeimg);
            int dist_special = hamming(hash_value,
                hash_index["droptype"][server]["SPECIAL_DROP"]);
            int dist_furni = hamming(hash_value,
                hash_index["droptype"][server]["FURNITURE"]);
            if (dist_special < dist_furni)
                droptype = "SPECIAL_DROP";
            else if (dist_furni < dist_special)
                droptype = "FURNITURE";
            else
                droptype = "UNDEFINED";

            output << "\t" << droptype.substr(0, 3) << "     ";
            output << "HSV [" << setw(2) << round(h) << ", ";
            output << fixed << setprecision(2);
            output << s << ", " << v << "]" << endl;
            output << defaultfloat << setprecision(6);
            output << "\t\tHamming distance (SPE): " << dist_special << endl;
            output << "\t\tHamming distance (FUR): " << dist_furni << endl;
            output << "\t\tHash value: " << hash_value << endl;
        } else {
            output << "\t" << droptype.substr(0, 3) << "     ";
            output << "HSV [" << setw(2) << round(h) << ", ";
            output << fixed << setprecision(2);
            output << s << ", " << v << "]" << endl;
            output << defaultfloat << setprecision(6);
        }
        return droptype;
    };

    Mat img = context.img;
    Mat img_bin = context.img_autobin;
    Rect baseline_v = context.baseline_v;
    Rect baseline_h = context.baseline_h;
    int item_diameter = context.item_diameter;

    vector<int> sp;

    sp = separate(
        img_bin(Rect(baseline_v.x + 5, baseline_h.y,
            img_bin.cols - (baseline_v.x + 5), 1)),
        LEFT, 0, true);
    { // fill the gap if it is smaller than threshold
        int i = 1;
        while (i + 1 < sp.size()) {
            if (sp[i + 1] - sp[i] < 5)
                sp.erase(sp.begin() + i, sp.begin() + i + 2);
            else
                i = i + 2;
        }
    }
    { // delete the line if it is smaller than threshold
        int i = 0;
        while (i < sp.size()) {
            if (sp[i + 1] - sp[i] < item_diameter)
                sp.erase(sp.begin() + i, sp.begin() + i + 2);
            else
                i = i + 2;
        }
    }
    { // predict the line if there is a gap > item diameter
        int i = 1;
        while (i + 1 < sp.size()) {
            if (sp[i + 1] - sp[i] > item_diameter) {
                int midpoint = (sp[i] + sp[i + 1]) / 2;
                int count = (sp[i + 2] - sp[i + 1]) / item_diameter;
                int length = (sp[i + 2] - sp[i + 1]) / count;
                sp.insert(sp.begin() + i + 1, midpoint - length / 2);
                sp.insert(sp.begin() + i + 2, midpoint + length / 2);
            }
            i = i + 2;
        }
    }

    int nodes = sp.size();
    if (nodes) {
        for (int i = 0; i < nodes; i = i + 2) {
            Mat droplineimg = img(Rect(
                sp[i], baseline_h.y, sp[i + 1] - sp[i], 1));
            string droptype = get_type(droplineimg);

            if (i == 0)
                sp[i] = baseline_v.x + round(baseline_v.height * 0.21);
            Rect rect_droptype = Rect(
                sp[i], baseline_h.y, (sp[i + 1] - sp[i]), 1);
            rectangle(context.img_debug, rect_droptype, Scalar(0, 0, 255));

            if (droptype == "FIRST") {
                output << "\t[**ERROR**: Droptype::First]" << endl;
                context.valid = false;
                context.validation["errors"].push_back(
                    { { "type", "Droptype::First" } });
                return;
            }
            if (droptype == "UNDEFINED") {
                output << "\t[**ERROR**: Droptype::Undefined]" << endl;
                context.valid = false;
                context.validation["errors"].push_back(
                    { { "type", "Droptype::Undefined" } });
                return;
            }
            context.droptypes.push_back({ droptype, { sp[i], sp[i + 1] } });
        }
        int lasttype = -1;
        for (auto& [idx, droptype] : context.droptypes.items()) {
            int currenttype = droptype_order[(string)droptype[0]];
            if (currenttype <= lasttype) {
                // ********** EXPERIMENTAL **********
                {
                    if (lasttype == 5) {
                        context.droptypes[stoi(idx) - 1][0] = "LMB";
                        lasttype = currenttype;
                        output << "\t[**WARNING**: Droptype::WrongOrder] ";
                        output << "Automatically fixed: [EXTRA_DROP] -> [LMB]" << endl;
                        context.validation["warnings"].push_back(
                            { { "type", "Droptype::WrongOrder" } });
                        continue;
                    }
                }
                // **********************************
                output << "\t[**ERROR**: Droptype::WrongOrder]" << endl;
                context.valid = false;
                context.validation["errors"].push_back(
                    { { "type", "Droptype::WrongOrder" } });
                return;
            } else
                lasttype = currenttype;
        }
        for (auto& droptype : context.droptypes) {
            string ktype = droptype[0];
            auto vrange = droptype[1];

            putText(context.img_debug, ktype.substr(0, 3),
                Point(vrange[BEGIN], baseline_h.y + 12),
                FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 255));
        }
    } else {
        output << "\t[**ERROR**: Droptype::NotFound]" << endl;
        context.valid = false;
        context.validation["errors"].push_back(
            { { "type", "Droptype::NotFound" } });
        return;
    }
}

void Analyzer::get_drops()
{
    auto detect_item = [this](Mat dropimg) {
        auto get_mask = [](Mat src) {
            Mat src_gray, mask;
            cvtColor(src, src_gray, COLOR_BGR2GRAY);
            threshold(src_gray, mask, 25, 255, THRESH_BINARY);
            return mask;
        };

        Rect rect_drop;
        {
            Size wholesize;
            Point upperleft;
            dropimg.locateROI(wholesize, upperleft);
            rect_drop = Rect(upperleft, dropimg.size());
        }

        double coeff = context.coeff;
        resize(dropimg, dropimg, Size(), coeff, coeff);

        double item_diameter = context.item_diameter;
        string itemId = "";
        double similarity = 0;
        double similarity2 = 0;
        Point offset;
        if (stage_index[context.stage_code]["drops"].empty()) {
            output << "\t[**ERROR**: Drop::IllegalDrop]" << endl;
            context.valid = false;
            context.validation["errors"].push_back(
                { { "type", "Drop::IllegalDrop" } });
            return tuple(itemId, similarity);
        }

        for (auto& item : stage_index[context.stage_code]["drops"].items()) {
            string itemid = item.value();

            Mat templ = templs[item_index[itemid]["img"]];
            double fx = item_diameter * coeff / 163;
            resize(templ, templ, Size(), fx, fx, INTER_AREA);
            Mat mask = get_mask(templ);
            Mat resimg;
            matchTemplate(dropimg, templ, resimg, TM_CCORR_NORMED, mask);

            double minval, maxval;
            Point minloc, maxloc;
            minMaxLoc(resimg, &minval, &maxval, &minloc, &maxloc);
            if (maxval > similarity) {
                itemId = itemid;
                similarity2 = similarity;
                similarity = maxval;
                offset = maxloc;
            }
            if (maxval > similarity2 && maxval < similarity)
                similarity2 = maxval;
        }

        rect_drop.x += offset.x / coeff;
        rect_drop.y += offset.y / coeff;
        rect_drop.width = rect_drop.height = round(183 * (item_diameter / 163));

        context.rectangles["drops"].push_back(
            { rect_drop.x, rect_drop.y, rect_drop.width, rect_drop.height });
        rectangle(context.img_debug, rect_drop, Scalar(0, 0, 255), 2);

        if (similarity > 0.9) {
            string name = item_index[itemId]["name_i18n"]["zh"];
            output << "\t" << setw(5) << itemId << "\t" << name;
            if (name.size() < 12)
                output << "\t";
            output << "\tSimilarity: " << similarity * 100 << "%";
            output << "\tSecond Similarity: " << similarity2 * 100 << "%" << endl;
        } else {
            itemId = "00000";
            output << "\t00000\tUnknown\t\t"
                   << "Similarity: " << similarity * 100 << "%" << endl;
            output << "\t\t[**ERROR**: Drop::LowConfidence]" << endl;
            context.valid = false;
            context.validation["errors"].push_back(
                { { "type", "Drop::LowConfidence" },
                    { "rect", context.rectangles["drops"].back() } });
        }
        return tuple(itemId, similarity);
    };

    auto detect_quantity = [this](Mat dropimg) {
        bool confidence = true;
        auto get_char = [this, &confidence](Mat charimg) {
            string charhash = shash16(charimg);
            string chr, chr2;
            int dist = 256;
            int dist2 = 256;
            for (auto& [kchar, vhash] : hash_index["item"][server].items()) {
                int d = hamming(charhash, vhash);
                if (d < dist) {
                    if (d == 0)
                        return kchar;
                    chr = kchar;
                    dist = d;
                }
                if (d < dist2 && d > dist) {
                    chr2 = kchar;
                    dist2 = d;
                }
            }
            if (dist >= 64)
                confidence = false;
            output << "\t\t[" << chr << "]"
                   << "\t\tHamming distance: " << dist
                   << "\tSecond char and distance: "
                   << "[" << chr2 << "] " << dist2 << endl;
            output << "\t\tHash value: " << charhash << endl;
            return chr;
        };

        Point offset = get_rect(context.rectangles["drops"].back()).tl();
        Rect baseline_v = context.baseline_v;
        Mat qimg = dropimg(Rect(
            0, round(baseline_v.y + baseline_v.height * 0.66) - offset.y,
            round(dropimg.cols * 0.82), round(baseline_v.height * 0.10)));
        offset.y = round(baseline_v.y + baseline_v.height * 0.66);

        vector sp = separate(qimg, RIGHT);

        string quantity = "";
        int nodes = sp.size();
        for (int i = 0; i < nodes; i = i + 2) {
            if (quantity.size() > 0) {
                if (sp[i - 2] - sp[i + 1] > baseline_v.height * 0.04)
                    break;
            }
            Mat charimg = qimg(Rect(
                sp[i], 0, sp[i + 1] - sp[i], qimg.rows));
            if (charimg.cols > charimg.rows) {
                if (quantity.size() > 0)
                    break;
                else
                    continue;
            }
            {
                Mat _;
                Mat1i stats;
                Mat1d centroids;
                int ccomps = connectedComponentsWithStats(
                    charimg, _, stats, centroids);
                if (ccomps - 1 != 1) {
                    if (quantity.size() > 0)
                        break;
                    else
                        continue;
                }
                if (stats(1, CC_STAT_WIDTH) > stats(1, CC_STAT_HEIGHT)) {
                    if (quantity.size() > 0)
                        break;
                    else
                        continue;
                }
                double charimg_area = charimg.cols * charimg.rows;
                if (stats(1, CC_STAT_AREA) / charimg_area < 0.15) {
                    if (quantity.size() > 0)
                        break;
                    else
                        continue;
                }
            }
            {
                bool valid = true;
                int rows = charimg.rows;
                int step = charimg.step;
                for (int co = 0; co < charimg.cols; co++) {
                    uchar* pix = charimg.data;
                    if ((bool)*(pix + co)
                        || (bool)*(pix + co + (rows - 1) * step)) {
                        valid = false;
                        break;
                    }
                }
                if (!valid) {
                    if (quantity.size() > 0)
                        break;
                    else
                        continue;
                }
            }

            charimg = charimg(boundingRect(charimg));
            squarize(charimg);
            quantity = quantity + get_char(charimg);

            rectangle(context.img_debug,
                Rect(sp[i] + offset.x, offset.y, sp[i + 1] - sp[i], qimg.rows),
                Scalar(0, 0, 255));
        }
        reverse(quantity.begin(), quantity.end());

        if (!confidence) {
            context.validation["warnings"].push_back(
                { { "type", "DropQuantity::LowConfidence" },
                    { "rect", context.rectangles["drops"].back() } });
        }
        if (quantity.size() == 0) {
            context.valid = false;
            context.validation["errors"].push_back(
                { { "type", "DropQuantity::NotFound" },
                    { "rect", context.rectangles["drops"].back() } });
        }
        return quantity;
    };

    dict droptypes = context.droptypes;
    Mat img = context.img;
    Mat img_small = context.img_small;
    double coeff = context.coeff;
    Rect baseline_v = context.baseline_v;
    Rect baseline_h = context.baseline_h;
    int item_diameter = context.item_diameter;

    if (!droptypes.empty()) {
        for (auto& droptype : droptypes) {
            auto ktype = droptype[0];
            auto vrange = droptype[1];
            if (ktype == "LMB")
                continue;
            else if (ktype == "FURNITURE") {
                output << "\n\t[FURNITURE]" << endl;
                output << "\tDetected" << endl;
                context.drops.push_back({ { "dropType", ktype },
                    { "itemId", "furni" },
                    { "quantity", 1 } });
                continue;
            } else {
                output << "\n\t[" << ktype << "]" << endl;
            }

            int typerange[] = { (int)vrange[BEGIN], (int)vrange[END] };
            int typerange_len = typerange[END] - typerange[BEGIN];
            int count = typerange_len / item_diameter;
            for (int i = 0; i < count; i++) {
                int range[] = {
                    typerange[BEGIN] + i * typerange_len / count,
                    typerange[BEGIN] + (i + 1) * typerange_len / count
                };
                Mat dropimg = img(
                    Range(baseline_v.y + baseline_v.height / 4, baseline_h.y),
                    Range(range[BEGIN], range[END]));
                auto [itemId, similarity] = detect_item(dropimg);

                if (itemId.empty())
                    continue;
                else if (itemId == "00000")
                    itemId = "";

                Mat dropimg_bin = context.img_bin127(
                    get_rect(context.rectangles["drops"].back()));
                string quantity = detect_quantity(dropimg_bin);

                context.drops.push_back({ { "dropType", ktype },
                    { "itemId", itemId },
                    { "quantity", quantity },
                    { "confidence", to_string(similarity) } });
            }
        }
    }
}

void Analyzer::get_fingerprint()
{
    Mat img_bin = context.img_gray;
    Rect baseline_v = context.baseline_v;
    resize(img_bin, img_bin, Size(8, 8));
    uchar* pix = img_bin.data;
    stringstream fp;
    for (int i = 0; i < 64; i++) {
        fp << setw(2) << setfill('0') << hex << (int)*pix;
        pix++;
    }
    context.fingerprint = fp.str();
}

double T = 0;

AnalyzeResult recognize(Mat img)
{
    Analyzer analyzer;
    AnalyzeResult result;
    int64 start, end;
    try {
        start = getTickCount();
        result = analyzer.analyze(img);
        end = getTickCount();
        cout << analyzer.output.str() << endl;
    } catch (const std::exception& e) {
        end = getTickCount();
        cout << analyzer.output.str() << endl;
        cout << "\n\n";
        cerr << "[**ERROR**: Unexpected Exception] " << e.what() << '\n';
        show_img(img);
    }

    auto t = (end - start) / getTickFrequency() * 1000;
    T = T + t;
    cout << "Time cost: " << t << endl;
    cout << "\n"
         << result.report() << endl;
    return result;
};

int main(int argc, char** argv)
{
    system("chcp 65001");
    system("cls");

    preload();
    server = "CN";

    string path_ = "err";

    if (is_directory(path_)) {
        for (const auto& png : directory_iterator(path_)) {
            Mat img = imread(png.path().u8string());
            cout << "File name: " << png.path().filename() << endl;
            AnalyzeResult result;
            result = recognize(img);
            for (int i = 0; i < 100; i++) {
                cout << "_";
            }
            cout << endl;
            if (!result.img_debug.empty())
                show_img(result.img_debug);
            else
                system("pause");
        }
        cout << "Total time cost: " << T << endl;
    } else if (is_regular_file(path_)) {
        Mat img = imread(path_);
        path p(path_);
        cout << "File name: " << p.filename() << endl;
        AnalyzeResult result;
        result = recognize(img);
        for (int i = 0; i < 100; i++) {
            cout << "_";
        }
        cout << endl;
        if (!result.img_debug.empty())
            show_img(result.img_debug);
        else
            system("pause");
    }
    return 0;
}