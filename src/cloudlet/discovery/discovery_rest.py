#!/usr/bin/env python

import os
import json

from flask import Flask
from flask.ext import restful
from flask.ext.restful import reqparse
from flask.ext.restful import Resource
from flask.ext.restful import abort
#from cloudlet.discovery.monitor import resource
from monitor import resource

from cloudlet.discovery.Const import RESTConst


class ResourceInfo(Resource):
    def __init__(self, *args, **kwargs):
        super(ResourceInfo, self).__init__(*args, **kwargs)

    def get(self):
        resource_monitor = resource.get_instance()
        ret_data = resource_monitor.get_static_resource()
        ret_data.update(resource_monitor.get_dynamic_resource())
        return json.dumps(ret_data)


class CacheInfo(Resource):
    parser = reqparse.RequestParser()
    parser.add_argument('app_id', type=str)

    def __init__(self, *args, **kwargs):
        super(CacheInfo, self).__init__(*args, **kwargs)
        self.dfs_root = RESTConst.DFS_ROOT_DIR

    def get(self, app_id):
        if app_id is None:
            abort(404, message ="Need application id to check cache status")

        query_path = os.path.join(self.dfs_root, app_id)
        query_path = os.path.abspath(query_path)
        if os.path.exists(query_path) is False:
            ret_json = {app_id:"Not valid"}
        else:
            if os.path.isdir(query_path) is True:
                score = self._cache_score_for_dir(query_path)
                ret_json = {app_id:int(score)}
            elif os.path.isfile(query_path) is True:
                score = self._cache_score_for_file(query_path)
                ret_json = {app_id:int(score)}
            else:
                ret_json = {app_id:None}
        return json.dumps(ret_json)

    def _cache_score_for_file(self, filepath):
        return 100

    def _cache_score_for_dir(self, dirpath):
        return 100


if __name__ == "__main__":
    try:
        # run REST server
        app = Flask(__name__)
        api = restful.Api(app)
        api.add_resource(ResourceInfo, '/api/v1/resource/')
        api.add_resource(CacheInfo, '/api/v1/resource/<string:app_id>')
        # do no turn on debug mode. it make a mess for graceful terminate
        app.run(host="0.0.0.0", port=8022, threaded=True, debug=True)
    except KeyboardInterrupt as e:
        ret_code = 1
    finally:
        pass
