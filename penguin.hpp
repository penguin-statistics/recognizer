#ifndef PENGUIN_HPP_
#define PENGUIN_HPP_

#include "json.hpp"
// #include <iomanip>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <set>
using dict = nlohmann::json;

void show_img(cv::Mat src)
{
    if (src.rows > 600) {
        double fx = 600.0 / src.rows;
        cv::resize(src, src,
            cv::Size(), fx, fx, cv::INTER_AREA);
    }
    cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);
    cv::imshow("Display window", src);
    cv::waitKey(0);
    cv::destroyWindow("Display window");
}

namespace penguin { // const
const int TEMPLATE_WIDTH = 183;
const int TEMPLATE_HEIGHT = 183;
const int TEMPLATE_DIAMETER = 163;
const int ITEM_RESIZED_WIDTH = 50;
} // namespace penguin

namespace penguin { // extern variable
std::string server;
dict item_index, hash_index;
std::map<std::string, cv::Mat> item_templs;
} // namespace penguin

namespace penguin { // tools function
enum DirectionFlags {
    TOP = 0,
    BOTTOM = 1,
    LEFT = 2,
    RIGHT = 3
};

enum RangeFlags {
    BEGIN = 0,
    END = 1
};
std::list<cv::Range> separate(const cv::Mat& src_bin, DirectionFlags direc, int n = 0)
{
    std::list<cv::Range> sp;
    bool isodd = false;
    uint begin;
    if (direc == TOP) {
        for (uint ro = 0; ro < src_bin.rows; ro++) {
            uchar* pix = src_bin.data + ro * src_bin.step;
            uint co = 0;
            for (; co < src_bin.cols; co++) {
                if ((bool)*pix && !isodd) {
                    begin = ro;
                    isodd = !isodd;
                    break;
                } else if ((bool)*pix && isodd)
                    break;
                pix++;
            }
            if (co == src_bin.cols && isodd) {
                uint end = ro;
                sp.push_back(cv::Range(begin, end));
                isodd = !isodd;
                if (sp.size() == n)
                    break;
            }
        }
        if (isodd) {
            uint end = src_bin.rows;
            sp.push_back(cv::Range(begin, end));
        }
    } else if (direc == BOTTOM) {
        for (uint ro = src_bin.rows - 1; ro >= 0; ro--) {
            uchar* pix = src_bin.data + ro * src_bin.step;
            uint co = 0;
            for (; co < src_bin.cols; co++) {
                if ((bool)*pix && !isodd) {
                    begin = ro + 1;
                    isodd = !isodd;
                    break;
                } else if ((bool)*pix && isodd)
                    break;
                pix++;
            }
            if (co == src_bin.cols && isodd) {
                uint end = ro + 1;
                sp.push_back(cv::Range(begin, end));
                isodd = !isodd;
                if (sp.size() == n)
                    break;
            }
        }
        if (isodd) {
            uint end = 0;
            sp.push_back(cv::Range(begin, end));
        }
    } else if (direc == LEFT) {
        for (uint co = 0; co < src_bin.cols; co++) {
            uchar* pix = src_bin.data + co;
            uint ro = 0;
            for (; ro < src_bin.rows; ro++) {
                if ((bool)*pix && !isodd) {
                    begin = co;
                    isodd = !isodd;
                    break;
                } else if ((bool)*pix && isodd)
                    break;
                pix = pix + src_bin.step;
            }
            if (ro == src_bin.rows && isodd) {
                uint end = co;
                sp.push_back(cv::Range(begin, end));
                isodd = !isodd;
                if (sp.size() == 2 * n)
                    break;
            }
        }
        if (isodd) {
            uint end = src_bin.cols;
            sp.push_back(cv::Range(begin, end));
        }
    } else if (direc == RIGHT) {
        for (uint co = src_bin.cols - 1; co >= 0; co--) {
            uchar* pix = src_bin.data + co;
            uint ro = 0;
            for (; ro < src_bin.rows; ro++) {
                if ((bool)*pix && !isodd) {
                    begin = co + 1;
                    isodd = !isodd;
                    break;
                } else if ((bool)*pix && isodd)
                    break;
                pix = pix + src_bin.step;
            }
            if (ro == src_bin.rows && isodd) {
                uint end = co + 1;
                sp.push_back(cv::Range(begin, end));
                isodd = !isodd;
                if (sp.size() == 2 * n)
                    break;
            }
        }
        if (isodd) {
            uint end = 0;
            sp.push_back(cv::Range(begin, end));
        }
    }
    return sp;
}

void squarize(cv::Mat& src_bin)
{
    const int h = src_bin.rows, w = src_bin.cols;
    if (h > w) {
        int d = h - w;
        hconcat(cv::Mat(h, d / 2, CV_8UC1, cv::Scalar(0)), src_bin, src_bin);
        hconcat(src_bin, cv::Mat(h, d - d / 2, CV_8UC1, cv::Scalar(0)), src_bin);
    }
    if (w > h) {
        int d = w - h;
        vconcat(cv::Mat(d / 2, w, CV_8UC1, cv::Scalar(0)), src_bin, src_bin);
        vconcat(src_bin, cv::Mat(d - d / 2, w, CV_8UC1, cv::Scalar(0)), src_bin);
    }
}

enum ResizeFlags {
    RESIZE_W16_H16 = 0,
    RESIZE_W32_H8 = 1,
    RESIZE_W8_H32 = 2
};

std::string shash(cv::Mat src_bin, ResizeFlags flag = RESIZE_W16_H16)
{
    cv::Size size_;
    switch (flag) {
    case RESIZE_W16_H16:
        size_ = cv::Size(16, 16);
        break;
    case RESIZE_W32_H8:
        size_ = cv::Size(32, 8);
        break;
    case RESIZE_W8_H32:
        size_ = cv::Size(8, 32);
        break;
    default:
        break;
    }

    resize(src_bin, src_bin, size_);
    std::stringstream hash_value;
    uchar* pix = src_bin.data;
    int tmp_dec = 0;
    for (int ro = 0; ro < 256; ro++) {
        tmp_dec = tmp_dec << 1;
        if ((bool)*pix)
            tmp_dec++;
        if (ro % 4 == 3) {
            hash_value << std::hex << tmp_dec;
            tmp_dec = 0;
        }
        pix++;
    }
    return hash_value.str();
}

enum HammingFlags {
    HAMMING16 = 16,
    HAMMING64 = 64
};
int hamming(std::string hash1, std::string hash2, HammingFlags flag = HAMMING64)
{
    hash1.insert(hash1.begin(), flag - hash1.size(), '0');
    hash2.insert(hash2.begin(), flag - hash2.size(), '0');
    int dist = 0;
    for (int i = 0; i < flag; i = i + 16) {
        size_t x = strtoull(hash1.substr(i, 16).c_str(), NULL, 16)
            ^ strtoull(hash2.substr(i, 16).c_str(), NULL, 16);
        while (x) {
            dist++;
            x = x & (x - 1);
        }
    }
    return dist;
}

} // namespace penguin

