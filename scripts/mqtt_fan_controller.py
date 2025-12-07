#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Controlador de Ventilador via MQTT para STM32F429ZI.

Este script conecta-se a um broker MQTT, monitora a temperatura publicada
pelo microcontrolador e decide automaticamente quando ligar ou desligar
o ventilador baseado em limiares configuráveis.

Uso:
    python mqtt_fan_controller.py [opções]

Exemplos:
    # Usar configurações padrão
    python mqtt_fan_controller.py

    # Configurar limiares de temperatura
    python mqtt_fan_controller.py --temp-on 45 --temp-off 38

    # Usar broker diferente
    python mqtt_fan_controller.py --broker localhost --port 1883

Configuração via variáveis de ambiente:
    MQTT_BROKER         - Endereço do broker (padrão: mqtt.rapport.tec.br)
    MQTT_PORT           - Porta do broker (padrão: 1883)
    MQTT_USERNAME       - Usuário para autenticação
    MQTT_PASSWORD       - Senha para autenticação
    MQTT_BASE_TOPIC     - Tópico base (padrão: racks)
    MQTT_RACK_NUMBER    - Identificador do rack (padrão: STM32F429)
    FAN_TEMP_ON         - Temperatura para ligar ventilador (padrão: 40.0)
    FAN_TEMP_OFF        - Temperatura para desligar ventilador (padrão: 35.0)

