import os
import shutil
import tempfile

import cv2
import numpy
import UnityPy
from PIL import Image

from FileGetter import FileGetter
from decrypt import decrypt


class iconGetter:

    file_list = [
        "spritepack/ui_item_icons_h1_0.ab",  # basic items
        "spritepack/ui_item_icons_h1_acticon_0.ab",  # randomMaterial
        "spritepack/ui_item_icons_h1_apsupply_0.ab",  # ap supply
        "activity/commonassets.ab",  # activity items
        "activity/[uc]act24side.ab"  # activity items
    ]

    def __init__(self, server) -> None:
        self.server = server
        self.fg = FileGetter(server)
        print(self.fg.server, self.fg._version)
        print(self.fg.file_list_url)
        self.item_table = self._get_item_table()
        print(f"item_table: {len(self.item_table)}")
        self.item_list = self._get_item_list()
        print(f"item_list: {len(self.item_list)}")
        self.bkg_img_list = self._get_bkg_img()
        print(f"bkg_img_list: {len(self.bkg_img_list)}")
        self.item_img_list = self._get_item_img()
        print(f"item_img_list: {len(self.item_img_list)}")

    def make_zip(self, path, hq=False):
        with tempfile.TemporaryDirectory() as temp:
            if hq:
                for itemId, pilimg in self.item_img_list.items():
                    cv2img = cv2.cvtColor(numpy.array(
                        pilimg), cv2.COLOR_RGBA2BGRA)
                    cv2.imwrite(temp + "/" + itemId + ".png", cv2img)
            else:
                for itemId, pilimg in self.item_img_list.items():
                    cv2img = cv2.cvtColor(numpy.array(
                        pilimg), cv2.COLOR_RGBA2BGRA)
                    cv2.imwrite(temp + "/" + itemId + ".jpg", cv2img,
                                [int(cv2.IMWRITE_JPEG_QUALITY), 10])
            fname = "items_" + self.server + "_" + self.fg._version
            shutil.make_archive(os.path.join(path, fname), "zip", temp)

    def _get_item_table(self):
        res = {}
        with self.fg.get("gamedata/excel/item_table.ab") as f:
            env = UnityPy.load(f)
            for obj in env.objects:
                if obj.type.name == "TextAsset":
                    textAssetFile = obj.read()
                    item_table = decrypt(textAssetFile)
                    for _, item in item_table["items"].items():
                        itemId = item["itemId"]
                        rarity = item["rarity"]
                        iconId = item["iconId"]
                        res[itemId] = {
                            "iconId": iconId, "rarity": rarity}
                    break
        return res

    def _get_item_list(self):
        item_set = set()
        penguin_stages = self.fg._get_json(
            "https://penguin-stats.io/PenguinStats/api/v2/stages?server=" + self.server)
        for stage in penguin_stages:
            if (stage["stageId"] == "recruit") or ("isGacha" in stage):
                continue
            if "dropInfos" in stage:
                for item in stage["dropInfos"]:
                    if ("itemId" in item):
                        item_set.add(item["itemId"])
            if "recognitionOnly" in stage:
                for item in stage["recognitionOnly"]:
                    item_set.add(item)

        res = {}
        for itemId in item_set:
            if (itemId == "furni") or (itemId == "4001_2000"):
                continue
            iconId = self.item_table[itemId]["iconId"]
            rarity = self.item_table[itemId]["rarity"]
            if iconId not in res:
                res[iconId] = []
            res[iconId].append({"itemId": itemId, "rarity": rarity})
        return res

    @staticmethod
    def _get_bkg_img():
        res = {"item": {}, "charm": {}, "act24": {}}
        for i in range(6):
            img = Image.open(
                os.path.join(os.path.dirname(__file__), f"background\\sprite_item_r{i}.png"))
            res["item"][i] = img
        for i in range(4):
            img = Image.open(
                os.path.join(os.path.dirname(__file__), f"background\\charm_r{i}.png"))
            res["charm"][i] = img
        for i in range(6):
            img = Image.open(
                os.path.join(os.path.dirname(__file__), f"background\\act24side_melding_{i+1}_bg.png"))
            res["act24"][i] = img
        return res

    def _get_item_img(self):
        res = {}
        for fname in iconGetter.file_list:
            with self.fg.get(fname) as f:
                env = UnityPy.load(f)
                for obj in env.objects:
                    if obj.type.name == "Sprite":
                        spriteFile = obj.read()
                        if (spriteFile.name in self.item_list):
                            item_img = spriteFile.image
                            for item in self.item_list[spriteFile.name]:
                                itemId = item["itemId"]
                                rarity = item["rarity"]
                                if "act24" in itemId:
                                    bkg_img: Image.Image = self.bkg_img_list["act24"][rarity].copy(
                                    )
                                else:
                                    bkg_img: Image.Image = self.bkg_img_list["item"][rarity].copy(
                                    )

                                offset_ld = {"x": 0, "y": 0}
                                m_Rect = spriteFile.m_Rect
                                offset_ld["x"] += (bkg_img.width -
                                                   m_Rect.width) / 2.0
                                offset_ld["y"] += (bkg_img.height -
                                                   m_Rect.height) / 2.0

                                textureRectOffset = spriteFile.m_RD.textureRectOffset
                                offset_ld["x"] += textureRectOffset.X
                                offset_ld["y"] += textureRectOffset.Y

                                offset = (
                                    round(offset_ld["x"]),
                                    bkg_img.height - item_img.height - round(offset_ld["y"]))
                                bkg_img.alpha_composite(item_img, offset)
                                res[itemId] = bkg_img
        return res


if __name__ == "__main__":
    ig = iconGetter("CN")
    ig.make_zip("D:/R", hq=False)
