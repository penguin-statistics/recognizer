import json
import shutil
import string
import tempfile
import zipfile

import cv2
import numpy
import requests
import UnityPy
from PIL import Image


class Extractor:
    def __init__(self, apk_path) -> None:
        self.apk_path = apk_path
        self.bkgimg_path_list = ["assets/bin/Data/sharedassets1.assets"]
        self.itemimg_path_list = [
            "assets/AB/Android/spritepack/ui_item_icons_h1_0.ab",  # basic items
            "assets/AB/Android/spritepack/ui_item_icons_h1_acticon_0.ab",  # randomMaterial
            "assets/AB/Android/spritepack/ui_item_icons_h1_apsupply_0.ab",  # ap supply
            "assets/AB/Android/activity/commonassets.ab"  # activity items
        ]
        self.item_list: dict = {}
        self.bkgimg_list: dict = {"item": {}, "charm": {}}
        self.itemimg_list: dict = {}
        with open("filter.json", 'r') as f:
            self.filter = json.loads(f.read())

    def extract(self):
        self._getItemList()
        self._getBkgImg()
        self._getItemImg()

    def _getFile(self, path: string = ""):
        z = zipfile.ZipFile(self.apk_path, 'r')
        if not path:
            return z
        else:
            return z.open(path, 'r')

    def _getItemList(self):
        url = "https://raw.githubusercontent.com/Kengxxiao/ArknightsGameData/master/zh_CN/gamedata/excel/item_table.json"
        with requests.get(url) as res:
            item_table: dict = json.loads(res.text)
        for _, item in item_table["items"].items():
            itemId = item["itemId"]
            rarity = item["rarity"]
            iconId = item["iconId"]
            if not (iconId in self.item_list):
                self.item_list[iconId] = {"itemId": itemId, "rarity": rarity}

    def _getBkgImg(self):
        apk = self._getFile()
        for p in self.bkgimg_path_list:
            count = 0
            for f in apk.namelist():
                if p + ".split" in f:
                    count = count + 1
            with tempfile.TemporaryFile() as outfile:
                for i in range(count):
                    with self._getFile(p + ".split" + str(i)) as infile:
                        outfile.write(infile.read())
                env = UnityPy.load(outfile)
                for obj in env.objects:
                    if obj.type == "Sprite":
                        data = obj.read()
                        if "sprite_item_r" in data.name:
                            self.bkgimg_list["item"][int(data.name[-1]) -
                                                     1] = data.image
                        elif "charm_r" in data.name:
                            self.bkgimg_list["charm"][int(
                                data.name[-1])] = data.image

    def _getItemImg(self):
        for p in self.itemimg_path_list:
            with self._getFile(p) as f:
                env = UnityPy.load(f)
                for obj in env.objects:
                    if obj.type == "Sprite":
                        data = obj.read()
                        if (data.name in self.item_list):
                            itemId = self.item_list[data.name]["itemId"]
                            if itemId in self.filter:
                                continue
                            item_img: Image.Image = data.image
                            rarity = self.item_list[data.name]["rarity"]
                            bkg_img: Image.Image = self.bkgimg_list["item"][
                                rarity].copy()

                            offset_old = data.m_RD.textureRectOffset
                            x = round(offset_old.X)
                            y = bkg_img.height - item_img.height - \
                                round(offset_old.Y)
                            offset = (x+1, y+1)

                            bkg_img.alpha_composite(item_img, offset)
                            self.itemimg_list[itemId] = bkg_img

        if "randomMaterial_1" in self.itemimg_list:
            self.itemimg_list["randomMaterial_6"] = self.itemimg_list["randomMaterial_1"]


if __name__ == "__main__":
    apk_path = input("drag the arknights apk here or input the path:\n")
    e = Extractor(apk_path)
    e.extract()

    with tempfile.TemporaryDirectory() as temp:
        for itemId, pilimg in e.itemimg_list.items():
            cv2img = cv2.cvtColor(numpy.array(pilimg), cv2.COLOR_RGBA2BGRA)
            cv2.imwrite(temp + "/" + itemId + ".jpg", cv2img,
                        [int(cv2.IMWRITE_JPEG_QUALITY), 10])
        shutil.make_archive("items", 'zip', temp)
