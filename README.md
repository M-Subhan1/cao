# Bootstrapped using esp-idf-vscode-boilerplate
Made with Boilerplate for developing ESP32 projects using ESP-IDF and VS Code

  > Note 1: You need to have installed [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) extension in your VS Code.

  > Note 2: Make sure that you have `ESP_IDF` environment variable (which leads to esp-idf folder) and path to XTENSA compiler _bin_ folder needs to be present on the `PATH`.

## How to set up

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

## Author

GitHub: [M-Subhan1](https://github.com/M-Subhan1)<br>
