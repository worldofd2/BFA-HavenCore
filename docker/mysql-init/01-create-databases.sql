-- EN: Runs once, only on first container start with an empty data volume
-- (mysql image convention: everything in /docker-entrypoint-initdb.d).
-- Creates the 4 empty databases the core expects; schema/data come later
-- via `docker compose run --rm db-import`.
-- ES: Corre una sola vez, solo en el primer arranque con el volumen de
-- datos vacio (convencion de la imagen de mysql: todo lo que esta en
-- /docker-entrypoint-initdb.d). Crea las 4 bases vacias que espera el
-- core; el esquema/datos se cargan despues con
-- `docker compose run --rm db-import`.
CREATE DATABASE IF NOT EXISTS bfa_auth CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE IF NOT EXISTS bfa_world CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE IF NOT EXISTS bfa_characters CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE IF NOT EXISTS bfa_hotfixes CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