Autor: Cascade AI
Data: 2025-12-07
Versão: 1.0.0
"""

import argparse
import json
import logging
import os
import signal
import sys
import time
from dataclasses import dataclass
from datetime import datetime
from typing import Optional

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("Erro: Biblioteca paho-mqtt não encontrada.")
    print("Instale com: pip install paho-mqtt")
    sys.exit(1)


# =============================================================================
# Configuração de Logging
# =============================================================================

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger(__name__)


# =============================================================================
# Estruturas de Dados
# =============================================================================

@dataclass
class MqttConfig:
    """Configuração de conexão MQTT."""
    broker: str = "mqtt.rapport.tec.br"
    port: int = 1883
    username: str = ""
    password: str = ""
    base_topic: str = "racks"
    rack_number: str = "STM32F429"
    client_id: str = "python_fan_controller"
    keepalive: int = 60


@dataclass
class FanControlConfig:
    """Configuração de controle do ventilador."""
    temp_on: float = 40.0      # Liga ventilador acima desta temperatura
    temp_off: float = 35.0     # Desliga ventilador abaixo desta temperatura
    hysteresis: float = 2.0    # Histerese para evitar oscilação


@dataclass
class SystemState:
    """Estado atual do sistema."""
    connected: bool = False
    fan_on: bool = False
    last_temperature: Optional[float] = None
    last_update: Optional[datetime] = None
    messages_received: int = 0
    commands_sent: int = 0


# =============================================================================
# Controlador Principal
# =============================================================================

class MqttFanController:
    """
    Controlador de ventilador baseado em temperatura via MQTT.
    
    Monitora a temperatura publicada pelo STM32 e envia comandos
    para ligar/desligar o ventilador conforme os limiares configurados.
    """

    def __init__(self, mqtt_config: MqttConfig, fan_config: FanControlConfig):
        """
        Inicializa o controlador.

        Args:
            mqtt_config: Configuração de conexão MQTT.
            fan_config: Configuração de controle do ventilador.
        """
        self.mqtt_config = mqtt_config
        self.fan_config = fan_config
        self.state = SystemState()
        self.running = True
        
        # Tópicos MQTT
        self.topic_temperature = f"{mqtt_config.base_topic}/{mqtt_config.rack_number}/temperature"
        self.topic_fan_command = f"{mqtt_config.base_topic}/{mqtt_config.rack_number}/fan/command"
        self.topic_fan_status = f"{mqtt_config.base_topic}/{mqtt_config.rack_number}/fan/status"
        self.topic_status = f"{mqtt_config.base_topic}/{mqtt_config.rack_number}/status"
        
        # Cliente MQTT
        self.client = mqtt.Client(
            client_id=mqtt_config.client_id,
            protocol=mqtt.MQTTv311,
            clean_session=True
        )
        
        # Callbacks
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message
        
        # Autenticação
        if mqtt_config.username:
            self.client.username_pw_set(
                mqtt_config.username,
                mqtt_config.password
            )
        
        # Tratamento de sinais para saída graciosa
        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)

    def _signal_handler(self, signum, frame):
        """Trata sinais de interrupção para saída graciosa."""
        logger.info("Sinal de interrupção recebido. Encerrando...")
        self.running = False

    def _on_connect(self, client, userdata, flags, rc):
        """Callback chamado quando conecta ao broker."""
        if rc == 0:
            self.state.connected = True
            logger.info(f"Conectado ao broker {self.mqtt_config.broker}:{self.mqtt_config.port}")
            
            # Subscrever aos tópicos
            topics = [
                (self.topic_temperature, 1),
                (self.topic_fan_status, 1),
                (self.topic_status, 1),
            ]
            
            for topic, qos in topics:
                client.subscribe(topic, qos)
                logger.info(f"Subscrito ao tópico: {topic}")
        else:
            error_messages = {
                1: "Versão de protocolo incorreta",
                2: "Identificador de cliente inválido",
                3: "Servidor indisponível",
                4: "Usuário ou senha incorretos",
                5: "Não autorizado",
            }
            error_msg = error_messages.get(rc, f"Erro desconhecido ({rc})")
            logger.error(f"Falha na conexão: {error_msg}")

    def _on_disconnect(self, client, userdata, rc):
        """Callback chamado quando desconecta do broker."""
        self.state.connected = False
        if rc == 0:
            logger.info("Desconectado do broker (normal)")
        else:
            logger.warning(f"Desconectado do broker inesperadamente (rc={rc})")

    def _on_message(self, client, userdata, msg):
        """Callback chamado quando recebe uma mensagem."""
        self.state.messages_received += 1
        topic = msg.topic
        
        try:
            payload = msg.payload.decode('utf-8')
            
            if topic == self.topic_temperature:
                self._handle_temperature(payload)
            elif topic == self.topic_fan_status:
                self._handle_fan_status(payload)
            elif topic == self.topic_status:
                self._handle_device_status(payload)
            else:
                logger.debug(f"Mensagem em tópico não tratado: {topic}")
                
        except Exception as e:
            logger.error(f"Erro ao processar mensagem: {e}")

    def _handle_temperature(self, payload: str):
        """
        Processa mensagem de temperatura e decide sobre o ventilador.

        Args:
            payload: Payload JSON com a temperatura.
        """
        try:
            data = json.loads(payload)
            temperature = float(data.get('temperature', 0))
            unit = data.get('unit', 'C')
            
            self.state.last_temperature = temperature
            self.state.last_update = datetime.now()
            
            logger.info(f"Temperatura recebida: {temperature:.1f}°{unit}")
            
            # Lógica de controle com histerese
            self._evaluate_fan_control(temperature)
            
        except json.JSONDecodeError:
            # Tenta interpretar como valor numérico simples
            try:
                temperature = float(payload)
                self.state.last_temperature = temperature
                self.state.last_update = datetime.now()
                logger.info(f"Temperatura recebida: {temperature:.1f}°C")
                self._evaluate_fan_control(temperature)
            except ValueError:
                logger.warning(f"Payload de temperatura inválido: {payload}")

    def _handle_fan_status(self, payload: str):
        """
        Processa mensagem de status do ventilador.

        Args:
            payload: Payload JSON com o status.
        """
        try:
            data = json.loads(payload)
            fan_state = data.get('fan', '').lower()
            changed_by = data.get('changed_by', 'unknown')
            
            self.state.fan_on = (fan_state == 'on')
            logger.info(f"Status do ventilador: {fan_state.upper()} (alterado por: {changed_by})")
            
        except json.JSONDecodeError:
            # Tenta interpretar como string simples
            fan_state = payload.strip().lower()
            self.state.fan_on = (fan_state in ['on', '1', 'true', 'ligado'])
            logger.info(f"Status do ventilador: {'ON' if self.state.fan_on else 'OFF'}")

    def _handle_device_status(self, payload: str):
        """
        Processa mensagem de status geral do dispositivo.

        Args:
            payload: Payload com o status.
        """
        logger.debug(f"Status do dispositivo: {payload}")

    def _evaluate_fan_control(self, temperature: float):
        """
        Avalia se deve ligar ou desligar o ventilador.

        Implementa lógica de histerese para evitar oscilação:
        - Liga quando temperatura >= temp_on
        - Desliga quando temperatura <= temp_off
        - Mantém estado atual entre temp_off e temp_on

        Args:
            temperature: Temperatura atual em graus Celsius.
        """
        temp_on = self.fan_config.temp_on
        temp_off = self.fan_config.temp_off
        
        # Ligar ventilador se temperatura muito alta
        if temperature >= temp_on and not self.state.fan_on:
            logger.warning(f"Temperatura ALTA ({temperature:.1f}°C >= {temp_on}°C) - Ligando ventilador")
            self._send_fan_command("on")
            
        # Desligar ventilador se temperatura suficientemente baixa
        elif temperature <= temp_off and self.state.fan_on:
            logger.info(f"Temperatura OK ({temperature:.1f}°C <= {temp_off}°C) - Desligando ventilador")
            self._send_fan_command("off")
            
        else:
            # Zona de histerese - mantém estado atual
            status = "LIGADO" if self.state.fan_on else "DESLIGADO"
            logger.debug(f"Temperatura: {temperature:.1f}°C - Ventilador {status} (mantido)")

    def _send_fan_command(self, command: str):
        """
        Envia comando para o ventilador via MQTT.

        Args:
            command: Comando a enviar ('on' ou 'off').
        """
        if not self.state.connected:
            logger.error("Não conectado ao broker. Comando não enviado.")
            return
        
        result = self.client.publish(
            self.topic_fan_command,
            command,
            qos=1,
            retain=False
        )
        
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            self.state.commands_sent += 1
            logger.info(f"Comando enviado: {command.upper()} -> {self.topic_fan_command}")
        else:
            logger.error(f"Falha ao enviar comando: {result.rc}")

    def connect(self) -> bool:
        """
        Conecta ao broker MQTT.

        Returns:
            True se conectou com sucesso, False caso contrário.
        """
        try:
            logger.info(f"Conectando a {self.mqtt_config.broker}:{self.mqtt_config.port}...")
            self.client.connect(
                self.mqtt_config.broker,
                self.mqtt_config.port,
                self.mqtt_config.keepalive
            )
            return True
        except Exception as e:
            logger.error(f"Erro ao conectar: {e}")
            return False

    def disconnect(self):
        """Desconecta do broker MQTT."""
        if self.state.connected:
            self.client.disconnect()
            logger.info("Desconectado do broker")

    def run(self):
        """Loop principal do controlador."""
        if not self.connect():
            logger.error("Não foi possível conectar ao broker. Encerrando.")
            return
        
        # Inicia loop de rede em thread separada
        self.client.loop_start()
        
        logger.info("=" * 60)
        logger.info("Controlador de Ventilador MQTT Iniciado")
        logger.info("=" * 60)
        logger.info(f"Broker: {self.mqtt_config.broker}:{self.mqtt_config.port}")
        logger.info(f"Tópico temperatura: {self.topic_temperature}")
        logger.info(f"Tópico comando: {self.topic_fan_command}")
        logger.info(f"Limiar para LIGAR: {self.fan_config.temp_on}°C")
        logger.info(f"Limiar para DESLIGAR: {self.fan_config.temp_off}°C")
        logger.info("=" * 60)
        logger.info("Pressione Ctrl+C para encerrar")
        logger.info("")
        
        try:
            while self.running:
                # Exibe status periódico
                if self.state.last_temperature is not None:
                    status = "LIGADO" if self.state.fan_on else "DESLIGADO"
                    elapsed = ""
                    if self.state.last_update:
                        delta = datetime.now() - self.state.last_update
                        elapsed = f" (há {delta.seconds}s)"
                    
                    logger.debug(
                        f"[STATUS] Temp: {self.state.last_temperature:.1f}°C{elapsed} | "
                        f"Fan: {status} | "
                        f"Msgs: {self.state.messages_received} | "
                        f"Cmds: {self.state.commands_sent}"
                    )
                
                time.sleep(1)
                
        except KeyboardInterrupt:
            pass
        finally:
            self.client.loop_stop()
            self.disconnect()
            
            logger.info("")
            logger.info("=" * 60)
            logger.info("Resumo da Sessão")
            logger.info("=" * 60)
            logger.info(f"Mensagens recebidas: {self.state.messages_received}")
            logger.info(f"Comandos enviados: {self.state.commands_sent}")
            if self.state.last_temperature is not None:
                logger.info(f"Última temperatura: {self.state.last_temperature:.1f}°C")
            logger.info("=" * 60)


# =============================================================================
# Funções Auxiliares
# =============================================================================

def load_config_from_env() -> tuple[MqttConfig, FanControlConfig]:
    """
    Carrega configuração de variáveis de ambiente.

    Returns:
        Tupla com (MqttConfig, FanControlConfig).
    """
    mqtt_config = MqttConfig(
        broker=os.getenv('MQTT_BROKER', 'mqtt.rapport.tec.br'),
        port=int(os.getenv('MQTT_PORT', '1883')),
        username=os.getenv('MQTT_USERNAME', 'rack'),
        password=os.getenv('MQTT_PASSWORD', 'rack4567senha'),
        base_topic=os.getenv('MQTT_BASE_TOPIC', 'racks'),
        rack_number=os.getenv('MQTT_RACK_NUMBER', 'STM32F429'),
        client_id=os.getenv('MQTT_CLIENT_ID', f'python_fan_ctrl_{os.getpid()}'),
    )
    
    fan_config = FanControlConfig(
        temp_on=float(os.getenv('FAN_TEMP_ON', '40.0')),
        temp_off=float(os.getenv('FAN_TEMP_OFF', '35.0')),
    )
    
    return mqtt_config, fan_config


def parse_arguments() -> argparse.Namespace:
    """
    Processa argumentos de linha de comando.

    Returns:
        Namespace com os argumentos processados.
    """
    parser = argparse.ArgumentParser(
        description='Controlador de Ventilador via MQTT para STM32F429ZI',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Exemplos:
  %(prog)s                              # Usar configurações padrão/env
  %(prog)s --temp-on 45 --temp-off 38   # Configurar limiares
  %(prog)s --broker localhost           # Usar broker local
  %(prog)s -v                           # Modo verboso
        """
    )
    
    # Configuração do broker
    parser.add_argument(
        '--broker', '-b',
        help='Endereço do broker MQTT (padrão: mqtt.rapport.tec.br)',
        default=None
    )
    parser.add_argument(
        '--port', '-p',
        type=int,
        help='Porta do broker MQTT (padrão: 1883)',
        default=None
    )
    parser.add_argument(
        '--username', '-u',
        help='Usuário para autenticação MQTT',
        default=None
    )
    parser.add_argument(
        '--password', '-P',
        help='Senha para autenticação MQTT',
        default=None
    )
    
    # Configuração de tópicos
    parser.add_argument(
        '--base-topic',
        help='Tópico base MQTT (padrão: racks)',
        default=None
    )
    parser.add_argument(
        '--rack',
        help='Identificador do rack (padrão: STM32F429)',
        default=None
    )
    
    # Configuração de controle
    parser.add_argument(
        '--temp-on',
        type=float,
        help='Temperatura para ligar ventilador (padrão: 40.0°C)',
        default=None
    )
    parser.add_argument(
        '--temp-off',
        type=float,
        help='Temperatura para desligar ventilador (padrão: 35.0°C)',
        default=None
    )
    
    # Opções gerais
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Modo verboso (DEBUG)'
    )
    parser.add_argument(
        '--quiet', '-q',
        action='store_true',
        help='Modo silencioso (apenas erros)'
    )
    
    return parser.parse_args()


def main():
    """Função principal."""
    args = parse_arguments()
    
    # Configurar nível de log
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    elif args.quiet:
        logging.getLogger().setLevel(logging.ERROR)
    
    # Carregar configuração base do ambiente
    mqtt_config, fan_config = load_config_from_env()
    
    # Sobrescrever com argumentos de linha de comando
    if args.broker:
        mqtt_config.broker = args.broker
    if args.port:
        mqtt_config.port = args.port
    if args.username:
        mqtt_config.username = args.username
    if args.password:
        mqtt_config.password = args.password
    if args.base_topic:
        mqtt_config.base_topic = args.base_topic
    if args.rack:
        mqtt_config.rack_number = args.rack
    if args.temp_on:
        fan_config.temp_on = args.temp_on
    if args.temp_off:
        fan_config.temp_off = args.temp_off
    
    # Validar configuração
    if fan_config.temp_off >= fan_config.temp_on:
        logger.error("Erro: temp-off deve ser menor que temp-on")
        sys.exit(1)
    
    # Criar e executar controlador
    controller = MqttFanController(mqtt_config, fan_config)
    controller.run()


if __name__ == '__main__':
    main()
