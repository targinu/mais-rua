# Mais Rua (Street More)

Plugin de efeito "meme" pra DAW, feito com JUCE. Um knob só, chamado **rua**:
quanto mais você abre, mais "rua" o som fica. Em 0% o sinal passa limpo.

## Download

Baixe o build mais recente (VST3 + Standalone pra Windows) na aba
[**Releases**](../../releases).

Pra instalar o VST3: copie `Mais Rua.vst3` pra pasta de plugins VST3 da sua
DAW (normalmente `C:\Program Files\Common Files\VST3`). O Standalone (`Mais
Rua.exe`) roda sozinho, sem DAW, pra testar rápido.

## O que ele faz

A cadeia progride junto com o knob **rua**:

- drive + saturação não-linear, com **duas curvas** selecionáveis no
  parâmetro **modo**: `Classico` (tanh puro) ou `Grao` (waveshaper
  assimétrico, mais harmônicos/textura)
- bitcrush + downsample (sample & hold) entrando a partir de ~35%
- a etapa de saturação + bitcrush roda em **2x oversampling** internamente,
  pra reduzir aliasing nos agudos (latência reportada corretamente ao host)
- reforço de graves (low shelf) e corte de agudos (low-pass 18k → 4k)
- compensação de volume e blend wet/dry (em 0% o sinal passa limpo, sample
  a sample, sem zipper noise)

A UI é desenhada à mão: knob com arco reativo, glow que pulsa com o áudio,
aberração cromática no título, film grain, scanlines, glitch e shake que
aumentam junto com o valor. A cor vai de teal (chill) a laranja/vermelho (rua).

## Build

Requisitos: CMake 3.22+, compilador C++17 (MSVC / clang / gcc) e git
(o CMake baixa o JUCE via FetchContent na primeira compilação).

Linux precisa dos headers de dev:

    sudo apt install libasound2-dev libx11-dev libxext-dev libxinerama-dev \
                     libxrandr-dev libxcursor-dev libfreetype6-dev libcurl4-openssl-dev

Compilar:

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

Saídas (com COPY_PLUGIN_AFTER_BUILD já vai pra pasta padrão de plugins do SO):

    build/MaisRua_artefacts/Release/VST3/Mais Rua.vst3
    build/MaisRua_artefacts/Release/Standalone/   (pra testar rápido sem DAW)

## Ajustes rápidos

- Versão do JUCE: `GIT_TAG` no `CMakeLists.txt`
- Intensidade do drive: constante `14.0f` no `processBlock` (`PluginProcessor.cpp`)
- Ponto onde o bitcrush entra: `(r - 0.35f) / 0.65f`
- Curva do modo "Grao": função `shapeGrit` no topo de `PluginProcessor.cpp`
- Fator de oversampling: segundo argumento do `juce::dsp::Oversampling` no
  construtor do processor (`1` = 2x)
- Fonte do título: `juce::Font (juce::FontOptions (46.0f, ...))` no `PluginEditor.cpp`
