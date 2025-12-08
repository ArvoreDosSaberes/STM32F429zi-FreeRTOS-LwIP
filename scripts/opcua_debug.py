#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Script de debug para testar servidor OPC-UA do STM32.
Mostra exatamente o que o servidor retorna em GetEndpoints.

Uso:
    python opcua_debug.py                    # Modo normal
    python opcua_debug.py --debug            # Modo debug (verbose)
    python opcua_debug.py --server IP:PORT   # Servidor customizado
"""

import asyncio
import sys
import logging
import argparse

from asyncua import Client
from asyncua import ua

# Configuração padrão
DEFAULT_SERVER = "192.168.0.228"
DEFAULT_PORT = 4840

async def run_test(server_host: str, server_port: int):
    """Testa conexão e lista endpoints."""
    server_url = f"opc.tcp://{server_host}:{server_port}"
    print(f"\n=== Testando servidor OPC-UA: {server_url} ===\n")
    
    client = Client(url=server_url)
    
    try:
        # Conectar ao socket
        print("1. Conectando ao socket...")
        await client.uaclient.connect_socket(server_host, server_port)
        print("   OK - Socket conectado")
        
        # Enviar Hello
        print("2. Enviando Hello...")
        await client.uaclient.send_hello(server_url)
        print("   OK - Hello enviado e ACK recebido")
        
        # Abrir SecureChannel com parâmetros
        print("3. Abrindo SecureChannel...")
        params = ua.OpenSecureChannelParameters()
        params.RequestType = ua.SecurityTokenRequestType.Issue
        params.SecurityMode = ua.MessageSecurityMode.None_
        params.RequestedLifetime = 3600000
        params.ClientNonce = b'\x00' * 32
        await client.uaclient.open_secure_channel(params)
        print("   OK - SecureChannel aberto")
        
        # Obter endpoints
        print("4. Obtendo endpoints (GetEndpoints)...")
        params = ua.GetEndpointsParameters()
        params.EndpointUrl = server_url
        endpoints = await client.uaclient.get_endpoints(params)
        
        print(f"\n=== {len(endpoints)} Endpoint(s) encontrado(s) ===\n")
        
        for i, ep in enumerate(endpoints):
            print(f"--- Endpoint {i+1} ---")
            print(f"  EndpointUrl: {ep.EndpointUrl}")
            print(f"  SecurityMode: {ep.SecurityMode} (None=1, Sign=2, SignAndEncrypt=3)")
            print(f"  SecurityPolicyUri: {ep.SecurityPolicyUri}")
            print(f"  TransportProfileUri: {ep.TransportProfileUri}")
            print(f"  SecurityLevel: {ep.SecurityLevel}")
            
            if ep.Server:
                print(f"  Server.ApplicationUri: {ep.Server.ApplicationUri}")
                print(f"  Server.ProductUri: {ep.Server.ProductUri}")
                print(f"  Server.ApplicationName: {ep.Server.ApplicationName}")
                print(f"  Server.ApplicationType: {ep.Server.ApplicationType}")
            
            if ep.UserIdentityTokens:
                print(f"  UserIdentityTokens: {len(ep.UserIdentityTokens)}")
                for j, token in enumerate(ep.UserIdentityTokens):
                    print(f"    Token {j+1}: PolicyId={token.PolicyId}, TokenType={token.TokenType}")
            
            print()
        
        # Verificar se há endpoint compatível
        print("5. Procurando endpoint compatível (SecurityMode=None, SecurityPolicy=None)...")
        compatible = None
        for ep in endpoints:
            if ep.SecurityMode == ua.MessageSecurityMode.None_ and \
               "None" in str(ep.SecurityPolicyUri):
                compatible = ep
                break
        
        if compatible:
            print(f"   OK - Endpoint compatível encontrado: {compatible.EndpointUrl}")
        else:
            print("   ERRO - Nenhum endpoint compatível encontrado!")
            print("   O cliente asyncua espera:")
            print("     - SecurityMode = 1 (None)")
            print("     - SecurityPolicyUri contendo 'None'")
        
        # Fechar canal
        print("\n6. Fechando SecureChannel...")
        await client.uaclient.close_secure_channel()
        print("   OK")
        
    except Exception as e:
        print(f"\nERRO: {e}")
        import traceback
        traceback.print_exc()
    
    finally:
        try:
            await client.uaclient.disconnect_socket()
        except:
            pass
    
    print("\n=== Teste concluído ===")


def main():
    """Ponto de entrada principal com argumentos de linha de comando."""
    parser = argparse.ArgumentParser(
        description='Script de debug para testar servidor OPC-UA do STM32',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Exemplos:
  python opcua_debug.py                           # Modo normal
  python opcua_debug.py --debug                   # Modo debug (verbose)
  python opcua_debug.py --server 192.168.1.100    # Servidor customizado
  python opcua_debug.py --server 192.168.1.100 --port 4841
        """
    )
    
    parser.add_argument(
        '--server', '-s',
        default=DEFAULT_SERVER,
        help=f'Endereço IP do servidor OPC-UA (padrão: {DEFAULT_SERVER})'
    )
    
    parser.add_argument(
        '--port', '-p',
        type=int,
        default=DEFAULT_PORT,
        help=f'Porta do servidor OPC-UA (padrão: {DEFAULT_PORT})'
    )
    
    parser.add_argument(
        '--debug', '-d',
        action='store_true',
        help='Habilita modo debug com logs detalhados'
    )
    
    args = parser.parse_args()
    
    # Configurar logging baseado no modo
    if args.debug:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.WARNING)
    
    # Executar teste
    asyncio.run(run_test(args.server, args.port))


if __name__ == '__main__':
    main()
