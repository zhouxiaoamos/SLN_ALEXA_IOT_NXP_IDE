# Smart Home for AVS

Smart Home for AVS is a module that allows Discovery Descriptors to be converted into C format so that
the application integrating this can configure itself to support the different capabilities.

This also allows the developer to focus on defining the behavior and then writing the actionable code to handle
Brightness, Power, Toggle, Mode and Range controllers

## Defining the Discovery Descriptor

Within the avs module, there is an example json file that NXP uses to enable Power and Brightness Controller
Follow the guidelines in at https://developer.amazon.com/en-US/docs/alexa/device-apis/alexa-discovery.html

The key things to change are the endpoint information and the manufacture description.

The following attributes in the JSON are ignored and auto generated inside at runtime in the firmware:
- "endpointId": "notused"
- "productId": "notused"
- "deviceSerialNumber": 0


### Steps

To Enable Smart Home
- Updated the json file inside the avs folder with the correct discovery endpoint information
- Run "python smart_home_avs_code_generator.py smart_home_avs_discovery_interface.json"
- Update the AIA_CLIENT_ID define with the client ID in the product definition in the Amazon Developer Portal
- Update the clientcredentialIOT_PRODUCT_NAME with the product ID defined in the Amazon Developer Portal
- Go to source/app_smart_home_task.c
- Modify the code to execute the specific behavior when a smart home message occurs
- Recompile the project
- Program the firmware and onboard the device to an Alexa account
- See the new device in the Alexa account (for NXP it's "NXP Light Switch")
- Use the Alexa Application or your voice to turn on/off or set the brightness value
- If the code has not changed, a log of the command will be printed, if the code has changed, it should execute the new functionality