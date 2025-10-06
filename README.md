# Dodge the Weather (raylib + Emscripten)

A micro-game built with raylib and compiled to WebAssembly using Emscripten.
You control a player who must dodge falling shapes — the type of shapes and background change depending on real-world weather in London, fetched live from the Open-Meteo API.
- **Sunny** → yellow circles (default if API fails)
- **Cloudy** → white blobs
- **Rainy** → thin blue raindrops

## Controls
- Move: WASD / Arrow Keys
- Start: SPACE (from Menu)
- Restart: R (Game Over)
- Menu: ESC (Game Over)

## Files

 Dodge/
 
 ├─ main.cpp                 # Full Raylib game code
 
 ├─ README.md   
 
 ├─ /web
 
 │   ├─ index.html           # Compiled HTML output
 
 │   ├─ index.js             # Generated Emscripten JavaScript
 
 │   ├─ index.wasm           # Generated WebAssembly binary
 
 │   └─ shell.html           # Template for compilation (includes weather API)


## How to recompile with Emscripten

If you modify main.cpp, you’ll need to rebuild using Emscripten.

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
- [Raylib source code](https://www.raylib.com/), cloned or downloaded locally

The example paths below assume you installed Raylib to `C:\raylib\raylib\src`.  

### Compile Command, run in project folder:

em++ Main.cpp -std=c++17 -O2 ^

  -I C:\raylib\raylib\src ^
  
  -DPLATFORM_WEB C:\raylib\raylib\build_web\libraylib_web.a ^
  
  -s USE_GLFW=3 -s ASYNCIFY ^
  
  -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 ^
  
  -s EXPORTED_RUNTIME_METHODS=ccall ^
  
  --shell-file web\shell.html ^
  
  -o web\index.html



This will regenerate:

web/index.html

web/index.js

web/index.wasm


## Important – Using the Provided shell.html
Note  * shell-file argument ensures the version of shell.html (with the weather fetch code) is used as the template

web/shell.html contains:

The weather API fetch logic

The JavaScript bridge calling SetWeather()

Styling to center the canvas in the browser



## Serve and play in a Browser

How to Play Locally 

### Option 1 — Download ZIP

Open a terminal inside the project folder

Run:

cd web
python -m http.server 8000


Open http://localhost:8000/index.html


### Option 2 — Clone via Git

Open a terminal and run:

git clone https://github.com/<your-repo-name>.git
cd <your-repo-name>/web

python -m http.server 8000

Then open http://localhost:8000/index.html


## API Used
The game fetches the current weather for London from Open-Meteo:
https://api.open-meteo.com/v1/forecast?latitude=51.5072&longitude=-0.1276&current_weather=true

The JavaScript inside web/shell.html fetches this data and sends the weather code (WMO) to the C++ function:
The response’s weathercode is mapped to:

0 → Sunny (clear sky) return 0 

1,2,3,45,48 → Cloudy (Cloudy, fog, mist..) return 1

Other codes → Rainy (Rain, Snow, thunder..)  return 2 default (everything else that is not sunny or cloudy)

When the game starts, a small JavaScript function in **`shell.html`** fetches London’s current weather code and sends it into the C++ code using:
```cpp
Module.ccall("SetWeather", null, ["number"], [kind]
