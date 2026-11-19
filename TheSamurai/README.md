# THE SAMURAI — Kick Designer (VST3 / AU / Standalone)

Diseñador de kicks por capas: cargas de 2 a 4 kicks, los cortas en **Punch / Body / Tail**, mezclas fuentes por parte, esculpes y exportas.

**Formatos de entrada:** WAV, **FLAC**, **AIFF**, OGG y MP3 (arrastrar y soltar o botón LOAD).
**Formatos de exportación:** WAV, FLAC o AIFF a 24 bits (se elige con la extensión al guardar).

---

## Qué hace

| Sección | Controles |
|---|---|
| **Fuentes A–D** | Arrastra un archivo o pulsa LOAD. Gain, Offset (±10 ms para alinear la fase) y PHASE (invertir polaridad). Al cargarlos, los kicks se recortan (silencio inicial) y se normalizan, así que las mezclas salen igualadas de nivel. |
| **Forma de onda** | Muestra el kick final. Arrastra las dos líneas blancas para mover los límites Punch→Body y Body→Tail. |
| **Punch / Body / Tail** | Fuente A + fuente B con BLEND, LEVEL, LOW CUT y HIGH CUT (cortes por parte), SOLO y LOCK. |
| **Sculpt** | Impact (transitorio), Drive (saturación), Tone (inclinación de EQ), Filter + Reso. Los 4 desplegables marcan el **orden** de la cadena. |
| **EQ** | Low shelf a 60 Hz, dos medios paramétricos y high shelf a 8 kHz. |
| **Output** | Length, Fade, X-Fade (cruce entre partes), Tune, Mono Bass (graves en mono por debajo de X Hz, con fase cero), Output, Velocity y Key Track. |
| **Barra superior** | PLAY (escuchar), EXPLORE (genera una variación con las partes no bloqueadas), BACK (deshacer la última exploración), EXPORT, SAVE/OPEN (diseños `.samurai` que incluyen el audio de las fuentes), DRAG TO DAW (arrastra el kick directamente a una pista). |

Se dispara con cualquier nota MIDI. Con KEY TRACK, la nota C3 (60) reproduce el tono original. El proyecto de tu DAW guarda el audio de las fuentes dentro del propio proyecto, así que aunque muevas los archivos originales no se pierde nada.

---

## Opción 1: compilar sin instalar nada (GitHub)

1. Crea un repositorio en GitHub y sube esta carpeta tal cual (incluida la carpeta oculta `.github`).
2. Abre la pestaña **Actions**. La compilación arranca sola (unos 10–15 min).
3. Al terminar, en Windows descarga **THE-SAMURAI-Setup-Windows**: dentro está `THE SAMURAI Setup 1.0.0.exe`. Ejecútalo e instalará el VST3 en `C:\Program Files\Common Files\VST3\` y la aplicación independiente en el menú Inicio (puedes elegir una o las dos). Se desinstala desde "Aplicaciones" de Windows.
   Como el instalador no está firmado, Windows SmartScreen puede avisar: pulsa "Más información" → "Ejecutar de todas formas".
4. Si prefieres instalar a mano (o estás en Mac), descarga **THE-SAMURAI-Windows** / **THE-SAMURAI-macOS** y copia `THE SAMURAI.vst3` en:
   - Windows: `C:\Program Files\Common Files\VST3\`
   - macOS: `/Library/Audio/Plug-Ins/VST3/` (y el `.component` en `/Library/Audio/Plug-Ins/Components/`)
5. Vuelve a escanear plugins en tu DAW.

En macOS, como el plugin no está firmado, la primera vez ejecuta en Terminal:
```
sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/THE SAMURAI.vst3"
```

## Opción 2: compilar en tu ordenador

**Windows**: instala Visual Studio 2022 Community (con la carga de trabajo "Desarrollo para el escritorio con C++"), Git y CMake 3.22 o superior.
**macOS**: instala Xcode, y CMake con `brew install cmake`.

```
cd TheSamurai
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

El plugin queda en `build/TheSamurai_artefacts/Release/VST3/THE SAMURAI.vst3`. CMake descarga JUCE 8 automáticamente la primera vez.

---

## Estructura

```
CMakeLists.txt            proyecto (JUCE 8 via FetchContent)
Source/KickEngine.*       motor de render offline (capas, crossfades, sculpt, EQ, mono bass)
Source/PluginProcessor.*  parámetros, carga de audio, MIDI, estado, export, explore
Source/PluginEditor.*     interfaz verde / negro / blanco
.github/workflows/        compilación automática Windows + macOS
```

Nota sobre la licencia: JUCE 8 es gratuito para uso personal y para ingresos bajos (consulta juce.com). Si vas a vender el plugin, revisa sus condiciones.
