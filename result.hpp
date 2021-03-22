#ifndef PENGUIN_RESULT_HPP_
#define PENGUIN_RESULT_HPP_

#include "core.hpp"
#include "json.hpp"
#include "md5.hpp"
#include "recognize.hpp"
#include <iomanip>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
using dict = nlohmann::ordered_json;

namespace penguin { // result
const int BASELINE_V_HEIGHT_MIN = 10;
const int RESULT_DIST_THRESHOLD = 25;
const double STAR_WIDTH_PROP = 0.4;
const double DROP_AREA_X_PROP = 0.21;
const double DROP_AREA_Y_PROP = 0.2;
const double DROP_AREA_HEIGHT_PROP = 0.8;
const double ITEM_DIAMETER_PROP = 0.524;
const double DROPTYPE_W_H_PROP = 7;

enum DroptypeFlags {
    DROPTYPE_UNDEFINED = 0,
    DROPTYPE_SANITY = 1,
    DROPTYPE_FIRST = 2,
    DROPTYPE_LMB = 3,
    DROPTYPE_FURNITURE = 4,
    DROPTYPE_NORMAL_DROP = 5,
    DROPTYPE_SPECIAL_DROP = 6,
    DROPTYPE_EXTRA_DROP = 7
};

enum HsvFlags {
    H = 0,
    S = 1,
    V = 2
};

std::map<DroptypeFlags, std::string>
    Droptype2Str {
        { DROPTYPE_UNDEFINED, "" },
        { DROPTYPE_SANITY, "SANITY" },
        { DROPTYPE_FIRST, "FIRST" },
        { DROPTYPE_LMB, "LMB" },
        { DROPTYPE_FURNITURE, "FURNITURE" },
        { DROPTYPE_NORMAL_DROP, "NORMAL_DROP" },
        { DROPTYPE_SPECIAL_DROP, "SPECIAL_DROP" },
        { DROPTYPE_EXTRA_DROP, "EXTRA_DROP" }
    };

const std::map<int, DroptypeFlags> HSV_H2Droptype {
    { 51, DROPTYPE_LMB },
    { 201, DROPTYPE_FIRST },
    { 0, DROPTYPE_NORMAL_DROP },
    { 360, DROPTYPE_NORMAL_DROP },
    { 25, DROPTYPE_SPECIAL_DROP },
    { 63, DROPTYPE_EXTRA_DROP },
    { 24, DROPTYPE_FURNITURE }
};

class Widget_ResultLabel : public Widget {
public:
    const bool is_result() const
    {
        return _is_result;
    }
    Widget_ResultLabel() = default;
    Widget_ResultLabel(Widget* const parent_widget)
        : Widget("result", parent_widget)
    {
    }
    Widget_ResultLabel(const cv::Mat& img, Widget* const parent_widget = nullptr)
        : Widget("result", parent_widget)
    {
        set_img(img);
    }
    void set_img(const cv::Mat& img)
    {
        Widget::set_img(img);
        if (_img.channels() == 3) {
            cv::cvtColor(_img, _img, cv::COLOR_BGR2GRAY);
            cv::threshold(_img, _img, 127, 255, cv::THRESH_BINARY);
        }
    }
    Widget_ResultLabel& analyze()
    {
        if (!_img.empty()) {
            _get_is_result();
        }
        if (!_is_result) {
            push_exception(ERROR, EXC_FALSE, report(true));
        }
        return *this;
    }
    const dict report(bool debug = false)
    {
        dict _report = dict::object();
        if (!debug) {
            _report.merge_patch(Widget::report());
            _report["isResult"] = _is_result;
        } else {
            _report.merge_patch(Widget::report(debug));
            _report["isResult"] = _is_result;
            _report["hash"] = _hash;
            _report["dist"] = _dist;
        }
        return _report;
    }

private:
    bool _is_result = false;
    std::string _hash = "";
    int _dist = HAMMING64 * 4;
    void _get_is_result()
    {
        auto& self = *this;
        auto resultrect = cv::boundingRect(_img);
        self._relate(resultrect.tl());
        if (resultrect.empty()) {
            return;
        }
        _img = _img(resultrect);
        auto result_img = _img;
        _hash = shash(result_img);
        std::string hash_std
            = resource.get<dict>("hash_index")["result"][server];
        _dist = hamming(_hash, hash_std);
        if (_dist <= RESULT_DIST_THRESHOLD) {
            _is_result = true;
        } else {
            _is_result = false;
        }
    }
};

class Widget_Stage : public Widget {
public:
    const std::string& stage_code() const
    {
        return _stage_code;
    }
    const std::string& stageId() const
    {
        return _stageId;
    }
    Widget_Stage() = default;
    Widget_Stage(Widget* const parent_widget)
        : Widget("stage", parent_widget)
    {
    }
    Widget_Stage(const cv::Mat& img, Widget* const parent_widget = nullptr)
        : Widget("stage", parent_widget)
    {
        set_img(img);
    }
    void set_img(const cv::Mat& img)
    {
        Widget::set_img(img);
        if (_img.channels() == 3) {
            cv::cvtColor(_img, _img, cv::COLOR_BGR2GRAY);
            double maxval;
            cv::minMaxIdx(_img, nullptr, &maxval);
            cv::threshold(_img, _img, maxval * 0.9, 255, cv::THRESH_BINARY);
        }
    }
    Widget_Stage& analyze()
    {
        if (!_img.empty()) {
            _get_stage();
        }
        if (_stageId.empty()) {
            push_exception(ERROR, EXC_NOTFOUND, report(true));
        }
        return *this;
    }
    const dict report(bool debug = false)
    {
        dict _report = dict::object();
        if (!debug) {
            _report.merge_patch(Widget::report());
            _report["stageCode"] = _stage_code;
            _report["stageId"] = _stageId;
        } else {
            _report.merge_patch(Widget::report(debug));
            _report["stageCode"] = _stage_code;
            _report["stageId"] = _stageId;
            for (auto& chr : _characters) {
                _report["chars"].push_back(chr.report(debug));
            }
        }
        return _report;
    }

private:
    std::string _stage_code = "";
    std::string _stageId = "";
    bool existance = false;
    std::list<Widget_Character> _characters;
    void _get_stage()
    {
        auto& self = *this;
        auto stagerect = cv::boundingRect(_img);
        self._relate(stagerect.tl());
        if (stagerect.empty()) {
            return;
        }
        _img = _img(stagerect);
        auto stage_img = _img;
        auto sp = separate(stage_img, LEFT);
        for (auto& range : sp) {
            int length = range.end - range.start;
            auto charimg = stage_img(cv::Rect(
                range.start, 0, length, height));
            std::string label = "chr." + std::to_string(_characters.size());
            Widget_Character chr {
                charimg, FONT_NOVECENTO_WIDEBOLD, label, this
            };
            chr.analyze();
            _stage_code += chr.chr();
            _characters.emplace_back(chr);
        }
        if (const auto& stage_index = resource.get<dict>("stage_index");
            stage_index.contains(_stage_code)) {
            _stageId = (std::string)stage_index[_stage_code]["stageId"];
        }
    }
};

class Widget_Stars : public Widget {
public:
    const bool is_3stars() const
    {
        return _stars == 3;
    }
    Widget_Stars() = default;
    Widget_Stars(Widget* const parent_widget)
        : Widget("3stars", parent_widget)
    {
    }
    Widget_Stars(const cv::Mat& img, Widget* const parent_widget = nullptr)
        : Widget(img, "3stars", parent_widget)
    {
        set_img(img);
    }
    void set_img(const cv::Mat& img)
    {
        Widget::set_img(img);
        if (_img.channels() == 3) {
            cv::cvtColor(_img, _img, cv::COLOR_BGR2GRAY);
            cv::threshold(_img, _img, 127, 255, cv::THRESH_BINARY);
        }
    }
    Widget_Stars& analyze()
    {
        if (!_img.empty()) {
            _get_is_3stars();
        }
        if (_stars != 3) {
            push_exception(ERROR, EXC_FALSE, report(true));
        }
        return *this;
    }
    const dict report(bool debug = false)
    {
        dict _report = dict::object();
        if (!debug) {
            _report.merge_patch(Widget::report());
            _report["count"] = _stars;
        } else {
            _report.merge_patch(Widget::report(debug));
            _report["count"] = _stars;
        }
        return _report;
    }

private:
    uint _stars = 0;
    void _get_is_3stars()
    {
        auto& self = *this;
        auto star_img = _img;
        auto laststar_range = separate(star_img, RIGHT, 1).back();
        auto laststar = star_img(cv::Range(0, height), laststar_range);
        auto starrect = cv::boundingRect(laststar);
        if (starrect.empty()) {
            return;
        }
        auto sp = separate(star_img(cv::Rect(0, 0, width, height / 2)), LEFT);
        for (auto it = sp.cbegin(); it != sp.cend();) {
            const auto& range = *it;
            if (auto length = range.end - range.start;
                length < height * STAR_WIDTH_PROP) {
                it = sp.erase(it);
            } else {
                ++it;
            }
        }
        starrect.x = sp.front().start;
        starrect.width = sp.back().end - starrect.x;
        _img = star_img = star_img(starrect);
        self._relate(starrect.tl());
        _stars = sp.size();
    }
};

class Widget_DroptypeLine : public Widget {
public:
    const DroptypeFlags droptype() const
    {
        return _droptype;
    }
    Widget_DroptypeLine() = default;
    Widget_DroptypeLine(Widget* const parent_widget)
        : Widget(parent_widget)
    {
    }
    Widget_DroptypeLine(
        const cv::Mat& img,
        Widget* const parent_widget = nullptr)
        : Widget(img, parent_widget)
    {
    }
    Widget_DroptypeLine& analyze()
    {
        if (!_img.empty()) {
            _get_droptype();
        }
        return *this;
    }
    const dict report(bool debug = false)
    {
        dict _report = dict::object();
        if (!debug) {
        } else {
            _report["hsv"] = { (int)round(_hsv[H]), _hsv[S], _hsv[V] };
            _report["dist"] = _dist;
        }
        return _report;
    }

private:
    DroptypeFlags _droptype = DROPTYPE_UNDEFINED;
    cv::Vec3f _hsv;
    int _dist = HSV_DIST_MAX;
    void _get_droptype()
    {
        _get_hsv();
        auto hsv = _hsv;
        if (hsv[S] < 0.1 && hsv[V] <= 0.9) {
            _droptype = DROPTYPE_NORMAL_DROP;
            _dist = 0;
            return;
        } else if (hsv[S] < 0.1 && hsv[V] > 0.9) {
            _droptype = DROPTYPE_SANITY;
            _dist = 0;
            return;
        } else {
            for (const auto& [kh, vtype] : HSV_H2Droptype) {
                if (vtype == DROPTYPE_NORMAL_DROP
                    || vtype == DROPTYPE_SANITY) {
                    continue;
                }
                int dist = abs(kh - hsv[H]);
                if (dist < _dist) {
                    _dist = dist;
                    _droptype = vtype;
                }
            }
        }
    }
    void _get_hsv()
    {
        auto droplineimg = _img;
        cv::Mat4f bgra(cv::Vec4f(cv::mean(droplineimg)));
        cv::Mat3f hsv;
        cv::cvtColor(bgra, hsv, cv::COLOR_BGRA2BGR);
        cv::cvtColor(hsv, hsv, cv::COLOR_BGR2HSV);
        _hsv = hsv[0][0];
        _hsv[V] /= 255;
    }
};

class Widget_DroptypeText : public Widget {
    struct DroptypeDist {
        std::string droptype;
        int dist;
        DroptypeDist(std::string droptype_, int dist_)
            : droptype(droptype_)
            , dist(dist_)
        {
        }
    };

public:
    const DroptypeFlags droptype() const
    {
        return _droptype;
    }
    Widget_DroptypeText() = default;
    Widget_DroptypeText(Widget* const parent_widget)
        : Widget(parent_widget)
    {
    }
    Widget_DroptypeText(
        const cv::Mat& img,
        Widget* const parent_widget = nullptr)
        : Widget(img, parent_widget)
    {
    }
    Widget_DroptypeText& analyze()
    {
        if (!_img.empty()) {
            _get_droptype();
        }
        return *this;
    }
    const dict report(bool debug = false)
    {
        dict _report = dict::object();
        if (!debug) {
        } else {
            _report["hash"] = _hash;
            for (const auto& droptypedist : _dist_list) {
                _report["dist"][droptypedist.droptype] = droptypedist.dist;
            }
        }
        return _report;
    }

private:
    DroptypeFlags _droptype = DROPTYPE_UNDEFINED;
    std::string _hash = "";
    int _dist = HAMMING64 * 4;
    std::vector<DroptypeDist> _dist_list;
    void _get_droptype()
    {
        _process_img();
        auto droptextimg = _img;
        if (server == "CN" || server == "KR") {
            droptextimg.adjustROI(0, 0, 0, -width / 2);
        }
        if (server == "US") {
            _hash = shash(droptextimg, RESIZE_W32_H8);
        } else {
            _hash = shash(droptextimg);
        }
        const auto& droptype_dict
            = resource.get<dict>("hash_index")["droptype"][server];
        int dist_spe = hamming(_hash, droptype_dict["SPECIAL_DROP"]);
        _dist_list.emplace_back("SPECIAL_DROP", dist_spe);
        int dist_fur = hamming(_hash, droptype_dict["FURNITURE"]);
        _dist_list.emplace_back("FURNITURE", dist_fur);
        if (dist_spe < dist_fur) {
            _dist = dist_spe;
            _droptype = DROPTYPE_SPECIAL_DROP;
        } else if (dist_fur < dist_spe) {
            _dist = dist_fur;
            _droptype = DROPTYPE_FURNITURE;
        } else {
            _droptype = DROPTYPE_UNDEFINED;
        }
    }
    void _process_img()
    {
        auto& self = *this;
        auto& droptextimg = _img;
        if (droptextimg.channels() == 3) {
            cv::cvtColor(droptextimg, droptextimg, cv::COLOR_BGR2GRAY);
        }
        double maxval;
        cv::minMaxIdx(droptextimg, nullptr, &maxval);
        cv::threshold(droptextimg, droptextimg, maxval / 2, 255, cv::THRESH_BINARY);
        while (droptextimg.rows > 0) {
            cv::Mat topline = droptextimg.row(0);
            int meanval = cv::mean(topline)[0];
            if (meanval < 127) {
                break;
            } else {
                droptextimg.adjustROI(-1, 0, 0, 0);
                self.y++;
            }
        }
        auto droptextrect = cv::boundingRect(droptextimg);
        if (droptextrect.empty()) {
            return;
        }
        droptextimg = droptextimg(droptextrect);
        self._relate(droptextrect.tl());
    }
};

class Widget_Droptype : public Widget {
public:
    const DroptypeFlags droptype() const
    {
        return _droptype;
    }
    const int items_count() const
    {
        return _items_count;
    }
    Widget_Droptype() = default;
    Widget_Droptype(
        const cv::Mat& img,
        const std::string& label,
        Widget* const parent_widget = nullptr)
        : Widget(img, label, parent_widget)
    {
    }
    Widget_Droptype& analyze()
    {
        if (!_img.empty()) {
            _get_droptype();
        }
        if (_droptype == DROPTYPE_UNDEFINED
            || _droptype == DROPTYPE_SANITY
            || _droptype == DROPTYPE_FIRST) {
            push_exception(ERROR, EXC_ILLEAGLE, report(true));
        }
        return *this;
    }
    const dict report(bool debug = false)
    {
        dict _report = dict::object();
        if (!debug) {
            _report.merge_patch(Widget::report());
            _report["dropType"] = Droptype2Str[_droptype];
        } else {
            _report.merge_patch(Widget::report(debug));
            _report["dropType"] = Droptype2Str[_droptype];
            _report["itemCount"] = _items_count;
            _report.merge_patch(_line.report(debug));
            if (!_text.empty()) {
                _report.merge_patch(_text.report(debug));
            }
        }
        return _report;
    }

private:
    DroptypeFlags _droptype = DROPTYPE_UNDEFINED;
    uint _items_count = round(width / (height * DROPTYPE_W_H_PROP));
    Widget_DroptypeLine _line { this };
    Widget_DroptypeText _text { this };
    void _get_droptype()
    {
        auto lineimg = _img(cv::Rect(0, 0, width, 1));
        _line.set_img(lineimg);
        _line.analyze();
        _droptype = _line.droptype();
        if (_droptype == DROPTYPE_SPECIAL_DROP
            || _droptype == DROPTYPE_FURNITURE) {
            auto textimg = _img(cv::Rect(0, 1, width, height - 1));
            _text.set_img(textimg);
            _text.analyze();
            _droptype = _text.droptype();
        }
    }
};

class Widget_DropArea : public Widget {
    struct Drop {
        Widget_Item dropitem;
        DroptypeFlags droptype;
        Drop(const Widget_Item& dropitem_, const DroptypeFlags& droptype_)
            : dropitem(dropitem_)
            , droptype(droptype_)
        {
        }
    };

public:
    Widget_DropArea() = default;
    Widget_DropArea(Widget* const parent_widget)
        : Widget(parent_widget)
    {
    }
    Widget_DropArea(
        const cv::Mat& img,
        const std::string& stage,
        Widget* const parent_widget = nullptr)
        : Widget(img, parent_widget)
    {
    }
    Widget_DropArea& analyze(const std::string& stage)
    {
        if (!_img.empty()) {
            _get_drops(stage);
        }
        if (_drop_list.empty()) {
            push_exception(ERROR, EXC_NOTFOUND, report(true));
        }
        return *this;
    }
    const dict report(bool debug = false)
    {
        dict _report = dict::object();
        if (_parent_widget == nullptr) {
            _report["exceptions"] = _exception_list;
        }
        _report["dropTypes"] = dict::array();
        _report["drops"] = dict::array();

        int droptypes_count = _droptype_list.size();
        for (int i = 0; i < droptypes_count; i++) {
            _report["dropTypes"].push_back(_droptype_list[i].report(debug));
        }
        int drops_count = _drop_list.size();
        for (int i = 0; i < drops_count; i++) {
            _report["drops"].push_back(
                { { "dropType", Droptype2Str[_drop_list[i].droptype] } });
            _report["drops"][i].merge_patch(_drop_list[i].dropitem.report(debug));
        }
        return _report;
    }

private:
    dict _drops_data;
    std::vector<Drop> _drop_list;
    std::vector<Widget_Droptype> _droptype_list;
    auto _get_separate()
    {
        cv::Mat img_bin = _img;
        int offset = round(height * 0.75);
        img_bin.adjustROI(-offset, 0, 0, 0);
        cv::cvtColor(img_bin, img_bin, cv::COLOR_BGR2GRAY);
        cv::adaptiveThreshold(img_bin, img_bin, 255,
            cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY,
            img_bin.rows / 2 * 2 + 1, -20);

        // get baseline_h
        int row = 0;
        int maxcount = 0; // will move in "for" in C++20
        for (int ro = img_bin.rows - 1; ro >= 0; ro--) {
            uchar* pix = img_bin.data + ro * img_bin.step;
            int count = 0; // will move in "for" in C++20
            for (int co = 0; co < img_bin.cols; co++) {
                if ((bool)*pix) {
                    count++;
                }
                pix++;
            }
            if (count > maxcount) {
                row = ro;
                maxcount = count;
            }
        }
        int baseline_h = row + offset;
        auto sp = separate(img_bin(cv::Rect(0, row, width, 1)), LEFT);
        int item_diameter
            = height / DROP_AREA_HEIGHT_PROP * ITEM_DIAMETER_PROP;
        for (auto it = sp.cbegin(); it != sp.cend();) {
            const auto& range = *it;
            if (auto length = range.end - range.start;
                length < item_diameter) {
                it = sp.erase(it);
            } else {
                ++it;
            }
        }
        sp.front().start = 0;
        return std::tuple(baseline_h, sp);
    }
    void _get_drops(std::string stage)
    {
        int item_diameter
            = height / DROP_AREA_HEIGHT_PROP * ITEM_DIAMETER_PROP;
        ItemTemplates templs { stage };
        auto [baseline_h, sp] = _get_separate();
        for (const auto& droptype_range : sp) {
            auto droptypeimg = _img(cv::Range(baseline_h, height), droptype_range);
            std::string label = "dropTypes." + std::to_string(_droptype_list.size());
            Widget_Droptype droptype { droptypeimg, label, this };
            droptype.analyze();
            _droptype_list.emplace_back(droptype);
            if (const auto type = droptype.droptype();
                type == DROPTYPE_UNDEFINED
                || type == DROPTYPE_SANITY
                || type == DROPTYPE_FIRST) {
                break;
            } else if (type == DROPTYPE_LMB) {
                continue;
            } else if (std::string label = "drops." + std::to_string(_drop_list.size());
                       type == DROPTYPE_FURNITURE) {
                _drop_list.emplace_back(
                    Drop(Widget_Item(FURNI_1, label, this), type));
            } else {
                int items_count = droptype.items_count();
                int length = (droptype_range.end - droptype_range.start) / items_count;
                for (int i = 0; i < items_count; i++) {
                    std::string label = "drops." + std::to_string(_drop_list.size());
                    auto range = cv::Range(
                        droptype_range.start + length * i,
                        droptype_range.start + length * (i + 1));
                    auto dropimg = _img(cv::Range(0, baseline_h), range);
                    Widget_Item drop { dropimg, item_diameter, label, this };
                    drop.analyze(templs);
                    _drop_list.emplace_back(drop, type);
                    _drops_data.push_back(
                        { { "dropType", Droptype2Str[type] },
                            { "itemId", drop.itemId() },
                            { "quantity", drop.quantity() } });
                }
            }
        }
    }
};

class Result : public Widget {
public:
    Result() = default;
    Result(const cv::Mat& img)
        : Widget(img)
    {
    }
    Result& analyze()
    {
        _get_baseline_v();
        _get_result_label();
        _get_stage();
        _get_stars();
        _get_drop_area();
        return *this;
    }
    std::string get_md5()
    {
        MD5 md5;
        return md5(_img.data, _img.step * _img.rows);
    }
    std::string get_fingerprint()
    {
        cv::Mat img;
        cv::resize(_img, img, cv::Size(8, 8));
        cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
        uchar* pix = img.data;
        std::stringstream fp;
        for (int i = 0; i < 64; i++) {
            fp << std::setw(2) << std::setfill('0') << std::hex << (int)*pix;
            pix++;
        }
        return fp.str();
    }
    const dict report(bool debug = false)
    {
        dict _report = dict::object();
        if (_parent_widget == nullptr) {
            _report["exceptions"] = _exception_list;
        }
        if (!debug) {
            _report["resultLabel"] = _result_label.report()["isResult"];
            _report["stage"] = _stage.report();
            _report["stars"] = _stars.report()["count"];
            _report.merge_patch(_drop_area.report());
        } else {
            _report["resultLabel"] = _result_label.report(debug);
            _report["stage"] = _stage.report(debug);
            _report["stars"] = _stars.report(debug);
            _report.merge_patch(_drop_area.report(debug));
        }
        return _report;
    }

private:
    Widget _baseline_v { this };
    Widget_Stage _stage { this };
    Widget_Stars _stars { this };
    Widget_ResultLabel _result_label { this };
    Widget_DropArea _drop_area { this };

