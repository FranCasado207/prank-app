# AGENTS.md

## O que é isto

PrankApp é um executável C++20, cross-platform (Linux/macOS/Windows), que corre em
segundo plano sem janela/consola visível. A cada 30 minutos tem 4% de hipótese de
subir o volume do sistema para 50% e tocar um som aleatório embutido no binário,
restaurando o volume anterior logo a seguir. Regista-se para arrancar com o sistema
(autostart por-utilizador, sem admin/root) e tem um kill switch via CLI
(`--uninstall`) que para o processo, remove o autostart e apaga o próprio binário.

É software de "prank" legítimo, pensado para correr em máquinas do próprio
utilizador ou com consentimento. Isso molda decisões de design importantes — ver
"Regras de design a não quebrar" abaixo antes de alterar o comportamento de
instalação/remoção.

## Stack e dependências

- C++20, CMake ≥ 3.20.
- [miniaudio](https://miniaud.io/) vendored em `third_party/miniaudio.h` (single
  header, MIT/public-domain). É a única dependência externa. Em Linux/macOS faz
  `dlopen` de ALSA/PulseAudio/CoreAudio em runtime — não precisa de
  `libasound-dev`/`libpulse-dev` para compilar, só `pthread`, `dl`, `m` no link
  (não usar toolchains musl estáticas: o `dlopen` do musl é um stub).
- Sem outras dependências de terceiros.

## Estrutura do projeto

```
CMakeLists.txt              # build: fontes condicionais por OS, embedding dos sons
cmake/bin2h.cmake           # gera generated/sound_data.h a partir de assets/sounds/*.wav
justfile                    # ver `just --list` — build/test/run/uninstall/gerir sons
.github/workflows/build.yml # CI: build+smoke-test+artefacto em ubuntu/macos/windows
assets/sounds/*.wav         # sons a embutir (qualquer .wav aqui é apanhado automaticamente)
third_party/miniaudio.h     # vendored, não editar
src/
  main.cpp                  # parsing de CLI (--foreground/--test-mode/--uninstall), orquestração
  scheduler.{hpp,cpp}       # loop de 30min/4% (ou 1min/100% em --test-mode), dispara volume+som
  audio.{hpp,cpp}           # toca um EmbeddedSound aleatório via miniaudio (in-memory, sem ficheiros)
  volume/                   # get/set_volume_percent — 1 .cpp por OS, mesma interface (volume.hpp)
  autostart/                # install()/uninstall() do autostart — 1 .cpp por OS (autostart.hpp)
  instance/                 # single-instance lock + stop-signal file — posix.cpp / windows.cpp
  platform/                 # self_delete() — posix.cpp (unlink direto) / windows.cpp (helper CreateProcessW)
```

Padrão geral: cada componente que difere por SO tem uma interface comum num
`.hpp` e uma implementação por plataforma (`_linux.cpp` / `_macos.cpp` /
`_windows.cpp`, ou `_posix.cpp` quando Linux e macOS partilham a mesma
implementação). O `CMakeLists.txt` escolhe os `.cpp` certos via
`if(WIN32)/elseif(APPLE)/else()`.

## Build e testes

Usa o `justfile` (`just --list` mostra tudo):

```bash
just build       # cmake configure + build
just test        # --foreground --test-mode: toca um som a cada minuto (100%), Ctrl+C para parar
just run         # modo normal: destaca-se para background + instala autostart
just status      # mostra se está a correr
just uninstall   # kill switch: para, remove autostart, apaga o binário (correr `just build` depois)
just show-autostart
just list-sounds
just add-sounds <wav...>   # troca os .wav embutidos e recompila
just rebuild      # clean + build
```

Sem `just`: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`.

Só foi compilado e testado manualmente em Linux até agora. O código Windows/macOS
foi escrito segundo as APIs corretas mas nunca compilado nem executado numa
máquina real dessas plataformas — tratar com mais cautela e validar via CI antes
de assumir que funciona.

## Regras de design a não quebrar sem avisar o utilizador

- **Kill switch fácil e transparente é intencional.** Nada de esconder o
  processo, anti-remoção, evasão de antivírus, ou qualquer truque que dificulte
  o `--uninstall`. Isto é o que distingue este projeto de malware.
- **Autostart é sempre por-utilizador**, nunca requer admin/root
  (Startup folder do utilizador no Windows, LaunchAgent no macOS,
  `~/.config/autostart` no Linux — nunca instalação system-wide).
- **`KeepAlive`/equivalente é sempre `false`** — se o utilizador matar o
  processo manualmente, não deve voltar a arrancar sozinho antes do próximo
  login. Consistente nas 3 plataformas.
- **O volume tem de voltar ao valor anterior** depois de cada som — sempre
  `get_volume_percent()` antes de subir para 50%, e restaurar esse valor depois
  de `audio::play_random_embedded_sound()` terminar (ver `scheduler.cpp`).
- **Os sons são sempre embutidos no binário em build-time**, nunca lidos de
  disco em runtime — `cmake/bin2h.cmake` converte cada `.wav` de
  `assets/sounds/` num array de bytes; ver `g_embedded_sounds[]` em
  `generated/sound_data.h` (gerado, não commitado). A escolha de qual tocar é
  aleatória (`std::uniform_int_distribution`) entre todos os embutidos.
- Manter cada `.wav` pequeno (idealmente < ~300KB) — a conversão hex→array via
  regex do CMake é lenta em ficheiros grandes.

## Pitfalls conhecidos (ver comentários no código onde relevante)

- Windows + subsistema `WIN32`: MSVC precisa de
  `target_link_options(PRIVATE /ENTRY:mainCRTStartup)` para poder usar `main()`
  normal em vez de `WinMain()` (já tratado no `CMakeLists.txt`; MinGW não
  precisa disto).
- Self-delete no Windows nunca usa `system()`/`timeout` — `timeout` falha sem
  stdin/consola (que este processo não tem) e `system()` faria flash de uma
  janela de consola. Usa `CreateProcessW` com
  `CREATE_NO_WINDOW|DETACHED_PROCESS` a correr `ping -n 6 127.0.0.1 & del`.
- Ordem de teardown do miniaudio importa: `ma_sound_uninit` antes de
  `ma_decoder_uninit`, antes de `ma_engine_uninit` (o sound lê do decoder).
- `MINIAUDIO_IMPLEMENTATION` só pode ser definido uma vez — está em
  `audio.cpp`; qualquer outro ficheiro que precise de tipos `ma_*` só inclui
  `miniaudio.h` sem o `#define`.
- `file(GLOB ... CONFIGURE_DEPENDS)` em `assets/sounds/*.wav` faz o CMake
  reconfigurar sozinho quando se adiciona/remove um `.wav`.

## CI

`.github/workflows/build.yml` builda em `ubuntu-latest`, `macos-latest` e
`windows-latest` (matrix), corre um smoke-test (`--foreground --test-mode`
durante alguns segundos, confirma que não crasha mesmo sem hardware de
áudio/sound server no runner) e publica o binário de cada plataforma como
artefacto do workflow run. Corre em push/PR/dispatch — precisa do repo estar
num remoto GitHub para disparar.