namespace penguin {
enum RectFlags {
    X = 0,
    Y = 1,
    WIDTH = 2,
    HEIGHT = 3
};

class Widget : public cv::Rect {
public:
    Widget() { }
    Widget(const cv::Mat& img, const cv::Point& topleft)
        : cv::Rect(topleft, img.size())
    {
        _img = img;
    }
    Widget(const cv::Mat& img, const cv::Rect& rect)
        : Widget(img, rect.tl())
    {
    }
    void relate(const Widget& widget)
    {
        auto& self = *this;
        self.x += widget.x;
        self.y += widget.y;
    }
    void relate(const cv::Point& topleft)
    {
        auto& self = *this;
        self.x += topleft.x;
        self.y += topleft.y;
    }

protected:
    cv::Mat _img;
    std::set<std::reference_wrapper<Widget>> child;
};

enum CharFlags {
    CHAR_STAGE = 0,
    CHAR_ITEM = 1
};

enum FontFlags {
    NOVECENTO_WIDEBOLD = 0,
    SOURCE_HAN_SANS_CN_MEDIUM = 1,
    NUBER_NEXT_DEMIBOLD_CONDENSED = 2,
    RODIN_PRO_DB = 3,
    SOURCE_HAN_SANS_KR_BOLD = 4
};

class Character : public Widget {
    struct CharDist {
        std::string chr;
        int dist;
        CharDist(const std::string& chr_, int dist_)
            : chr(chr_)
            , dist(dist_)
        {
        }
    };

public:
    const std::string chr() const { return _chr; }
    const int dist() const { return _dist; }
    Character() { }
    Character(const cv::Mat& img_bin, const cv::Point& topleft, FontFlags flag)
        : Widget(img_bin, topleft)
    {
        font = flag;
        _img = _img(cv::boundingRect(_img));
        _get_char();
    }
    Character(const cv::Mat& img_bin, const cv::Rect& rect, FontFlags flag)
        : Character(img_bin, rect.tl(), flag)
    {
    }

private:
    std::string _chr;
    int _dist = HAMMING64 * 4;
    FontFlags font;
    std::string _hash;
    std::vector<CharDist> _dist_list;
    void _get_char()
    {
        cv::Mat src_bin = _img;
        squarize(src_bin);
        _hash = shash(src_bin);
        std::string chr;
        dict char_dict;
        if (font == NOVECENTO_WIDEBOLD)
            char_dict = hash_index["stage"];
        else
            char_dict = hash_index["item"][server];
        for (const auto& [kchar, vhash] : char_dict.items()) {
            int dist = hamming(_hash, vhash, HAMMING64);
            _dist_list.push_back(CharDist(kchar, dist));
            if (dist < _dist) {
                _chr = kchar;
                _dist = dist;
            }
        }

        std::sort(_dist_list.begin(), _dist_list.end(),
            [](const CharDist& val1, const CharDist& val2) {
                return val1.dist < val2.dist;
            });
    }
};

std::map<std::string, FontFlags> Server2Font {
    { "CN", SOURCE_HAN_SANS_CN_MEDIUM },
    { "US", NUBER_NEXT_DEMIBOLD_CONDENSED },
    { "JP", RODIN_PRO_DB },
    { "KR", SOURCE_HAN_SANS_KR_BOLD }
};

class ItemQuantity : public Widget {
public:
    int quantity() { return _quantity; }
    ItemQuantity() { }
    ItemQuantity(const cv::Mat& img_bin, const cv::Point& topleft)
        : Widget(img_bin, topleft)
    {
        _get_quantity();
    }
    ItemQuantity(const cv::Mat& img_bin, const cv::Rect& rect)
        : Widget(img_bin, rect)
    {
    }
    const Character& operator[](uint i) const { return _quantity_[i]; }

private:
    uint _quantity = 0;
    std::vector<Character> _quantity_;
    void _get_quantity()
    {
        std::string quantity_str = "";
        auto sp = separate(_img, LEFT);
        for (const auto& range : sp) {
            cv::Mat charimg = _img(cv::Range(0, _img.rows), range);
            auto chr = Character(charimg, cv::Point(range.start, 0), Server2Font[server]);
            chr.relate(*this);
            quantity_str += chr.chr();
            _quantity_.push_back(chr);
        }
        _quantity = std::stoi(quantity_str); //try-catch
    }
};

class Stage;
class ItemTemplates {
    struct Templ {
        std::string itemId;
        cv::Mat img;
        Templ(std::pair<const std::string, cv::Mat> templ)
            : itemId(templ.first)
            , img(templ.second)
        {
        }
    };

public:
    std::list<Templ> _templ_list;
    ItemTemplates()
    {
        for (auto& templ : item_templs) {
            _templ_list.push_back(templ);
        }
    }
    ItemTemplates(const Stage& stage)
    {
        // TODO
    }
};

class Item : public Widget {
    struct ItemConfidence {
        std::string itemId;
        double confidence;
        ItemConfidence(const std::string& itemId_, double conf_)
            : itemId(itemId_)
            , confidence(conf_)
        {
        }
    };

public:
    const std::string& itemId() { return _itemId; }
    Item() { }
    Item(
        const cv::Mat& img,
        const cv::Point& topleft,
        double diameter,
        const ItemTemplates& templs = ItemTemplates())
        : Widget(img, topleft)
    {
        _diameter = diameter;
        _get_item(templs);
        // _get_quantity();
    }
    Item(
        const cv::Mat& img,
        const cv::Rect& rect,
        double diameter,
        const ItemTemplates& templs = ItemTemplates())
        : Widget(img, rect)
    {
    }

private:
    std::string _itemId = "";
    double _confidence = 0;
    ItemQuantity _quantity;
    int _diameter;
    std::vector<ItemConfidence> _confidence_list;
    void _get_item(const ItemTemplates& templs)
    {
        auto s = cv::getTickCount();

        auto& self = *this;
        cv::Mat itemimg;
        double coeff = (double)ITEM_RESIZED_WIDTH / _img.cols;
        if (coeff < 1) {
            int width = ITEM_RESIZED_WIDTH;
            int height = round(_img.rows * coeff);
            resize(_img, itemimg, cv::Size(width, height));
        }
        std::map<std::string, cv::Point> _tmp_itemId2loc;
        for (const auto& templ : templs._templ_list) {
            const std::string& itemId = templ.itemId;
            cv::Mat templimg = templ.img;
            double fx = _diameter * coeff / TEMPLATE_DIAMETER;
            resize(templimg, templimg, cv::Size(), fx, fx, cv::INTER_AREA);
            cv::Mat mask;
            cv::cvtColor(templimg, mask, cv::COLOR_BGR2GRAY);
            cv::threshold(mask, mask, 25, 255, cv::THRESH_BINARY);
            cv::Mat resimg;
            cv::matchTemplate(itemimg, templimg, resimg, cv::TM_CCORR_NORMED, mask);
            double minval, maxval;
            cv::Point minloc, maxloc;
            cv::minMaxLoc(resimg, &minval, &maxval, &minloc, &maxloc);
            _confidence_list.push_back(ItemConfidence(itemId, maxval));
            _tmp_itemId2loc[itemId] = maxloc;
        }
        std::sort(_confidence_list.begin(), _confidence_list.end(),
            [](const ItemConfidence& val1, const ItemConfidence& val2) {
                return val1.confidence > val2.confidence;
            });
        std::string itemId = _confidence_list.front().itemId;
        cv::Point topleft_new = _tmp_itemId2loc[itemId];
        cv::Size size_new = cv::Size(
            round(TEMPLATE_WIDTH * ((double)_diameter / TEMPLATE_DIAMETER)),
            round(TEMPLATE_HEIGHT * ((double)_diameter / TEMPLATE_DIAMETER)));
        _img = _img(cv::Rect(topleft_new, size_new));
        self.relate(topleft_new);
        _confidence = _confidence_list.front().confidence;
        if (_confidence > 0.9) {
            _itemId = itemId;
        }
        auto e = cv::getTickCount();
        std::cout << (e - s) / cv::getTickFrequency() * 1000 << std::endl;
    }
    void _get_quantity()
    {
        cv::Mat quantityimg;
    }
};

namespace result {
    extern dict stage_index;
} // namespace result

} // namespace penguin

#endif