/*
* Copyright 2020 NXP.
* This software is owned or controlled by NXP and may only be used strictly in accordance with the
* license terms that accompany it. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you
* agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
* applicable license terms, then you may not retain, install, activate or otherwise use the software.
*/

/*******************************************************************************
* Includes
******************************************************************************/

/*******************************************************************************
* Definitions
******************************************************************************/

const char avs_smart_home_json_string[] = "{\"event\": {\"header\": {\"namespace\": \"Alexa.Discovery\", \"name\": \"AddOrUpdateReport\", \"payloadVersion\": \"3\"" \
										", \"messageId\": \"notused\", \"eventCorrelationToken\": \"notused\"}, \"payload\": {\"endpoints\": [{\"endpointId\": \"" \
										"notused\", \"registration\": {\"productId\": \"notused\", \"deviceSerialNumber\": 0}, \"manufacturerName\": \"NXP Semico" \
										"nductors\", \"description\": \"NXP Smart Home Development Kit\", \"friendlyName\": \"\", \"displayCategories\": [\"LIGHT" \
										"\", \"ALEXA_VOICE_ENABLED\"], \"capabilities\": [{\"type\": \"AlexaInterface\", \"interface\": \"Alexa.PowerController\"" \
										", \"version\": \"3\", \"properties\": {\"supported\": [{\"name\": \"powerState\"}], \"proactivelyReported\": true, \"ret" \
										"rievable\": true}}, {\"type\": \"AlexaInterface\", \"interface\": \"Alexa.BrightnessController\", \"version\": \"3\", \"" \
										"properties\": {\"supported\": [{\"name\": \"brightness\"}], \"proactivelyReported\": true, \"retrievable\": true}}, {\"t" \
										"ype\": \"AlexaInterface\", \"interface\": \"Alexa\", \"version\": \"3\"}]}]}}}";

/*******************************************************************************
* Public Functions
******************************************************************************/

