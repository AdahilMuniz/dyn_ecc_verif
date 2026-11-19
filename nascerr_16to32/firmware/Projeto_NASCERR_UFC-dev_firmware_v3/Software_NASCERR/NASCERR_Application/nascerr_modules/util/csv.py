import csv
import os

class ExperimentResultSaver:
    def __init__(self, file_path: str):
        self.file_path = file_path
        
    def save_result(self, data: dict):
        """
        Salva os resultados do experimento no arquivo CSV.
        :param data: O dicionário contendo os dados de resposta do experimento.
        """
        # Verifica se o arquivo já existe
        file_exists = os.path.isfile(self.file_path)
        
        # Defina os campos do CSV com base nos dados que você está recebendo
        fieldnames = ['timestamp', 'id','write_type','ecc','offset','data_length', 'number_errors_last_iteration', 'correct_data', 'avg_percent_error', 'errors_addr', 'errors_result']
        
        try:
            with open(self.file_path, mode='w', newline='') as csvfile:
                
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames, delimiter=';')
            
                writer.writeheader()
                
                if len(data["result_array"]) == 0:
                    writer.writerow({
                        'timestamp': data["timestamp"],
                        'id': data["id"],
                        'write_type': data["write_type"],
                        'ecc': data["mode"],
                        'offset': data["offset"],
                        'data_length': data["data_length"],
                        'number_errors_last_iteration': data["number_errors"],
                        'correct_data': data["corrected_data"],
                        'avg_percent_error': f"{float(data['percent_error']):.2f}".replace('.', ',') if data['percent_error'] is not None else "Infinito",
                        'errors_addr': 'Nenhum erro encontrado',
                        'errors_result': 'Nenhum erro encontrado'
                    })

                for err in data["result_array"]:
                    writer.writerow({
                    'timestamp': data["timestamp"],
                    'id': data["id"],
                    'write_type': data["write_type"],
                    'ecc': data["mode"],
                    'offset': data["offset"],
                    'data_length': data["data_length"],
                    'number_errors_last_iteration': data["number_errors"],
                    'correct_data': data["corrected_data"],
                    'avg_percent_error': f"{float(data['percent_error']):.2f}".replace('.', ',') if data['percent_error'] is not None else "Infinito",
                    'errors_addr': err['addr'],
                    'errors_result': err['result']
                })
                    # error_row = base_row.copy()
                    # error_row['errors_addr'] = err['addr']
                    # error_row['errors_result'] = err['result']
                    # writer.writerow(error_row)

        except Exception as e:
            print(f"Erro ao salvar dados no CSV: {str(e)}")

    def format_errors(self, error_list):
        """
        Formata a lista de erros no formato de tuplas para ser armazenada no CSV.
        Cada erro será exibido como uma tupla (Addr: ..., Res: ...).
        """
        return ', '.join([f"(Addr: {err['addr']}, Res: {err['result']})" for err in error_list])

