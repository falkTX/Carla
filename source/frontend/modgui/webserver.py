#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Carla bridge for LV2 modguis
# Copyright (C) 2015-2020 Filipe Coelho <falktx@falktx.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# For a full copy of the GNU General Public License see the doc/GPL.txt file.

# ------------------------------------------------------------------------------------------------------------
# Imports (Global)

import os

from PyQt5.QtCore import pyqtSignal, QThread

# ------------------------------------------------------------------------------------------------------------
# Generate a random port number between 9000 and 18000

from random import random

PORTn = 8998 + int(random()*9000)

# ------------------------------------------------------------------------------------------------------------
# Imports (asyncio)

try:
    from asyncio import new_event_loop, set_event_loop
    haveAsyncIO = True
except:
    haveAsyncIO = False

# ------------------------------------------------------------------------------------------------------------
# Imports (tornado)

from tornado.log import enable_pretty_logging
from tornado.ioloop import IOLoop
from tornado.util import unicode_type
from tornado.web import HTTPError
from tornado.web import Application, RequestHandler, StaticFileHandler

# ------------------------------------------------------------------------------------------------------------
# Set up environment for the webserver

PORT     = str(PORTn)
ROOT     = "/usr/share/mod"
DATA_DIR = os.path.expanduser("~/.local/share/mod-data/")
HTML_DIR = os.path.join(ROOT, "html")

os.environ['MOD_DEV_HOST'] = "1"
os.environ['MOD_DEV_HMI']  = "1"
os.environ['MOD_DESKTOP']  = "1"

os.environ['MOD_DATA_DIR']           = DATA_DIR
os.environ['MOD_HTML_DIR']           = HTML_DIR
os.environ['MOD_KEY_PATH']           = os.path.join(DATA_DIR, "keys")
os.environ['MOD_CLOUD_PUB']          = os.path.join(ROOT, "keys", "cloud_key.pub")
os.environ['MOD_PLUGIN_LIBRARY_DIR'] = os.path.join(DATA_DIR, "lib")

os.environ['MOD_PHANTOM_BINARY']        = "/usr/bin/phantomjs"
os.environ['MOD_SCREENSHOT_JS']         = os.path.join(ROOT, "screenshot.js")
os.environ['MOD_DEVICE_WEBSERVER_PORT'] = PORT

# ------------------------------------------------------------------------------------------------------------
# Imports (MOD)

from modtools.utils import get_plugin_info, get_plugin_gui, get_plugin_gui_mini

# ------------------------------------------------------------------------------------------------------------
# MOD related classes

class JsonRequestHandler(RequestHandler):
    def write(self, data):
        if isinstance(data, (bytes, unicode_type, dict)):
            RequestHandler.write(self, data)
            self.finish()
            return

        elif data is True:
            data = "true"
            self.set_header("Content-Type", "application/json; charset=UTF-8")

        elif data is False:
            data = "false"
            self.set_header("Content-Type", "application/json; charset=UTF-8")

        else:
            data = json.dumps(data)
            self.set_header("Content-Type", "application/json; charset=UTF-8")

        RequestHandler.write(self, data)
        self.finish()

class EffectGet(JsonRequestHandler):
    def get(self):
        uri = self.get_argument('uri')

        try:
            data = get_plugin_info(uri)
        except:
            print("ERROR: get_plugin_info for '%s' failed" % uri)
            raise HTTPError(404)

        self.write(data)

class EffectFile(StaticFileHandler):
    def initialize(self):
        # return custom type directly. The browser will do the parsing
        self.custom_type = None

        uri = self.get_argument('uri')

        try:
            self.modgui = get_plugin_gui(uri)
        except:
            raise HTTPError(404)

        try:
            root = self.modgui['resourcesDirectory']
        except:
            raise HTTPError(404)

        return StaticFileHandler.initialize(self, root)

    def parse_url_path(self, prop):
        try:
            path = self.modgui[prop]
        except:
            raise HTTPError(404)

        if prop in ("iconTemplate", "settingsTemplate", "stylesheet", "javascript"):
            self.custom_type = "text/plain"

        return path

    def get_content_type(self):
        if self.custom_type is not None:
            return self.custom_type
        return StaticFileHandler.get_content_type(self)

class EffectResource(StaticFileHandler):

    def initialize(self):
        # Overrides StaticFileHandler initialize
        pass

    def get(self, path):
        try:
            uri = self.get_argument('uri')
        except:
            return self.shared_resource(path)

        try:
            modgui = get_plugin_gui_mini(uri)
        except:
            raise HTTPError(404)

        try:
            root = modgui['resourcesDirectory']
        except:
            raise HTTPError(404)

        try:
            super(EffectResource, self).initialize(root)
            return super(EffectResource, self).get(path)
        except HTTPError as e:
            if e.status_code != 404:
                raise e
            return self.shared_resource(path)
        except IOError:
            raise HTTPError(404)

    def shared_resource(self, path):
        super(EffectResource, self).initialize(os.path.join(HTML_DIR, 'resources'))
        return super(EffectResource, self).get(path)

# ------------------------------------------------------------------------------------------------------------
# WebServer Thread

class WebServerThread(QThread):
    # signals
    running = pyqtSignal()

    def __init__(self, parent=None):
        QThread.__init__(self, parent)

        self.fApplication = Application(
            [
                (r"/effect/get/?", EffectGet),
                (r"/effect/file/(.*)", EffectFile),
                (r"/resources/(.*)", EffectResource),
                (r"/(.*)", StaticFileHandler, {"path": HTML_DIR}),
            ],
            debug=True)

        self.fPrepareWasCalled = False
        self.fEventLoop = None

    def run(self):
        if not self.fPrepareWasCalled:
            self.fPrepareWasCalled = True
            if haveAsyncIO:
                self.fEventLoop = new_event_loop()
                set_event_loop(self.fEventLoop)
            self.fApplication.listen(PORT, address="0.0.0.0")
            if int(os.getenv("MOD_LOG", "0")):
                enable_pretty_logging()

        self.running.emit()
        IOLoop.instance().start()

    def stopWait(self):
        IOLoop.instance().stop()
        if self.fEventLoop is not None:
            self.fEventLoop.call_soon_threadsafe(self.fEventLoop.stop)
        return self.wait(5000)
