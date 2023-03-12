#ifndef PENGUIN_ACT24_HPP_
#define PENGUIN_ACT24_HPP_

#include <limits>
#include <optional>
#include <queue>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "../3rdparty/json/include/json.hpp"
#include "core.hpp"
#include "recognize.hpp"

using dict = nlohmann::ordered_json;
// extern void show_img(cv::Mat src);

namespace penguin
{
const int ACT24_TEMPLATE_DIAMETER = 72;

class Widget_ItemAct24 : public Widget
{
    struct ItemConfidence
    {
        std::string itemId;
        double confidence;
        ItemConfidence(const std::string& itemId_, double conf_)
            : itemId(itemId_), confidence(conf_) {}
    };

public:
    const std::string& itemId() const { return _itemId; }
    const int quantity() const { return _quantity.quantity(); }
    const double confidence() const { return _confidence; }
    const cv::Mat quantity_img() const { return _quantity.img(); }
    Widget_ItemAct24() = default;
    Widget_ItemAct24(FurniFlags furni, const std::string& label, Widget* const parent_widget = nullptr)
        : Widget(label, parent_widget),
          _itemId("furni"),
          _confidence(1),
          _quantity(Widget_ItemQuantity(furni)) {}
    Widget_ItemAct24(const cv::Mat& img, int diameter, const std::string& label, Widget* const parent_widget = nullptr)
        : Widget(img, label, parent_widget), _diameter(diameter) {}
    Widget_ItemAct24& analyze(const ItemTemplates& templs = ItemTemplates(),
                              const bool without_exception = false)
    {
        if (!_img.empty())
        {
            _get_item(templs);
            _get_quantity();
            if (_confidence < _CONFIDENCE_THRESHOLD &&
                without_exception == false)
            {
                push_exception(ERROR, ExcSubtypeFlags::EXC_LOWCONF);
            }
        }
        else
        {
            // add exception empty
        }

        return *this;
    }
    const dict report(bool debug = false)
    {
        dict rpt = dict::object();
        rpt.merge_patch(Widget::report(debug));
        if (!debug)
        {
            rpt["itemId"] = _itemId;
            rpt["quantity"] = _quantity.report()["quantity"];
        }
        else
        {
            rpt["itemId"] = _itemId;
            rpt["quantity"] = _quantity.report(debug);
            for (const auto& conf : _confidence_list)
            {
                rpt["confidence"][conf.itemId] = conf.confidence;
            }
        }
        return rpt;
    }

private:
    std::string _itemId;
    double _confidence = 0;
    Widget_ItemQuantity _quantity = {this};
    int _diameter = 0;
    std::vector<ItemConfidence> _confidence_list;
    void _get_item(const ItemTemplates& templs)
    {
        cv::Mat item_img = _img;
        int coeff_multiinv = width / ITEM_RESIZED_WIDTH;
        double coeff = 1.0 / coeff_multiinv;
        if (coeff < 1)
        {
            resize(item_img, item_img, cv::Size(), coeff, coeff, cv::INTER_AREA);
        }
        std::map<std::string, cv::Point> _tmp_itemId2loc;
        for (const auto& templ : templs.templ_list())
        {
            const std::string& itemId = templ.itemId;
            cv::Mat templimg = templ.img;
            double fx = _diameter * coeff / ACT24_TEMPLATE_DIAMETER;
            resize(templimg, templimg, cv::Size(), fx, fx, cv::INTER_AREA);
            cv::Mat resimg;
            cv::matchTemplate(item_img, templimg, resimg, cv::TM_CCOEFF_NORMED);
            double minval, maxval;
            cv::Point minloc, maxloc;
            cv::minMaxLoc(resimg, &minval, &maxval, &minloc, &maxloc);
            _confidence_list.emplace_back(itemId, maxval);
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
        cv::Size size_new = cv::Size(_diameter, _diameter);
        if (topleft_new.x + size_new.width > width)
        {
            size_new.width = width - topleft_new.x;
        }
        if (topleft_new.y + size_new.height > height)
        {
            size_new.height = height - topleft_new.y;
        }
        _img = _img(cv::Rect(topleft_new, size_new));
        _get_abs_pos();
        _confidence = _confidence_list.front().confidence;
        if (_confidence > _CONFIDENCE_THRESHOLD)
        {
            _itemId = itemId;
        }
    }
    void _get_quantity()
    {
        cv::Mat quantity_img = _img(
            cv::Range(static_cast<int>(round(0.71 * height)), height),
            cv::Range(0, static_cast<int>(round(0.98 * width))));
        _quantity.set_img(quantity_img);
        _quantity.analyze();
    }
};

class Widget_DropAct24 : public Widget
{
public:
    Widget_DropAct24() = default;
    Widget_DropAct24(Widget* const parent_widget)
        : Widget("dropAreaAct24", parent_widget) {}
    Widget_DropAct24(const cv::Mat& img, Widget* const parent_widget = nullptr)
        : Widget("dropAreaAct24", parent_widget)
    {
        set_img(img);
    }
    void set_img(cv::Mat img)
    {
        if (!img.empty() && img.channels() == 3)
        {
            cv::Mat img_bin = img(cv::Range(0, img.rows), cv::Range(0, static_cast<int>(round(0.2 * img.rows))));
            cv::cvtColor(img_bin, img_bin, cv::COLOR_BGR2GRAY);
            cv::threshold(img_bin, img_bin, 63, 255, cv::THRESH_BINARY);
            auto sp = separate(img_bin, DirectionFlags::TOP, 1);
            _baseline_h = sp.front().end;
            sp = separate(img_bin, DirectionFlags::LEFT, 1);
            img = img(cv::Range(0, img.rows), cv::Range(sp.front().start, img.cols));

            img_bin = img(cv::Range(_baseline_h, img.rows), cv::Range(0, img.cols));
            cv::cvtColor(img_bin, img_bin, cv::COLOR_BGR2GRAY);
            cv::threshold(img_bin, img_bin, 200, 255, cv::THRESH_BINARY);
            sp = separate(img_bin, DirectionFlags::LEFT);
            int right_edge = sp.back().end + sp.front().start - static_cast<int>(round(_baseline_h * 0.47));
            _img = img(cv::Range(0, img.rows), cv::Range(0, right_edge));
            _get_abs_pos();
        }
    }

