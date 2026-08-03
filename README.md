# Mais Rua (Street More)

Plugin de efeito "meme" pra DAW, feito com JUCE. Um knob só, chamado **rua**:
quanto mais você abre, mais "rua" o som fica. Em 0% o sinal passa limpo.

## Download

Baixe o instalador (`MaisRua-X.Y.Z-Setup.exe`) na aba
[**Releases**](../../releases) e rode — ele instala o VST3 na pasta certa
(`C:\Program Files\Common Files\VST3`) e o Standalone com atalho no menu
Iniciar, do jeito que qualquer outro plugin instala. Pede permissão de
administrador porque escreve em Program Files, igual instalador de VST
normal.

Se preferir sem instalador: o `.zip` com `Mais Rua.vst3` (pasta) e `Mais
Rua.exe` soltos também está na release — nesse caso copie o `.vst3` você
mesmo pra pasta de VST3 da sua DAW.

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

A UI é minimalista: só o título e um knob grande, sem textura, sombra ou
gradiente. Atrás do knob tem uma rua vista de lado — conforme "rua" sobe, os
prédios somem (espalhado, não em varredura) até sobrar só o asfalto, com
carros e buracos que vão aparecendo. Nos últimos 20% do knob a cena vira
noite: o poste acende e some uma casinha pichada, a única estrutura que
resiste na rua vazia.

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

### Gerar o instalador (Windows)

Depois de compilar o Release, com o [Inno Setup 6](https://jrsoftware.org/isinfo.php)
instalado:

    ISCC.exe installer\MaisRua.iss

Gera `installer-output\MaisRua-X.Y.Z-Setup.exe`. Pra mudar a versão, edite
`MyAppVersion` no topo do `installer\MaisRua.iss`.
