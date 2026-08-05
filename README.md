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

Clicar em "MAIS RUA BY FROZENSHADE", no rodapé do plugin, abre uma tela
"sobre" com a versão instalada e um botão pra checar se tem atualização
nova. O troféu no canto abre a lista de conquistas escondidas — cada uma
mostra o requisito pra desbloquear.
