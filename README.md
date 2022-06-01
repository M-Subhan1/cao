# Introdcution

An ESP-32 bot written using ESP-IDF framework. I have found myself glued to discord due to my interests (programming, gaming) which require a lot of collobration. If you can relate, this project will allow you to automate your appliances without ever diverting attention from discord. 

## How to set up
  > Note 1: You need to have installed [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) extension in your VS Code.
  > Note 2: Make sure that you have `ESP_IDF` environment variable (which leads to esp-idf folder) and path to XTENSA compiler _bin_ folder needs to be present on the `PATH`.
  
1. Clone repository
```
git clone https://github.com/M-Subhan1/cao.git
```

2. Go inside of project folder
```
cd cao
```

3. Start VSC
```
code .
```

4. Config, Build and Flash

```
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py -p (PORT) flash
```

5. Add Bot to server

## List of Commands

- bind <DEVICE_NAME> - Binds the device to a free pin
- !unbind <DEVICE_NAME> - Frees the pin bound to the device
- !on <DEVICE_NAME> - Turn on the device
- !off <DEVICE_NAME> - Turns off the device
- !disable <DEVICE_NAME> - Disables a device, stopping all features from working
- !enable <DEVICE_NAME> - Enables a device, allowing all features to work
- !on_delayed <DEVICE_NAME> <DELAY_IN_SEC> - Turns on the device after Time delay(in seconds)
- !off_delayed <DEVICE_NAME> <DELAY_IN_SEC> - Turns off the device after Time delay(in seconds)
- !on_delayed <DEVICE_NAME> <DELAY_IN_SEC> - Turns off the device after Time delay(in seconds)
- !clear_timers <DEVICE_NAME> - Clears all timers for the specified device
- !timers <DEVICE_NAME> - List all active timers
- !devices - Prints details about all registered devices
- !help - Returns the list of available commands

## Author

GitHub: [M-Subhan1](https://github.com/M-Subhan1)<br>
