#!/bin/bash
# EN: Imports every *.sql dump found in ./sql/base into its matching
# database (filename without extension = database name).
# ES: Importa cada dump *.sql que haya en ./sql/base a su base de datos
# correspondiente (nombre de archivo sin extension = nombre de la base).
set -e
shopt -s nullglob
files=(/sql/base/*.sql)
if [ ${#files[@]} -eq 0 ]; then
  echo "No .sql files found in ./sql/base -- download them first (see docker/README.md)."
  echo "No hay archivos .sql en ./sql/base -- descargalos primero (ver docker/README.md)."
  exit 1
fi
for f in "${files[@]}"; do
  echo "Importing / Importando $f ($(du -h "$f" | cut -f1)) ..."
  mysql -h mysql -uroot -p"${DB_ROOT_PASSWORD:-admin}" --max_allowed_packet=1G "$(basename "$f" .sql)" < "$f"
done
echo "Import complete / Import completo."
