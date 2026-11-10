#!/bin/bash

log_file="$HOME/update_log.txt"

echo ">>> $(date) - INICIANDO UPDATE REPO" >> "$log_file"

for i in {1..10}; do
    echo "Tentativa $i de git pull..." >> "$log_file"
    git -C /home/nascerr4/Desktop/Projeto_NASCERR_UFC pull origin master >> "$log_file" 2>&1 && break
    echo "Rede ainda não disponível ou erro. Tentando novamente em 10 segundos..." >> "$log_file"
    sleep 5
done

echo ">>> $(date) - FIM DO UPDATE" >> "$log_file"
