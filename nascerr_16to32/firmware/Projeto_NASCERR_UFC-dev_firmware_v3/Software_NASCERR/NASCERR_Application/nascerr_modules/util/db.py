import os
import json
from typing import List
from datetime import datetime

caminho_comandos = "logs/commands.json"
caminho_resultados = "logs/results.json"

class LoggerJSON:
    def __init__(self):
        self.caminho_comandos = caminho_comandos
        self.caminho_resultados = caminho_resultados

        # Garante que os diretórios existem
        os.makedirs(os.path.dirname(caminho_comandos), exist_ok=True)
        os.makedirs(os.path.dirname(caminho_resultados), exist_ok=True)

    def save_command_json(self, comandos):
        """
        Salva o último comando da lista no JSON.
        """
        try:
            with open(self.caminho_comandos, 'r', encoding='utf-8') as f:
                dados_existentes = json.load(f)
        except (FileNotFoundError, json.JSONDecodeError):
            dados_existentes = []

        comando_dict = comandos[-1].to_dict()
        dados_existentes.append(comando_dict)

        with open(self.caminho_comandos, 'w', encoding='utf-8') as f:
            json.dump(dados_existentes, f, indent=4, ensure_ascii=False)

    def save_results_json(self, resultados):
        """
        Salva o último resultado da lista no JSON.
        """
        try:
            with open(self.caminho_resultados, 'r', encoding='utf-8') as f:
                dados_existentes = json.load(f)
        except (FileNotFoundError, json.JSONDecodeError):
            dados_existentes = []

        resultado_dict = resultados[-1].to_dict()
        dados_existentes.append(resultado_dict)

        with open(self.caminho_resultados, 'w', encoding='utf-8') as f:
            json.dump(dados_existentes, f, indent=4, ensure_ascii=False)
