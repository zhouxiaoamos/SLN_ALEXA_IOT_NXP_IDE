"""

Copyright 2019 NXP.

This software is owned or controlled by NXP and may only be used strictly in accordance with the
license terms that accompany it. By expressly accepting such terms or by downloading, installing,
activating and/or otherwise using the software, you are agreeing that you have read, and that you
agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
applicable license terms, then you may not retain, install, activate or otherwise use the software.

File
++++
CodeGenerator/__init__.py

Brief
+++++
** Wrapper for Generic Code Generator **

.. versionadded:: 0.0

API
+++

"""

import os
import platform
import subprocess
import json
import logging
import re

#logging.basicConfig(level=logging.DEBUG)

DEBUG_LOG = False

class CodeGenerator(object):
    """
        CodeGenerator code generator


    """

    def __init__(self, filename):
        """
            :param CodeGen: Code Generation Object
            :type dev: String

            :returns: None
        """
        self.__filename = filename
        #CodeGen.__init__(self, "discovery_endpoint_gen.c", "discovery_endpoint_gen.h")

    def write(self, content):
        success = True

        try:
            f = open(self.__filename, "w")

            for line in content:
                f.write(line)
                f.write("\n")
        except:
            success = False
        return success

    def read(self):
        content = []

        f = open(self.__filename, "r")
        content = f.readlines()

        return content

    def get_filename(self):
        return self.__filename

