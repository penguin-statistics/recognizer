import hashlib
import io
import json
import warnings
import zipfile
from contextlib import contextmanager

import requests


@contextmanager
def get(filename):
    z = _get_file(filename)
    with z.open(filename) as f:
        h = hashlib.md5()
        while _ := f.read():
            h.update(_)
        if h.hexdigest() != file_list[filename]:
            warnings.warn("MD5 not match")
        yield f


def _get_json(json_url: str):
    res = requests.get(json_url).content.decode()
    return json.loads(res)


def _get_network_config() -> dict:
    network_config_url = "https://ak-conf.hypergryph.com/config/prod/official/network_config"
    network_config = _get_json(network_config_url)
    return json.loads(network_config["content"])


def _get_version() -> str:
    funcVer = network_config["funcVer"]
    version_url = network_config["configs"][funcVer]["network"]["hv"]\
        .replace("{0}", platform)
    return _get_json(version_url)["resVersion"]


def _get_file_list() -> dict:
    global root_url
    funcVer = network_config["funcVer"]
    root_url = network_config["configs"][funcVer]["network"]["hu"]\
        + f"/{platform}/assets/{version}/"
    file_list_url = root_url + "hot_update_list.json"
    file_arr: list = _get_json(file_list_url)["abInfos"]
    return {_["name"]: _["md5"] for _ in file_arr}


def _get_file(filename: str):
    if filename in file_list:
        url = root_url + \
            (filename.rsplit('.', 1)[0] + ".dat")\
            .replace('/', '_')\
            .replace('#', '__')
    return zipfile.ZipFile(io.BytesIO(requests.get(url).content), 'r')


platform = "Android"

root_url = ""
network_config = _get_network_config()
version = _get_version()
file_list = _get_file_list()

_get_network_config()
_get_version()
_get_file_list()
