set shell := ["bash", "-cu"]

build_dir := "build"
exe := build_dir / "PrankApp"

# Lista as receitas disponíveis
default:
    @just --list

# Configura + compila (recria o binário se tiver sido apagado por --uninstall)
build:
    cmake -B {{build_dir}} -DCMAKE_BUILD_TYPE=Release
    cmake --build {{build_dir}} -j$(nproc)

# Apaga a pasta de build por completo (configuração + binário)
clean:
    rm -rf {{build_dir}}

# Build limpo do zero
rebuild: clean build

# Corre em primeiro plano, tocando um som a cada minuto (100%), para testar
# rapidamente som + subida/restauro de volume. Ctrl+C para parar.
test: build
    {{exe}} --foreground --test-mode

# Corre em modo normal: destaca-se para segundo plano e regista o autostart.
run: build
    {{exe}}

# Mostra se o PrankApp está a correr
status:
    @pgrep -fa '[P]rankApp' || echo "(não está a correr)"

# Kill switch: para o processo, remove o autostart e apaga o próprio binário.
# Corre `just build` depois se quiseres o executável de volta.
uninstall:
    {{exe}} --uninstall

# Mostra o ficheiro de autostart, se existir
show-autostart:
    @cat ~/.config/autostart/prankapp.desktop 2>/dev/null \
        || cat ~/Library/LaunchAgents/com.prankapp.agent.plist 2>/dev/null \
        || echo "(nenhum autostart registado)"

# Substitui os .wav de assets/sounds/ pelos ficheiros dados (caminhos separados
# por espaço) e recompila para os embutir. Ex: just add-sounds ~/Downloads/a.wav ~/Downloads/b.wav
add-sounds *files:
    rm -f assets/sounds/*.wav
    cp {{files}} assets/sounds/
    just build

# Lista os .wav atualmente embutidos
list-sounds:
    @ls -la assets/sounds/*.wav 2>/dev/null || echo "(nenhum .wav em assets/sounds/)"
