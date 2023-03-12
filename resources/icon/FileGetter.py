import hashlib
import io
import json
import warnings
import zipfile
from contextlib import contextmanager

import requests


class FileGetter:

    _NETWORK_CONFIG_URLS = {
        'CN': 'https://ak-conf.hypergryph.com/config/prod/official/network_config',
        'JP': 'https://ak-conf.arknights.jp/config/prod/official/network_config',
        'US': 'https://ak-conf.arknights.global/config/prod/official/network_config',
        'KR': 'https://ak-conf.arknights.kr/config/prod/official/network_config',
        'TW': 'https://ak-conf.txwy.tw/config/prod/official/network_config',
    }

    def __init__(self, server) -> None:
        self.server = server
        self._platform = "Android"
        self._network_config = self._get_network_config()
        self._version = self._get_version()
        self._file_list = self._get_file_list()

    @contextmanager
    def get(self, filename):
        z = self._get_file(filename)
        with z.open(filename) as f:
            h = hashlib.md5()
            while _ := f.read():
                h.update(_)
            if h.hexdigest() != self._file_list[filename]:
                warnings.warn("MD5 not match")
            yield f

    def _get_file(self, filename: str):
        if filename in self._file_list:
            url = self._root_url + \
                (filename.rsplit('.', 1)[0] + ".dat")\
                .replace('/', '_')\
                .replace('#', '__')
        return zipfile.ZipFile(io.BytesIO(requests.get(url).content), 'r')

    @staticmethod
    def _get_json(json_url: str):
        res = requests.get(json_url).content.decode()
        return json.loads(res)

    def _get_network_config(self) -> dict:
        network_config_url = FileGetter._NETWORK_CONFIG_URLS[self.server]
        network_config = self._get_json(network_config_url)
        return json.loads(network_config["content"])

    def _get_version(self) -> str:
        funcVer = self._network_config["funcVer"]
        self.version_url = self._network_config["configs"][funcVer]["network"]["hv"]\
            .replace("{0}", self._platform)
        return self._get_json(self.version_url)["resVersion"]

    def _get_file_list(self) -> dict:
        funcVer = self._network_config["funcVer"]
        self._root_url = self._network_config["configs"][funcVer]["network"]["hu"]\
            + f"/{self._platform}/assets/{self._version}/"
        self.file_list_url = self._root_url + "hot_update_list.json"
        file_arr: list = self._get_json(self.file_list_url)["abInfos"]
        return {_["name"]: _["md5"] for _ in file_arr}


if __name__ == "__main__":
    fg = FileGetter("KR")
    with fg.get("gamedata/excel/item_table.ab") as f:
        import UnityPy
        env = UnityPy.load(f)
        print(fg._get_json(fg.version_url))
