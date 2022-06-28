"""

Copyright 2019 NXP.

This software is owned or controlled by NXP and may only be used strictly in accordance with the
license terms that accompany it. By expressly accepting such terms or by downloading, installing,
activating and/or otherwise using the software, you are agreeing that you have read, and that you
agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
applicable license terms, then you may not retain, install, activate or otherwise use the software.

File
++++
discovery_endpoint_gen/__init__.py

Brief
+++++
** Wrapper for Discovery Endpoint Code Generator **

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
import CodeGenerator as CodeGen

#logging.basicConfig(level=logging.DEBUG)

DEBUG_LOG = False

class DiscoveryEndpointGen(object):
    """
        DiscovertEndpointGen code generator


    """

    def __init__(self):
        """
            :param CodeGen: Code Generation Object
            :type dev: String

            :returns: None
        """
        self.__c_generator = CodeGen.CodeGenerator("discovery_endpoint_gen.c")
        self.__h_generator = CodeGen.CodeGenerator("discovery_endpoint_gen.h")

    def __get_name_in_c(self, thing, name):
        if isinstance(thing, str):
            return "\tchar " + "\t\t\t\t\tavs_sh_" + name + "[" + str(len(thing)) + "];"
        if isinstance(thing, int):
            return "\tuint32_t " + "\t\t\t\t\tavs_sh_" + name + ";"
        if isinstance(thing, bool):
            return "\tbool " + "\t\t\t\t\tavs_sh_" + name + ";"

        return "What Happened?"
        print(str(type(thing)))

    def __get_value_in_c(self, thing, name):
        if isinstance(thing, str):
            return "\"" + thing + "\""
        if isinstance(thing, int):
            return str(thing)
        if isinstance(thing, bool):
            return str(thing)

        return "What Happened?"
        print(str(type(thing)))

    def __item_struct_generator(self, json_input, name, item_callback, struct_contents):
        if isinstance(json_input, dict):
            contents = []
            struct_line = "typedef struct _avs_smart_home_" + name
            if struct_line in struct_contents:
                print("Struct already Exists")
            else:
                contents.append("typedef struct _avs_smart_home_" + name)
                contents.append("{")
                for k, v in json_input.items():
                    line = self.__item_struct_generator(v, k, item_callback, struct_contents)
                    contents.append(line)
                contents.append("} avs_smart_home_" + name + ";")
                contents.append("\n")
                struct_contents.extend(contents)

            return "\tavs_smart_home_" + name + "\tavs_sh_" + name + ";"
        elif isinstance(json_input, list):
            for item in json_input:
                self.__item_struct_generator(item, name, item_callback, struct_contents)
            return "\tavs_smart_home_" + name + "\tavs_sh_" + name + "[" + str(len(json_input)) + "];"
        else:
            return self.__get_name_in_c(json_input, name)
            
    def __generate_structure_initialization(self, json_input, name, item_callback, depth):
        contents = "\\\n"
        tabs_start = ""
        tabs_end = ""
        for i in range(0, depth):
            tabs_start = tabs_start + "\t"
            
        for i in range(0, depth - 1):
            tabs_end = tabs_end + "\t"
        
        contents = contents + tabs_start
        depth+=1

        if isinstance(json_input, dict):
            contents = contents + "{"
            first = True
            for k, v in json_input.items():
                if first == False:
                    contents = contents + ", "
                line = self.__generate_structure_initialization(v, k, item_callback, depth)
                contents = contents + line
                first = False
            contents = contents + "\n" + tabs_start + "}\\"

            return contents
        elif isinstance(json_input, list):
            contents = contents + "{"
            first = True
            for item in json_input:
                if first == False:
                    contents = contents + ", "

                contents = contents + self.__generate_structure_initialization(item, name, item_callback, depth)
                first = False
            contents = contents + "}\\"
            return contents
        else:
            contents = ""
            contents = contents + self.__get_value_in_c(json_input, name)
            return contents

    def __generate_header(self, json_input, name, item_callback):
        content = []

        content.append("/*")
        content.append("* Copyright 2020 NXP.")
        content.append("* This software is owned or controlled by NXP and may only be used strictly in accordance with the")
        content.append("* license terms that accompany it. By expressly accepting such terms or by downloading, installing,")
        content.append("* activating and/or otherwise using the software, you are agreeing that you have read, and that you")
        content.append("* agree to comply with and are bound by, such license terms. If you do not agree to be bound by the")
        content.append("* applicable license terms, then you may not retain, install, activate or otherwise use the software.")
        content.append("*/")

        content.append("")

        content.append("/***********************************************************************************************************************")
        content.append("* This file was generated by the NXP AVS Smart Home Conversion Tool. Any manual edits made to this file")
        content.append("* will be overwritten if the respective MCUXpresso Build option is used")
        content.append("**********************************************************************************************************************/")

        content.append("")
        

        content.append("/** @file " + self.__h_generator.get_filename())
        content.append(" */")

        content.append("")

        content.append("#ifndef _" + self.__h_generator.get_filename().replace(".", "_").upper() + "_")
        content.append("#define _" + self.__h_generator.get_filename().replace(".", "_").upper() + "_")

        content.append("")

        content.append("#if defined(__cplusplus)")
        content.append("extern \"C\" {")
        content.append("#endif")
        
        content.append("")

        content.append("/*******************************************************************************")
        content.append("* Includes")
        content.append("******************************************************************************/")
        
        content.append("")

        content.append("#include \"cJSON.h\"")

        content.append("")

        content.append("/*******************************************************************************")
        content.append("* Definitions")
        content.append("******************************************************************************/")

        #content.append(self.__item_struct_generator(json_input, name, item_callback, content))

        #content.append("")

        content.append("extern const char avs_smart_home_json_string[];")

        content.append("")

        content.append("/*******************************************************************************")
        content.append("* Public Functions")
        content.append("******************************************************************************/")

        content.append("#if defined(__cplusplus)")
        content.append("}")
        content.append("#endif")

        content.append("")

        content.append("#endif /* _" + self.__h_generator.get_filename().replace(".", "_").upper() + "_ */")

        return content
        # Generate the license header and template

    def __generate_source_file(self, json_input, name, item_callback):
        content = []

        content.append("/*")
        content.append("* Copyright 2020 NXP.")
        content.append("* This software is owned or controlled by NXP and may only be used strictly in accordance with the")
        content.append("* license terms that accompany it. By expressly accepting such terms or by downloading, installing,")
        content.append("* activating and/or otherwise using the software, you are agreeing that you have read, and that you")
        content.append("* agree to comply with and are bound by, such license terms. If you do not agree to be bound by the")
        content.append("* applicable license terms, then you may not retain, install, activate or otherwise use the software.")
        content.append("*/")

        content.append("")

        content.append("/*******************************************************************************")
        content.append("* Includes")
        content.append("******************************************************************************/")
        
        content.append("")

        content.append("/*******************************************************************************")
        content.append("* Definitions")
        content.append("******************************************************************************/")
        
        content.append("")
        
        json_str = json.dumps(json_input).replace("\"", "\\\"")
        formated_json_str = '\" \\\n\t\t\t\t\t\t\t\t\t\t\"'.join(json_str[i:i+120] for i in range(0, len(json_str), 120))

        content.append("const char avs_smart_home_json_string[] = \"" + (formated_json_str + "\";"))

        content.append("")

        #struct_return = self.__generate_structure_initialization(json_input, name, item_callback, 0)

        #content.append(struct_return)

        #content.append("")

        content.append("/*******************************************************************************")
        content.append("* Public Functions")
        content.append("******************************************************************************/")
        
        content.append("")

        return content
        # Generate the license header and template

    def __generate_structure_header(self, json_contents):
        """
            Private function to determine if an extension must be added to executable calls

            :returns: (str) Appropriate file extension, if needed
        """
        ext = '.exe' if useExe(prog_name) else ''
        return ext

    def __generate_initialized_struct_source(self, json_contents):
        """
            Private function to determine if an extension must be added to executable calls

            :returns: (str) Appropriate file extension, if needed
        """
        ext = '.exe' if useExe(prog_name) else ''
        return ext


    def execute(self, json_input, name, item_callback):
        """
            Execute application at specified entry point

            :param entry: Absolute address of app entry point
            :type entry: hexadecimal string
            :param param: Parmerter to pass into R0
            :type param: hexadecimal string
            :param stack: Initial stack pointer address
            :type stack: hexadecimal string

            :returns: (dict) Status [See __handle_return]
        """

        print(self.__c_generator.get_filename())
        print(self.__h_generator.get_filename())

        contents = self.__generate_header(json_input, name, item_callback)
        self.__h_generator.write(contents)
        
        contents = self.__generate_source_file(json_input, name, item_callback)
        self.__c_generator.write(contents)

        print("")
        
        #for x in self.__generate_template_source_file():
        #    print(x)

        #self.__item_generator(json_input, name, item_callback)
        #return self.__handle_return(out, DEBUG_LOG)
