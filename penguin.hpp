#ifndef PENGUIN_HPP_
#define PENGUIN_HPP_

#include "json.hpp"
// #include <iomanip>
#include <any>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
using dict = nlohmann::ordered_json;

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
const double _ITEM_QTY_Y_PROP = 0.7;
const double _ITEM_QTY_WIDTH_PROP = 0.83;
const double _ITEM_QTY_HEIGHT_PROP = 0.16;
const double _ITEM_CHR_HEIGHT_PROP = 0.7;
} // namespace penguin

namespace penguin { // global
std::string server;

class Resource {
public:
    static Resource& get_instance()
    {
        static Resource instance;
        return instance;
    }
    void add(const std::string& name, const std::any& src)
    {
        if (!name.empty()) {
            _resource[name] = src;
        }
    }
    template <typename SrcType>
    const SrcType& get(const std::string& name)
    {
        if (auto it = _resource.find(name); it != _resource.end()) {
            SrcType& res = std::any_cast<SrcType&>(it->second);
            return res;
        } else {
            SrcType& res = std::any_cast<SrcType&>(_resource[""]);
            return res;
        }
    }

private:
    std::map<std::string, std::any> _resource;
    Resource()
    {
        _resource[""] = std::any();
    };
    Resource(const Resource&) = delete;
    Resource(Resource&&) = delete;
    Resource& operator=(const Resource&) = delete;
    Resource& operator=(Resource&&) = delete;
};
auto& resource = Resource::get_instance();
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
    int begin;
    if (direc == TOP) {
        for (int ro = 0; ro < src_bin.rows; ro++) {
            uchar* pix = src_bin.data + ro * src_bin.step;
            int co = 0;
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
                int end = ro;
                sp.emplace_back(cv::Range(begin, end));
                isodd = !isodd;
                if (sp.size() == n)
                    break;
            }
        }
        if (isodd) {
            int end = src_bin.rows;
            sp.emplace_back(cv::Range(begin, end));
        }
    } else if (direc == BOTTOM) {
        for (int ro = src_bin.rows - 1; ro >= 0; ro--) {
            uchar* pix = src_bin.data + ro * src_bin.step;
            int co = 0;
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
                int end = ro + 1;
                sp.emplace_back(cv::Range(end, begin));
                isodd = !isodd;
                if (sp.size() == n)
                    break;
            }
        }
        if (isodd) {
            int end = 0;
            sp.emplace_back(cv::Range(end, begin));
        }
    } else if (direc == LEFT) {
        for (int co = 0; co < src_bin.cols; co++) {
            uchar* pix = src_bin.data + co;
            int ro = 0;
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
                int end = co;
                sp.emplace_back(cv::Range(begin, end));
                isodd = !isodd;
                if (sp.size() == 2 * n)
                    break;
            }
        }
        if (isodd) {
            int end = src_bin.cols;
            sp.emplace_back(cv::Range(begin, end));
        }
    } else if (direc == RIGHT) {
        for (int co = src_bin.cols - 1; co >= 0; co--) {
            uchar* pix = src_bin.data + co;
            int ro = 0;
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
                int end = co + 1;
                sp.emplace_back(cv::Range(end, begin));
                isodd = !isodd;
                if (sp.size() == 2 * n)
                    break;
            }
        }
        if (isodd) {
            int end = 0;
            sp.emplace_back(cv::Range(end, begin));
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

class Widget {
public:
    int x = 0;
    int y = 0;
    const int& width = _img.cols;
    const int& height = _img.rows;
    Widget() = default;
    Widget(const Widget& widget)
    {
        _img = widget._img;
        x = widget.x;
        y = widget.y;
        _parent = widget._parent;
    }
    Widget(Widget&& widget) noexcept
    {
        _img = widget._img;
        x = widget.x;
        y = widget.y;
        _parent = widget._parent;
    }
    Widget(const cv::Mat& img, Widget* const widget = nullptr)
    {
        _parent = widget;
        auto& parent = *_parent;
        auto& self = *this;
        _img = img;
        if (_parent != nullptr) {
            cv::Size _;
            cv::Point topleft;
            img.locateROI(_, topleft);
            x = topleft.x;
            y = topleft.y;
            _relate(parent);
        }
    }
    bool empty() const
    {
        return width <= 0 || height <= 0;
    }
    virtual const dict& debug_report()
    {
        _report["Rect"] = { x, y, width, height };
        return _report;
    }
    void show() const
    {
        show_img(_img);
    }
    Widget& operator=(Widget& widget)
    {
        if (this != &widget) {
            x = widget.x;
            y = widget.y;
            _img = widget._img;
        }
        return *this;
    }
    Widget& operator=(Widget&& widget) noexcept
    {
        if (this != &widget) {
            x = widget.x;
            y = widget.y;
            _img = widget._img;
        }
        return *this;
    }

protected:
    cv::Mat _img;
    Widget* _parent = nullptr;
    dict _report;
    void _relate(Widget& widget)
    {
        auto& self = *this;
        self.x += widget.x;
        self.y += widget.y;
    }
    void _relate(const cv::Point& topleft)
    {
        auto& self = *this;
        self.x += topleft.x;
        self.y += topleft.y;
    }
};

enum FontFlags {
    FONT_UNDEFINED = 0,
    FONT_NOVECENTO_WIDEBOLD = 1,
    FONT_SOURCE_HAN_SANS_CN_MEDIUM = 2,
    FONT_NUBER_NEXT_DEMIBOLD_CONDENSED = 3,
    FONT_RODIN_PRO_DB = 4,
    FONT_SOURCE_HAN_SANS_KR_BOLD = 5
};

const std::map<FontFlags, std::string> Font2Str {
    { FONT_NOVECENTO_WIDEBOLD, "Novecento Widebold" },
    { FONT_SOURCE_HAN_SANS_CN_MEDIUM, "Source Han Sans CN Medium" },
    { FONT_NUBER_NEXT_DEMIBOLD_CONDENSED, "Nuber Next Demibold Condensed" },
    { FONT_RODIN_PRO_DB, "Rodin Pro DB" },
    { FONT_SOURCE_HAN_SANS_KR_BOLD, "Source Han Sans KR Bold" }
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
    Character() = default;
    Character(
        const cv::Mat& img_bin,
        FontFlags flag,
        Widget* const parent = nullptr)
        : Widget(img_bin, parent)
    {
        font = flag;
        _get_char();
    }
    const dict& debug_report()
    {
        Widget::debug_report();
        _report["Char"] = _chr;
        _report["Hash"] = _hash;
        for (const auto& chardist : _dist_list) {
            _report["Dist"][chardist.chr] = chardist.dist;
        }
        return _report;
    }

private:
    std::string _chr = "";
    int _dist = HAMMING64 * 4;
    FontFlags font = FONT_UNDEFINED;
    std::string _hash = "";
    std::vector<CharDist> _dist_list;
    void _get_char()
    {
        auto& self = *this;
        auto charrect = cv::boundingRect(_img);
        _img = _img(charrect);
        self._relate(charrect.tl());
        auto charimg = _img;
        squarize(charimg);
        _hash = shash(charimg);
        std::string chr;
        dict char_dict;
        if (const auto& hash_index = resource.get<dict>("hash_index");
            font == FONT_NOVECENTO_WIDEBOLD)
            char_dict = hash_index["stage"];
        else
            char_dict = hash_index["item"][server];
        for (const auto& [kchar, vhash] : char_dict.items()) {
            int dist = hamming(_hash, vhash, HAMMING64);
            _dist_list.emplace_back(CharDist(kchar, dist));
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

const std::map<std::string, FontFlags> Server2Font {
    { "CN", FONT_SOURCE_HAN_SANS_CN_MEDIUM },
    { "US", FONT_NUBER_NEXT_DEMIBOLD_CONDENSED },
    { "JP", FONT_RODIN_PRO_DB },
    { "KR", FONT_SOURCE_HAN_SANS_KR_BOLD }
};

class ItemQuantity : public Widget {
public:
    const int quantity() const { return _quantity; }
    ItemQuantity() = default;
    ItemQuantity(const cv::Mat& img, Widget* const parent = nullptr)
        : Widget(img, parent)
    {
        if (_img.channels() != 1) {
            cv::cvtColor(_img, _img, cv::COLOR_BGR2GRAY);
        }
        cv::threshold(_img, _img, 127, 255, cv::THRESH_BINARY);
        _get_quantity();
    }
    const Character& operator[](uint i) const
    {
        auto it = _characters.begin();
        std::advance(it, i);
        return *it;
    }
    const dict& debug_report()
    {
        Widget::debug_report();
        _report["Quantity"] = _quantity;
        _report["Font"] = Font2Str.at(Server2Font.at(server));
        _report["Chars"] = dict::array();
        for (auto& chr : _characters) {
            _report["Chars"].push_back(chr.debug_report());
        }
        return _report;
    }

private:
    uint _quantity = 0;
    std::list<Character> _characters;
    void _get_quantity()
    {
        auto& self = *this;
        auto qtyimg = _img;
        std::string quantity_str = "";
        auto sp = separate(qtyimg, RIGHT);
        sp.remove_if([&](const cv::Range& range) {
            int length = range.end - range.start;
            return length > height * 0.5;
        });
        auto it = sp.cbegin();
        for (auto& range : sp) {
            int length = range.end - range.start;
            bool quantity_empty = quantity_str.empty();
            auto charimg = qtyimg(cv::Rect(
                range.start, 0, length, height));

            cv::Mat _;
            cv::Mat1i stats;
            cv::Mat1d centroids;
            int ccomps = cv::connectedComponentsWithStats(
                charimg, _, stats, centroids);
            if (ccomps - 1 != 1) {
                if (quantity_empty) {
                    sp.erase(it);
                    ++it;
                    continue;
                } else {
                    sp.erase(it, sp.cend());
                    break;
                }
            }
            if (auto ccomp = cv::Rect(
                    stats(1, cv::CC_STAT_LEFT), stats(1, cv::CC_STAT_TOP),
                    stats(1, cv::CC_STAT_WIDTH), stats(1, cv::CC_STAT_HEIGHT));
                ccomp.width > ccomp.height
                || ccomp.height / (double)height < _ITEM_CHR_HEIGHT_PROP) {
                if (quantity_empty) {
                    sp.erase(it);
                    ++it;
                    continue;
                } else {
                    sp.erase(it, sp.cend());
                    break;
                }
            }
            auto chr = Character(charimg, Server2Font.at(server), this);
            if (auto chr_str = chr.chr(); chr_str == "W") {
                quantity_str.insert(0, "0000");
            } else if (chr_str == "K") {
                quantity_str.insert(0, "000");
            } else {
                quantity_str.insert(0, chr_str);
            }
            _characters.push_front(chr);
            ++it;
        }
        if (!quantity_str.empty()) {
            _quantity = std::stoi(quantity_str);
            cv::Point topleft = cv::Point(sp.back().start, 0);
            cv::Point bottomright = cv::Point(sp.front().end, height - 1);
            _img = qtyimg(cv::Rect(topleft, bottomright));
            self._relate(topleft);
        } else {
            _img = cv::Mat();
        }
    }
};

class ItemTemplates {
    struct Templ {
        std::string itemId;
        cv::Mat img;
        Templ(const std::string itemId_, cv::Mat templimg_)
            : itemId(itemId_)
            , img(templimg_)
        {
        }
        Templ(std::pair<const std::string, cv::Mat> templ)
            : itemId(templ.first)
            , img(templ.second)
        {
        }
    };

public:
    const std::list<Templ>& templ_list() const
    {
        return _templ_list;
    };
    ItemTemplates()
    {
        const auto& item_templs
            = resource.get<std::map<std::string, cv::Mat>>("item_templs");
        for (const auto& templ : item_templs) {
            _templ_list.emplace_back(templ);
        }
    }
    ItemTemplates(const std::string stage_code)
    {
        const auto& stage_drop
            = resource.get<dict>("stage_index")[stage_code]["drops"];
        const auto& item_templs
            = resource.get<std::map<std::string, cv::Mat>>("item_templs");
        for (const auto& item : stage_drop.items()) {
            std::string itemId = item.value();
            auto templimg = item_templs.at(itemId);
            _templ_list.emplace_back(Templ(itemId, templimg));
        }
    }

private:
    std::list<Templ> _templ_list;
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
    const std::string& itemId() const { return _itemId; }
    const int quantity() const { return _quantity.quantity(); }
    Item() = default;
    Item(
        const cv::Mat& img,
        double diameter,
        const ItemTemplates& templs = ItemTemplates(),
        Widget* const parent = nullptr)
        : Widget(img, parent)
    {
        _diameter = diameter;
        _get_item(templs);
        _get_quantity();
    }
    const dict& debug_report()
    {
        Widget::debug_report();
        _report["itemId"] = _itemId;
        _report["Quantity"] = _quantity.debug_report();
        for (const auto& conf : _confidence_list) {
            _report["Confidence"][conf.itemId] = conf.confidence;
        }
        return _report;
    }

private:
    std::string _itemId = "";
    double _confidence = 0;
    ItemQuantity _quantity;
    int _diameter;
    std::vector<ItemConfidence> _confidence_list;
    void _get_item(const ItemTemplates& templs)
    {
        auto& self = *this;
        auto itemimg = _img;
        int coeff_multiinv = width / ITEM_RESIZED_WIDTH;
        double coeff = 1.0 / coeff_multiinv;
        if (coeff < 1) {
            resize(itemimg, itemimg, cv::Size(), coeff, coeff, cv::INTER_AREA);
        }
        std::map<std::string, cv::Point> _tmp_itemId2loc;
        for (const auto& templ : templs.templ_list()) {
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
            _confidence_list.emplace_back(ItemConfidence(itemId, maxval));
            _tmp_itemId2loc[itemId] = maxloc;
        }
        std::sort(_confidence_list.begin(), _confidence_list.end(),
            [](const ItemConfidence& val1, const ItemConfidence& val2) {
                return val1.confidence > val2.confidence;
            });
        std::string itemId = _confidence_list.front().itemId;
        cv::Point topleft_new = _tmp_itemId2loc[itemId];
        topleft_new.x *= coeff_multiinv;
        topleft_new.y *= coeff_multiinv;
        cv::Size size_new = cv::Size(
            round(TEMPLATE_WIDTH * ((double)_diameter / TEMPLATE_DIAMETER)),
            round(TEMPLATE_HEIGHT * ((double)_diameter / TEMPLATE_DIAMETER)));
        _img = _img(cv::Rect(topleft_new, size_new));
        self._relate(topleft_new);
        _confidence = _confidence_list.front().confidence;
        if (_confidence > 0.9) {
            _itemId = itemId;
        }
    }
    void _get_quantity()
    {
        cv::Rect quantityrect = cv::Rect(
            0, round(height * _ITEM_QTY_Y_PROP),
            round(width * _ITEM_QTY_WIDTH_PROP),
            round(height * _ITEM_QTY_HEIGHT_PROP));
        cv::Mat quantityimg = _img(quantityrect);
        _quantity = ItemQuantity(quantityimg, this);
    }
};

namespace result {
    class Stage : public Widget {
    public:
        const std::string& stage_code() const
        {
            return _stage_code;
        }
        Stage() = default;
        Stage(const cv::Mat& img, Widget* const parent = nullptr)
            : Widget(img, parent)
        {
            if (_img.channels() != 1) {
                cv::cvtColor(_img, _img, cv::COLOR_BGR2GRAY);
            }
            cv::threshold(_img, _img, 127, 255, cv::THRESH_BINARY);
            _get_stage();
        }
        const dict& debug_report()
        {
            Widget::debug_report();
            _report["stage_code"] = _stage_code;
            _report["stage_id"] = _stageId;
            _report["Chars"] = dict::array();
            for (auto& chr : _characters) {
                _report["Chars"].push_back(chr.debug_report());
            }
            return _report;
        }

    private:
        std::string _stage_code = "";
        std::string _stageId = "";
        std::list<Character> _characters;
        void _get_stage()
        {
            auto& self = *this;
            auto stagerect = cv::boundingRect(_img);
            self._relate(stagerect.tl());
            _img = _img(stagerect);
            auto stageimg = _img;
            auto sp = separate(stageimg, LEFT);
            for (auto& range : sp) {
                int length = range.end - range.start;
                auto charimg = stageimg(cv::Rect(
                    range.start, 0, length, height));
                auto chr = Character(
                    charimg, FONT_NOVECENTO_WIDEBOLD, this);
                _stage_code += chr.chr();
                _characters.emplace_back(chr);
            }
            const auto& stage_index = resource.get<dict>("stage_index");
            _stageId = (std::string)stage_index[_stage_code]["stageId"];
        }
    };
} // namespace result

} // namespace penguin

#endif // PENGUIN_HPP_