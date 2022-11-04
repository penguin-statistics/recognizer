import fnmatch
import json
import os
import shutil
import sys
import tempfile
import cv2
import numpy

import requests
import UnityPy
from PIL import Image

import FileGetter


def update():
    _get_item_list()
    _get_bkg_img()
    _get_item_img()


def _get_item_list():
    url = "https://raw.githubusercontent.com/Kengxxiao/ArknightsGameData/master/zh_CN/gamedata/excel/item_table.json"
    with requests.get(url) as res:
        item_table: dict = json.loads(res.text)
    for _, item in item_table["items"].items():
        itemId = item["itemId"]
        rarity = item["rarity"]
        iconId = item["iconId"]
        if not (iconId in item_list):
            item_list[iconId] = {"itemId": itemId, "rarity": rarity}


def _get_bkg_img():
    for i in range(6):
        img = Image.open(f"background\\sprite_item_r{i}.png")
        bkgimg_list["item"][i] = img


def _get_item_img():
    for filename in name_list:
        with FileGetter.get(filename) as f:
            env = UnityPy.load(f)
            for obj in env.objects:
                if obj.type == "Sprite":
                    data = obj.read()
                    if (data.name in item_list):
                        itemId = item_list[data.name]["itemId"]
                        if any([fnmatch.fnmatch(itemId, pattern) for pattern in filter]):
                            continue
                        item_img: Image.Image = data.image
                        rarity = item_list[data.name]["rarity"]
                        bkg_img: Image.Image = bkgimg_list["item"][
                            rarity].copy()

                        offset_old = data.m_RD.textureRectOffset
                        x = round(offset_old.X)
                        y = bkg_img.height - item_img.height - \
                            round(offset_old.Y)
                        offset = (x+1, y+1)

                        bkg_img.alpha_composite(item_img, offset)
                        itemimg_list[itemId] = bkg_img

    if "randomMaterial_1" in itemimg_list:
        itemimg_list["randomMaterial_5"] = itemimg_list["randomMaterial_1"]
        itemimg_list["randomMaterial_6"] = itemimg_list["randomMaterial_1"]
        itemimg_list["randomMaterial_7"] = itemimg_list["randomMaterial_1"]


os.chdir(sys.path[0])

name_list = [
    "ui/gacha/limited_33_0_1.ab",  # basic items
    "spritepack/ui_item_icons_h1_acticon_0.ab",  # randomMaterial
    "spritepack/ui_item_icons_h1_apsupply_0.ab",  # ap supply
    "activity/commonassets.ab"  # activity items
]

item_list = {}
bkgimg_list = {"item": {}, "charm": {}}
itemimg_list: dict = {}
with open("filter.json", 'r') as f:
    filter = json.loads(f.read())

update()

with tempfile.TemporaryDirectory() as temp:
    for itemId, pilimg in itemimg_list.items():
        pilimg.save(f"items_HD/{itemId}.png")
        cv2img = cv2.cvtColor(numpy.array(pilimg), cv2.COLOR_RGBA2BGRA)
        cv2.imwrite(temp + "/" + itemId + ".jpg", cv2img,
                    [int(cv2.IMWRITE_JPEG_QUALITY), 10])
    shutil.make_archive("items", 'zip', temp)
