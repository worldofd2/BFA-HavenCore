#!/bin/bash
# Shared entrypoint for worldserver/bnetserver inside the runtime container.
# Entrypoint compartido para worldserver/bnetserver dentro del contenedor runtime.
# Usage / Uso: entrypoint.sh worldserver|bnetserver [args...]
set -euo pipefail

SERVICE="${1:?Usage/Uso: entrypoint.sh worldserver|bnetserver}"
shift || true

ETC="/opt/bfacore/etc"
CONF="${ETC}/${SERVICE}.conf"
DIST="${ETC}/${SERVICE}.conf.dist"

DB_HOST="${DB_HOST:-mysql}"
DB_PORT="${DB_PORT:-3306}"
DB_USER="${DB_USER:-root}"
DB_PASS="${DB_PASS:-admin}"

if [ ! -f "$CONF" ]; then
  echo "[entrypoint] Generating ${CONF} from ${DIST} / Generando ${CONF} a partir de ${DIST}"
  cp "$DIST" "$CONF"
  sed -i \
    -e "s#127.0.0.1;3306;root;admin;bfa_auth#${DB_HOST};${DB_PORT};${DB_USER};${DB_PASS};bfa_auth#g" \
    -e "s#127.0.0.1;3306;root;admin;bfa_world#${DB_HOST};${DB_PORT};${DB_USER};${DB_PASS};bfa_world#g" \
    -e "s#127.0.0.1;3306;root;admin;bfa_characters#${DB_HOST};${DB_PORT};${DB_USER};${DB_PASS};bfa_characters#g" \
    -e "s#127.0.0.1;3306;root;admin;bfa_hotfixes#${DB_HOST};${DB_PORT};${DB_USER};${DB_PASS};bfa_hotfixes#g" \
    "$CONF"

  # EN: DBUpdater's auto-updater (DBUpdater.cpp) needs the `mysql` binary
  # (not installed in the runtime image) and the sql/ source tree as it
  # existed at /src during the build (doesn't exist at runtime). Since the
  # schema is already imported by hand with `docker compose run --rm
  # db-import`, we disable it so worldserver doesn't exit 1 without logging
  # anything useful about it.
  # ES: El auto-updater de DBUpdater (DBUpdater.cpp) necesita el binario
  # `mysql` (no instalado en la imagen runtime) y el arbol fuente sql/ tal
  # como estaba en /src durante la build (no existe en runtime). Como el
  # esquema ya se importa a mano con `docker compose run --rm db-import`, lo
  # desactivamos para evitar que worldserver salga con exit 1 sin loguear
  # nada util al respecto.
  sed -i -e "s#^Updates\.EnableDatabases.*#Updates.EnableDatabases = 0#" "$CONF"

  # EN: The .conf.dist files ship with Windows-style path separators (e.g.
  # ".\ClientData", ".\Data\Logs"), which on Linux are just a literal
  # filename character instead of a path separator. Convert backslashes to
  # "/" only on the known path lines.
  # ES: Los .conf.dist traen paths con separador de Windows (ej.
  # ".\ClientData", ".\Data\Logs"), que en Linux son un nombre de archivo
  # literal en vez de una ruta relativa. Convertimos los backslashes a "/"
  # solo en las lineas de paths conocidas.
  sed -i -E '/^(DataDir|LogsDir)[[:space:]]*=/ s#\\#/#g' "$CONF"
fi

echo "[entrypoint] Waiting for MySQL at / Esperando a MySQL en ${DB_HOST}:${DB_PORT}..."
for i in $(seq 1 60); do
  if (exec 3<>"/dev/tcp/${DB_HOST}/${DB_PORT}") 2>/dev/null; then
    exec 3<&- 3>&-
    break
  fi
  sleep 2
done

mkdir -p /opt/bfacore/bin/Logs /opt/bfacore/bin/Data/Logs

cd /opt/bfacore/bin
exec "./${SERVICE}" "$@"
