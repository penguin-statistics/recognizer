import json
import requests
import sys


class Maker():
    item_url = "https://penguin-stats.io/PenguinStats/api/v2/items"
    stage_url = "https://penguin-stats.io/PenguinStats/api/v2/stages?server=CN"

    def __init__(self) -> None:
        self.item_index = {}
        self.stage_index = {}
        self._get_itemindex()
        self._get_stageindex()

    def _get_itemindex(self):
        with requests.get(self.item_url) as f:
            item_table = json.loads(f.text)
        for item in item_table:
            itemId = item["itemId"]
            name_i18n = item["name_i18n"]
            self.item_index[itemId] = {"name_i18n": name_i18n}

    def _get_stageindex(self):
        with requests.get(self.stage_url) as f:
            stage_table = json.loads(f.text)
        for stage in stage_table:
            code = stage["code"]
            stageId = stage["stageId"]
            if stageId[:5] == "tough":
                difficulty = "TOUGH"
            else:
                difficulty = "NORMAL"
            drops = []
            if "dropInfos" in stage:
                for item in stage["dropInfos"]:
                    if "itemId" in item:
                        if item["itemId"] != "furni":
                            drops.append(item["itemId"])
            if "recognitionOnly" in stage:
                for itemId in stage["recognitionOnly"]:
                    drops.append(itemId)
            if not (code in self.stage_index):
                self.stage_index[code] = {}
            self.stage_index[code][difficulty] = {
                "stageId": stageId,
                "drops": list(set(drops)),
                "existence": True
            }


if __name__ == "__main__":
    current_path = sys.path[0] + "\\"
    m = Maker()
    with open(current_path + "item_index.json", 'w', encoding="utf-8") as f:
        f.write(json.dumps(m.item_index, ensure_ascii=False))
    with open(current_path + "stage_index.json", 'w', encoding="utf-8") as f:
        f.write(json.dumps(m.stage_index, ensure_ascii=False))