    void _get_baseline_v()
    {
        if (_status == STATUS_HASERROR || _status == STATUS_ERROR) {
            return;
        }
        cv::Mat img_bin = _img;
        int offset = height / 2;
        img_bin.adjustROI(-offset, 0, 0, -width / 2);
        cv::cvtColor(img_bin, img_bin, cv::COLOR_BGR2GRAY);
        cv::threshold(img_bin, img_bin, 127, 255, cv::THRESH_BINARY);

        int column = 0;
        cv::Range range = { 0, 0 };
        for (int co = 0; co < img_bin.cols; co++) {
            uchar* pix = img_bin.data + (img_bin.rows - 1) * img_bin.step + co;
            int start = 0, end = 0;
            bool prober[2] = { false, false }; // will move in "for" in C++20
            for (int ro = img_bin.rows - 1; ro > 0; ro--) {
                prober[END] = prober[BEGIN];
                prober[BEGIN] = (bool)*pix;
                if (prober[BEGIN] == true && prober[END] == false) {
                    end = ro + 1;
                } else if (prober[BEGIN] == false && prober[END] == true) {
                    start = ro + 1;
                }
                if (start != 0 && end != 0) {
                    break;
                }
                pix = pix - img_bin.step;
            }
            if ((start != 0 && end != 0)
                && (end - start > range.end - range.start)) {
                column = co;
                range.start = start;
                range.end = end;
            }
        }
        cv::Mat baseline_v_img = img_bin(range, cv::Range(column, column + 1));
        _baseline_v.set_img(baseline_v_img);
        _baseline_v.y += offset;
        if (_baseline_v.empty()
            || _baseline_v.height < BASELINE_V_HEIGHT_MIN
            || _baseline_v.x <= _baseline_v.height) {
            _result_label.push_exception(ERROR, EXC_FALSE, report(true));
        }
    }
    void _get_result_label()
    {
        if (_status == STATUS_HASERROR || _status == STATUS_ERROR) {
            return;
        }
        const auto& bv = _baseline_v;
        auto result_img = _img(
            cv::Range(bv.y + bv.height / 2, bv.y + bv.height),
            cv::Range(0, bv.x - 5));
        _result_label.set_img(result_img);
        _result_label.analyze();
    }
    void _get_stage()
    {
        if (_status == STATUS_HASERROR || _status == STATUS_ERROR) {
            return;
        }
        const auto& bv = _baseline_v;
        auto stage_img = _img(
            cv::Range(bv.y, bv.y + bv.height / 4),
            cv::Range(0, bv.x / 2));
        _stage.set_img(stage_img);
        _stage.analyze();
    }
    void _get_stars()
    {
        if (_status == STATUS_HASERROR || _status == STATUS_ERROR) {
            return;
        }
        const auto& bv = _baseline_v;
        auto star_img = _img(
            cv::Range(bv.y, bv.y + bv.height / 2),
            cv::Range(_stage.x + _stage.width, bv.x - 5));
        _stars.set_img(star_img);
        _stars.analyze();
    }
    void _get_drop_area()
    {
        if (_status == STATUS_HASERROR || _status == STATUS_ERROR) {
            return;
        }
        const auto& bv = _baseline_v;
        auto drop_area_img = _img(
            cv::Range(bv.y + bv.height * DROP_AREA_Y_PROP, bv.y + bv.height),
            cv::Range(bv.x + bv.height * DROP_AREA_X_PROP, width));
        _drop_area.set_img(drop_area_img);
        _drop_area.analyze(_stage.stage_code());
    }
};

} // namespace penguin

#endif // PENGUIN_RESULT_HPP_