    void set_item_diameter(int diameter)
    {
        _item_diameter = diameter;
    }
    Widget_DropAct24& analyze(const std::string& stage)
    {
        if (!_img.empty())
        {
            _get_drops(stage);
        }
        else
        {
            // add img empty exc
        }
        return *this;
    }
    const dict report(bool debug = false)
    {
        dict rpt = dict::object();
        rpt.merge_patch(Widget::report(debug));

        rpt["drops"] = dict::array();

        size_t drops_count = _drop_list.size(); // will move in "for" in C++20
        for (int i = 0; i < drops_count; i++)
        {
            rpt["drops"].push_back({{"dropType", "NORMAL_DROP"}});
            rpt["drops"][i].merge_patch(_drop_list[i].report(debug));
        }
        return rpt;
    }

private:
    int _item_diameter = 0;
    int _baseline_h = 0;
    dict _drops_data;
    std::vector<Widget_ItemAct24> _drop_list;

    void _get_drops(const std::string& stage)
    {
        if (_status == StatusFlags::HAS_ERROR || _status == StatusFlags::ERROR)
        {
            return;
        }

        cv::Mat img_bin = _img(cv::Range(0, _baseline_h), cv::Range(0, width));
        cv::cvtColor(img_bin, img_bin, cv::COLOR_BGR2GRAY);
        cv::threshold(img_bin, img_bin, 127, 255, cv::THRESH_BINARY);
        auto sp = separate(img_bin, DirectionFlags::LEFT);
        sp.erase(
            std::remove_if(sp.begin(), sp.end(),
                           [&](const cv::Range& range) {
                               int length = range.end - range.start;
                               return length < _item_diameter * 0.95 || length > _item_diameter * 1.05;
                           }),
            sp.end());

        ItemTemplates templs {stage};
        if (templs.templ_list().empty())
        {
            widget_label = "act24drops";
            push_exception(ERROR, ExcSubtypeFlags::EXC_ILLEGAL, "Empty templetes");
            return;
        }
        else
        {
            for (auto& range : sp)
            {
                std::string label = "act24drops." + std::to_string(_drop_list.size());
                auto dropimg = _img(cv::Range(0, _baseline_h), cv::Range(range.start - 5, range.end + 5));
                Widget_ItemAct24 drop {dropimg, _item_diameter, label, this};
                drop.analyze(templs);
                _drop_list.emplace_back(drop);
                _drops_data.push_back({{"dropType", "NORMAL_DROP"},
                                       {"itemId", drop.itemId()},
                                       {"quantity", drop.quantity()}});
            }
        }
    }
};

} // namespace penguin

#endif // PENGUIN_ACT24_HPP_
