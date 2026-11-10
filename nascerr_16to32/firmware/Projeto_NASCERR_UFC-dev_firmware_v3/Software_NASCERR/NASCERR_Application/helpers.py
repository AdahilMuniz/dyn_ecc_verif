from typing import Optional
from fastapi import Form
from pydantic import BaseModel, Field
from nascerr_modules.nascerr_command import *
from nascerr_modules.nascerr_response import *
from nascerr_modules.nascerr_image_processing import *
from datetime import datetime
import math
import os
import json

class CommandEntry:
    def __init__(self, command: 'NASCERR_CTRL', send_date: datetime, status: str):
        self.command = command
        self.send_date = send_date
        self.status = status

    def __repr__(self):
        return f"CommandEntry(command={self.command}, send_date={self.send_date})"

    @property
    def formatted_date(self):
        # Formatação da data como string
        return self.send_date.strftime("%Y-%m-%d %H:%M:%S")
    def to_dict(self):
        """
        Convertendo para um dicionário.
        """
        command_data = self.command.to_dict() if hasattr(self.command, 'to_dict') else str(self.command)
        return {
            "command": command_data,
            "send_date": self.formatted_date,
            "status": self.status
        }



class ResponseEntry:
    def __init__(self, response: 'NASCERR_RESPONSE', receive_date: datetime, status: str):
        """
        Estrutura para armazenar as respostas do tipo NASCERR_RESPONSE.
        :param response: A resposta do tipo NASCERR_RESPONSE.
        :param receive_date: Data/hora em que a resposta foi recebida.
        :param status: Status da operação.
        """
        self.response = response
        self.receive_date = receive_date
        self.status = status

    def __repr__(self):
        return f"ResponseEntry(response={self.response}, receive_date={self.receive_date}, status={self.status})"

    @property
    def formatted_date(self):
        """
        Formatação da data como string.
        """
        return self.receive_date.strftime("%Y-%m-%d %H:%M:%S")

    def to_dict(self):
        """
        Convertendo para um dicionário.
        """
        response_data = self.response.to_dict() if hasattr(self.response, 'to_dict') else self.response

        # if isinstance(response_data, dict) and 'percent_error' in response_data:
        #     percent_error = response_data['percent_error']
        #     if isinstance(percent_error, float) and (math.isnan(percent_error) or math.isinf(percent_error)):
        #         response_data['percent_error'] = 0xFFFFFFFF  # ou um valor padrão aceitável
        return {
            "response": response_data, 
            "receive_date": self.formatted_date,
            "status": self.status
        }
    
class StatusResponse(BaseModel):
    # Modelo para a resposta do status
    status: str


class CommandFormModel(BaseModel):
    # Modelo para CommandForm
    cmd_type: CmdTypeDef
    data_length: Optional[int] = Field(None, ge=1, le=1024)
    repeat: Optional[int] = Field(1, ge=1)
    write_type: Optional[W_DataTypeDef] = None
    read: Optional[Seq_ReadTypeDef] = None
    ecc: Optional[NASCERR_MODE] = None
    delay: Optional[int] = Field(0, ge=0)
    offset: Optional[int] = Field(None, ge=0)

    class Config:
        # Enables enum values to be automatically converted to their integer values
        use_enum_values = True

    @classmethod
    def as_form(
        cls,
        cmd_type: int = Form(...),
        data_length: Optional[int] = Form(None),
        repeat: Optional[int] = Form(1),
        write_type: Optional[int] = Form(None),
        read: Optional[int] = Form(None),
        ecc: Optional[int] = Form(None),
        delay: Optional[int] = Form(0),
        offset: Optional[int] = Form(None)
        # image_type: Optional[int] = Form(None),
        # filter_type: Optional[int] = Form(FilterTypeDef.FILTER_FF.value),
        # comp_type: Optional[int] = Form(CompTypeDef.LOSS_FF.value)
    ) -> "CommandFormModel":
        return cls(
            cmd_type=CmdTypeDef(cmd_type),
            data_length=data_length,
            repeat=repeat,
            write_type=W_DataTypeDef(write_type) if write_type is not None else None,
            read=Seq_ReadTypeDef(read) if read is not None else None,
            ecc=NASCERR_MODE(ecc) if ecc is not None else None,
            delay=delay,
            offset=offset
            # image_type=ImageTypeDef(image_type) if image_type is not None else None,
            # filter_type=FilterTypeDef(filter_type) if filter_type is not None else FilterTypeDef.FILTER_FF,
            # comp_type=CompTypeDef(comp_type) if comp_type is not None else CompTypeDef.LOSS_FF
        )

##############################################


class IDGenerator:
    # Gerar ID incrementall (DELETAR DEPOIS DE DB)
    def __init__(self):
        self.current_id = 0

    def generate_id(self):
        self.current_id += 1
        return self.current_id

class PersistentIDManager:
    def __init__(self, path="logs/last_id.json"):
        self.path = path
        os.makedirs(os.path.dirname(self.path), exist_ok=True)

    def _load_last_id(self):
        if os.path.exists(self.path):
            try:
                with open(self.path, "r", encoding="utf-8") as f:
                    return json.load(f).get("last_id", 0)
            except (json.JSONDecodeError, OSError):
                return 0
        return 0

    def _save_last_id(self, id_value):
        try:
            with open(self.path, "w", encoding="utf-8") as f:
                json.dump({"last_id": id_value}, f)
        except OSError:
            pass

    def generate_id(self):
        last_id = self._load_last_id()
        new_id = last_id + 1
        self._save_last_id(new_id)
        return new_id

    def get_current_id(self):
        return self._load_last_id()


##############################################
