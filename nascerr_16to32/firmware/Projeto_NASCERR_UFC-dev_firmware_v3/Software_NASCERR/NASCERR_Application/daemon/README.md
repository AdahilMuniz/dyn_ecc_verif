# software_nascerr.service

Daemon systemd para executar a aplicação NASCERR dentro de um ambiente virtual Python.

## 📌 Visão Geral
O `software_nascerr.service` é um serviço systemd que:
- Inicia automaticamente a aplicação `main.py`.
- Ativa um ambiente virtual Python (`.venv`).
- Reinicia automaticamente em caso de falha.
- Inicia após a rede estar disponível.

## 📂 Estrutura do Projeto
```
/home/nascerr4/Desktop/Projeto_NASCERR_UFC/Software_NASCERR/NASCERR_Application/
│-- venv/                # Ambiente virtual Python
│-- main.py              # Script principal da aplicação
```

## 🔧 Instalação e Configuração

### 1️⃣ Criar e ativar a venv (se ainda não existir)
```bash
cd /home/nascerr4/Desktop/Projeto_NASCERR_UFC/Software_NASCERR/NASCERR_Application
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
sudo pigpiod
sudo systemctl enable pigpiod
sudo systemctl start pigpiod
```

### 2️⃣ Criar o serviço systemd
Crie o arquivo do serviço:
```bash
sudo nano /etc/systemd/system/software_nascerr.service
```

Adicione o seguinte conteúdo:
```ini
[Unit]
Description=Daemon Python para rodar o software NASCERR dentro da venv
After=network.target

[Service]
Type=simple
WorkingDirectory=/home/nascerr4/Desktop/Projeto_NASCERR_UFC/Software_NASCERR/NASCERR_Application
ExecStart=/bin/bash -c 'source /home/nascerr4/Desktop/Projeto_NASCERR_UFC/Software_NASCERR/NASCERR_Application/venv/bin/activate && exec python main.py'
Restart=always

[Install]
WantedBy=multi-user.target
```

### 3️⃣ Habilitar e iniciar o serviço
Após criar o arquivo de serviço, execute os seguintes comandos:

```bash
sudo systemctl daemon-reload
sudo systemctl enable software_nascerr
sudo systemctl start software_nascerr
```

## 🚀 Comandos para Gerenciamento
Verificar o status do serviço:
```bash
sudo systemctl status software_nascerr
```

Parar o serviço:
```bash
sudo systemctl stop software_nascerr
```

Reiniciar o serviço:
```bash
sudo systemctl restart software_nascerr
```

Exibir logs em tempo real:
```bash
journalctl -u software_nascerr -f
```

---

# update_repositorio_nascerr.service

### 🔄 Serviço de Atualização Automática do Repositório

Este serviço garante que o código da branch `dev_daemon` do repositório seja atualizado automaticamente toda vez que o Raspberry Pi for iniciado.

#### 📁 Local do Script

O script utilizado para a atualização se encontra em:

```
Software_NASCERR/NASCERR_Application/daemon/update_repositorio.sh
```

#### 🔧 Conteúdo do Script

```bash
#!/bin/bash

log_file="/home/nascerr4/update_log.txt"

echo ">>> $(date) - INICIANDO UPDATE REPO" >> "$log_file"

for i in {1..10}; do
    echo "Tentativa $i de git pull..." >> "$log_file"
    git -C /home/nascerr4/Desktop/Projeto_NASCERR_UFC pull origin dev_daemon >> "$log_file" 2>&1 && break
    echo "Rede ainda não disponível ou erro. Tentando novamente em 10 segundos..." >> "$log_file"
    sleep 5
done

echo ">>> $(date) - FIM DO UPDATE" >> "$log_file"

```

> ⚠️ Certifique-se de que o script tenha permissão de execução:
>
> ```bash
> chmod +x Software_NASCERR/NASCERR_Application/daemon/update_repositorio.sh
> ```


#### ⚙️ Unidade Systemd do Serviço

O serviço está localizado em `/etc/systemd/system/update_repositorio_nascerr.service` e tem o seguinte conteúdo:

```ini
[Unit]
Description=Atualiza automaticamente o repositório NASCERR no boot
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=nascerr4
WorkingDirectory=/home/nascerr4/Desktop/Projeto_NASCERR_UFC
ExecStart=/home/nascerr4/Desktop/Projeto_NASCERR_UFC/Software_NASCERR/NASCERR_Application/daemon/update_repositorio.sh

[Install]
WantedBy=multi-user.target
```

Você pode acessa-lo usando:
```bash
sudo nano /etc/systemd/system/update_repositorio_nascerr.service
```

#### 🚀 Ativando o Serviço

Após configurar, rode os seguintes comandos para ativar o serviço no boot:

```bash
sudo systemctl daemon-reload
sudo systemctl enable update_repositorio_nascerr.service
sudo systemctl start update_repositorio_nascerr.service
```


#### 📄 Log de Atualizações

As saídas do `git pull` e mensagens de status são gravadas no arquivo:

```
Software_NASCERR/NASCERR_Application/daemon/update_log.txt
```

Você pode verificar o log com:

```bash
cat Software_NASCERR/NASCERR_Application/daemon/update_log.txt
```

---
